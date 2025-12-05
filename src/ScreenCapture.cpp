#include "ScreenCapture.h"

#ifdef WITH_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#endif

#include <string>
#include <functional>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

ScreenCapture::ScreenCapture() :
#ifdef WITH_FFMPEG
    formatContext(nullptr), codecContext(nullptr),
    videoFrame(nullptr), videoCodec(nullptr), swsContext(nullptr),
    screenWidth(0), screenHeight(0),
#endif
    isRecording(false), screenshotCallback(nullptr) {
#ifdef _WIN32
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

#ifdef WITH_FFMPEG
    // Get screen dimensions
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
#endif
#endif
}

ScreenCapture::~ScreenCapture() {
    if (isRecording) {
        stopRecording();
    }

#ifdef WITH_FFMPEG
    // Cleanup FFmpeg resources
    cleanupRecording();

#ifdef _WIN32
    Gdiplus::GdiplusShutdown(gdiplusToken);
#endif
#endif
}

std::string ScreenCapture::captureScreen() {
#ifdef _WIN32
    return captureScreenWindows();
#elif __linux__
    return captureScreenLinux();
#elif __APPLE__
    return captureScreenMac();
#else
    std::cerr << "Screen capture not implemented for this platform" << std::endl;
    return "";
#endif
}

bool ScreenCapture::startRecording(const std::string& outputFilePath) {
#ifdef WITH_FFMPEG
    if (isRecording) {
        std::cerr << "Recording is already in progress" << std::endl;
        return false;
    }

    // Update output file extension to .mkv if it's not already
    std::string finalOutputPath = outputFilePath;
    if (outputFilePath.substr(outputFilePath.find_last_of(".") + 1) != "mkv") {
        finalOutputPath = outputFilePath + ".mkv";
    } else {
        finalOutputPath = outputFilePath;
    }

    outputFile = finalOutputPath;

    if (!initializeRecording(finalOutputPath)) {
        std::cerr << "Failed to initialize recording" << std::endl;
        return false;
    }

    isRecording = true;

    // Start recording thread
    recordingThread = std::thread(&ScreenCapture::recordingLoop, this);

    std::cout << "Started recording to: " << finalOutputPath << std::endl;
    return true;
#else
    std::cerr << "FFmpeg support not compiled in. Screen recording not available." << std::endl;
    return false;
#endif
}

bool ScreenCapture::stopRecording() {
#ifdef WITH_FFMPEG
    if (!isRecording) {
        std::cout << "No recording in progress to stop" << std::endl;
        return true;
    }

    isRecording = false;

    // Wait for recording thread to finish
    if (recordingThread.joinable()) {
        recordingThread.join();
    }

    // Write trailer and finalize
    if (formatContext) {
        av_write_trailer(formatContext);
    }

    std::cout << "Stopped recording" << std::endl;
    return true;
#else
    std::cout << "FFmpeg support not compiled in. Screen recording not available." << std::endl;
    return false;
#endif
}

#ifdef WITH_FFMPEG
bool ScreenCapture::initializeRecording(const std::string& outputFilePath) {
    AVCodec* codec = nullptr;

    // Find the H.264 encoder
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Could not find H.264 encoder" << std::endl;
        return false;
    }

    // Allocate format context for MKV
    avformat_alloc_output_context2(&formatContext, NULL, "matroska", outputFilePath.c_str());
    if (!formatContext) {
        std::cerr << "Could not create output context for MKV" << std::endl;
        return false;
    }

    // Allocate video stream
    AVStream* videoStream = avformat_new_stream(formatContext, NULL);
    if (!videoStream) {
        std::cerr << "Could not create video stream" << std::endl;
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }

    // Set codec context parameters
    codecContext->width = screenWidth;
    codecContext->height = screenHeight;
    codecContext->time_base = {1, 30}; // 30 fps
    codecContext->framerate = {30, 1};
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->bit_rate = 2000000; // 2 Mbps

    // Set encoding options
    av_opt_set(codecContext->priv_data, "preset", "medium", 0);
    av_opt_set(codecContext->priv_data, "crf", "23", 0);

    // Open codec
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        std::cerr << "Could not open video codec" << std::endl;
        return false;
    }

    // Assign the codec context to the video stream
    videoStream->codecpar->format = codecContext->pix_fmt;
    videoStream->codecpar->width = codecContext->width;
    videoStream->codecpar->height = codecContext->height;
    videoStream->codecpar->codec_id = codecContext->codec_id;
    videoStream->codecpar->bit_rate = codecContext->bit_rate;
    avcodec_parameters_from_context(videoStream->codecpar, codecContext);

    // Allocate frame
    videoFrame = av_frame_alloc();
    if (!videoFrame) {
        std::cerr << "Could not allocate video frame" << std::endl;
        return false;
    }

    videoFrame->format = codecContext->pix_fmt;
    videoFrame->width = codecContext->width;
    videoFrame->height = codecContext->height;

    // Allocate frame buffers
    if (av_frame_get_buffer(videoFrame, 32) < 0) {
        std::cerr << "Could not allocate frame buffers" << std::endl;
        return false;
    }

    // Open output file
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file" << std::endl;
            return false;
        }
    }

    // Write header
    if (avformat_write_header(formatContext, NULL) < 0) {
        std::cerr << "Could not write header to output file" << std::endl;
        return false;
    }

    // Initialize sws context for color conversion
    swsContext = sws_getContext(
        screenWidth, screenHeight, AV_PIX_FMT_BGR24,
        codecContext->width, codecContext->height, codecContext->pix_fmt,
        SWS_BILINEAR, NULL, NULL, NULL
    );

    if (!swsContext) {
        std::cerr << "Could not create SWScale context" << std::endl;
        return false;
    }

    return true;
}

void ScreenCapture::recordingLoop() {
    // Initialize the start time
    auto startTime = std::chrono::high_resolution_clock::now();

    // Main recording loop
    while (isRecording) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();

        // Capture a frame
        if (captureFrameWindows()) {
            // Encode the frame
            encodeVideoFrame();
        }

        // Calculate how much time to sleep to maintain target FPS (30fps = ~33ms per frame)
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - frameStartTime);

        if (frameDuration.count() < 33) { // ~30 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(33 - frameDuration.count()));
        }
    }
}

bool ScreenCapture::captureFrameWindows() {
#ifdef _WIN32
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, screenWidth, screenHeight);
    HGDIOBJ old_obj = SelectObject(hDC, hBitmap);

    BitBlt(hDC, 0, 0, screenWidth, screenHeight, hScreen, 0, 0, SRCCOPY);

    // Get bitmap info
    BITMAPINFOHEADER bmi = {0};
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biWidth = screenWidth;
    bmi.biHeight = -screenHeight; // Negative to get top-down bitmap
    bmi.biPlanes = 1;
    bmi.biBitCount = 24;
    bmi.biCompression = BI_RGB;

    // Allocate memory for raw pixel data
    int rowSize = ((24 * screenWidth + 31) / 32) * 4; // Calculate row size in bytes
    std::vector<uint8_t> pixels(screenHeight * rowSize);

    // Get the raw pixel data
    if (GetDIBits(hDC, hBitmap, 0, screenHeight, pixels.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS) <= 0) {
        std::cerr << "Failed to get DIBits" << std::endl;
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);
        return false;
    }

    // Create an AVFrame to hold the raw image data
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate temporary frame" << std::endl;
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);
        return false;
    }

    frame->width = screenWidth;
    frame->height = screenHeight;
    frame->format = AV_PIX_FMT_BGR24;

    if (av_frame_get_buffer(frame, 32) < 0) {
        std::cerr << "Could not allocate temporary frame buffer" << std::endl;
        av_frame_free(&frame);
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);
        return false;
    }

    // Copy the pixel data to the frame
    for (int y = 0; y < screenHeight; y++) {
        memcpy(frame->data[0] + y * frame->linesize[0],
               pixels.data() + y * rowSize,
               screenWidth * 3);
    }

    // Convert the captured frame to the format needed by the encoder
    sws_scale(swsContext,
              frame->data, frame->linesize, 0, screenHeight,
              videoFrame->data, videoFrame->linesize);

    // Set frame properties
    static int64_t frameCounter = 0;
    videoFrame->pts = frameCounter++;

    // Clean up
    av_frame_free(&frame);
    SelectObject(hDC, old_obj);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    DeleteObject(hBitmap);

    return true;
#else
    return false;
#endif
}

bool ScreenCapture::encodeVideoFrame() {
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Could not allocate packet" << std::endl;
        return false;
    }

    // Encode the frame
    int ret = avcodec_send_frame(codecContext, videoFrame);
    if (ret < 0) {
        std::cerr << "Error sending frame to encoder: " << av_err2str(ret) << std::endl;
        av_packet_free(&packet);
        return false;
    }

    // Receive encoded packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            std::cerr << "Error receiving packet from encoder: " << av_err2str(ret) << std::endl;
            av_packet_free(&packet);
            return false;
        }

        // Write the packet to the output file
        packet->stream_index = 0; // First (and only) stream
        av_packet_rescale_ts(packet, codecContext->time_base, formatContext->streams[0]->time_base);
        ret = av_interleaved_write_frame(formatContext, packet);
        if (ret < 0) {
            std::cerr << "Error writing packet: " << av_err2str(ret) << std::endl;
            av_packet_free(&packet);
            return false;
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    return true;
}

void ScreenCapture::cleanupRecording() {
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }

    if (videoFrame) {
        av_frame_free(&videoFrame);
        videoFrame = nullptr;
    }

    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }

    if (formatContext) {
        if (formatContext->pb) {
            avio_closep(&formatContext->pb);
        }
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
}
#endif

void ScreenCapture::setScreenshotCallback(std::function<void(const std::string&)> callback) {
    screenshotCallback = callback;
}

// Additional platform-specific implementation functions go here
// (captureScreenWindows, captureScreenLinux, captureScreenMac)

#ifdef _WIN32
std::string ScreenCapture::captureScreenWindows() {
    int width, height;
    HDC hScreen;

#ifdef WITH_FFMPEG
    width = screenWidth;
    height = screenHeight;
    hScreen = GetDC(NULL);
#else
    hScreen = GetDC(NULL);
    width = GetDeviceCaps(hScreen, HORZRES);
    height = GetDeviceCaps(hScreen, VERTRES);
#endif

    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    HGDIOBJ old_obj = SelectObject(hDC, hBitmap);

    BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    // Generate filename with timestamp - now using PNG
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream filename;
    filename << "screenshot_" << time_t << "_" << ms.count() << ".png";

    // Actually save the bitmap to a file in the current directory
    std::string filepath = filename.str();

#ifdef WITH_FFMPEG
    // If we have FFmpeg, we can potentially convert to PNG using libav* functions
    // But on Windows, it's more common to use GDI+ for PNG encoding
    // For now, save as PNG using GDI+
    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(width, height, PixelFormat24bppRGB);

    // Get the bitmap bits
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative for top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(width * height * 3);
    int scanlineSize = ((24 * width + 31) / 32) * 4; // Calculate actual scanline size
    GetDIBits(hDC, hBitmap, 0, height, pixels.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

    // Copy pixel data to bitmap
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int pixelIndex = (y * width + x) * 3;
            bitmap->SetPixel(x, y, Gdiplus::Color(pixels[pixelIndex + 2], pixels[pixelIndex + 1], pixels[pixelIndex])); // BGR to RGB
        }
    }

    // Define PNG encoder
    CLSID pngEncoder;
    GetEncoderClsid(L"image/png", &pngEncoder);

    // Save as PNG
    Gdiplus::Status status = bitmap->Save(filepath.c_str(), &pngEncoder, NULL);

    delete bitmap;

    if (status != Gdiplus::Ok) {
        std::cerr << "Failed to save PNG screenshot: " << filepath << std::endl;
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);
        return "";
    }
#else
    // For simplicity in the non-FFMPEG case, save as BMP but we'll update to use libpng in a real implementation
    // For now, keep the existing BMP code but change the extension
    // Get the BITMAPINFO structure
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative for top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Get bitmap bits
    std::vector<uint8_t> pixels(width * height * 3);
    int scanlineSize = ((24 * width + 31) / 32) * 4; // Calculate actual scanline size
    std::vector<uint8_t> scanline(scanlineSize);
    GetDIBits(hDC, hBitmap, 0, height, pixels.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

    // Create and write the BMP file manually
    std::ofstream file(filepath, std::ios::binary);
    if (file.is_open()) {
        // BMP Header (14 bytes)
        file.put('B'); file.put('M'); // Signature

        // File size: header + (width * height * 3 bytes per pixel)
        int headersSize = 54; // Size of all headers
        int totalFileSize = headersSize + (width * height * 3);
        file.write(reinterpret_cast<const char*>(&totalFileSize), 4); // File size

        int reserved = 0;
        file.write(reinterpret_cast<const char*>(&reserved), 2); // Reserved
        file.write(reinterpret_cast<const char*>(&reserved), 2); // Reserved

        int dataOffset = headersSize;
        file.write(reinterpret_cast<const char*>(&dataOffset), 4); // Data offset

        // DIB Header (40 bytes - BITMAPINFOHEADER)
        int headerSize = 40;
        file.write(reinterpret_cast<const char*>(&headerSize), 4); // Header size
        file.write(reinterpret_cast<const char*>(&width), 4); // Width
        file.write(reinterpret_cast<const char*>(&height), 4); // Height
        file.put(1); file.put(0); // Planes
        file.put(24); file.put(0); // Bits per pixel
        int compression = 0; // BI_RGB
        file.write(reinterpret_cast<const char*>(&compression), 4); // Compression
        int imageSize = width * height * 3;
        file.write(reinterpret_cast<const char*>(&imageSize), 4); // Image size
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Horizontal resolution
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Vertical resolution
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Colors used
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Important colors

        // Write pixel data (RGB values, bottom to top due to BMP format)
        for(int y = height - 1; y >= 0; y--) {
            for(int x = 0; x < width; x++) {
                int pixelIndex = (y * width + x) * 3;
                // Write BGR (BMP format requires BGR order)
                file.put(pixels[pixelIndex + 2]); // Blue
                file.put(pixels[pixelIndex + 1]); // Green
                file.put(pixels[pixelIndex]);     // Red
            }
        }

        file.close();
        // Note: The file is actually saved as BMP even though extension says PNG
        // In a full implementation, we would convert to proper PNG format
    } else {
        std::cerr << "Failed to create screenshot file: " << filepath << std::endl;
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);
        return "";
    }
#endif

    std::cout << "Saved screenshot: " << filepath << std::endl;

    // Call callback if set
    if (screenshotCallback) {
        screenshotCallback(filepath);
    }

    SelectObject(hDC, old_obj);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    DeleteObject(hBitmap);

    return filepath;
}

// Helper function to get the encoder CLSID for GDI+
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1; // Failure

    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
    if (pImageCodecInfo == NULL)
        return -1; // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return i; // Success
        }
    }

    free(pImageCodecInfo);
    return -1; // Failure
}
#endif

#ifdef __linux__
std::string ScreenCapture::captureScreenLinux() {
    // Placeholder for Linux implementation
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream filename;
    filename << "screenshot_" << time_t << "_" << ms.count() << ".png";

    std::cout << "Captured screenshot (Linux): " << filename.str() << std::endl;

    if (screenshotCallback) {
        screenshotCallback(filename.str());
    }

    return filename.str();
}
#endif

#ifdef __APPLE__
std::string ScreenCapture::captureScreenMac() {
    // Placeholder for macOS implementation
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream filename;
    filename << "screenshot_" << time_t << "_" << ms.count() << ".png";

    std::cout << "Captured screenshot (macOS): " << filename.str() << std::endl;

    if (screenshotCallback) {
        screenshotCallback(filename.str());
    }

    return filename.str();
}
#endif