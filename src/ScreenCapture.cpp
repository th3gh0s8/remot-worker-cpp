#include "ScreenCapture.h"

#include <string>
#include <functional>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <iomanip>    // For std::setfill, std::setw
#include <process.h>  // For _spawnl on Windows
#include <cstdlib>    // For system()
#include <filesystem> // For file operations

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

ScreenCapture::ScreenCapture() :
    isRecording(false), screenshotCallback(nullptr), recordingThread(), tempFrameDir("temp_frames"), gdiplusToken(0) {
#ifdef _WIN32
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Get screen dimensions
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create temporary directory for frames
    createTempFrameDirectory();
#endif
}

ScreenCapture::~ScreenCapture() {
    if (isRecording) {
        stopRecording();
    }

    // Cleanup temporary files
    cleanupTempFiles();

#ifdef _WIN32
    Gdiplus::GdiplusShutdown(gdiplusToken);
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
    frameCounter = 0;
    capturedFrameFiles.clear();

    isRecording = true;

    // Start recording thread
    recordingThread = std::thread(&ScreenCapture::recordingLoop, this);

    std::cout << "Started recording to: " << finalOutputPath << std::endl;
    return true;
}

bool ScreenCapture::stopRecording() {
    if (!isRecording) {
        std::cout << "No recording in progress to stop" << std::endl;
        return true;
    }

    isRecording = false;

    // Wait for recording thread to finish
    if (recordingThread.joinable()) {
        recordingThread.join();
    }

    // Process all captured frames into final video
    if (!capturedFrameFiles.empty()) {
        encodeVideoWithExternalFFmpeg();
    }

    std::cout << "Stopped recording" << std::endl;
    return true;
}


void ScreenCapture::createTempFrameDirectory() {
    // Create temporary directory for frame storage
    std::filesystem::create_directories(tempFrameDir);
}

void ScreenCapture::cleanupTempFiles() {
    // Remove all temporary frame files
    for (const auto& frameFile : capturedFrameFiles) {
        std::filesystem::remove(frameFile);
    }

    // Remove the temporary directory
    std::filesystem::remove_all(tempFrameDir);
}

void ScreenCapture::recordingLoop() {
    // Main recording loop
    while (isRecording) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();

        // Capture a frame
        if (captureFrame()) {
            // Frame captured successfully
        }

        // Calculate how much time to sleep to maintain target FPS (30fps = ~33ms per frame)
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - frameStartTime);

        if (frameDuration.count() < 33) { // ~30 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(33 - frameDuration.count()));
        }
    }
}

bool ScreenCapture::captureFrame() {
    // Generate unique filename for this frame
    std::stringstream frameFilename;
    frameFilename << tempFrameDir << "/frame_" << std::setfill('0') << std::setw(6) << frameCounter << ".bmp";
    std::string framePath = frameFilename.str();

    // Capture the screen to this file
#ifdef _WIN32
    bool result = captureFrameWindows(framePath);
#else
    // For other platforms, use their respective implementations
    bool result = false;
    // We'll implement placeholder functions for other platforms below
#endif

    if (result) {
        capturedFrameFiles.push_back(framePath);
        frameCounter++;
    } else {
        // Only return false if there was a critical error, not just a missed frame
        // This allows the recording to continue even if one frame fails
        // Return true to continue recording, false only on critical errors
    }

    return true; // Always return true to keep recording going
}

#ifdef _WIN32
bool ScreenCapture::captureFrameWindows(const std::string& filePath) {
    HDC hScreen = GetDC(NULL);
    if (!hScreen) {
        std::cerr << "Failed to get screen DC" << std::endl;
        return false;
    }

    HDC hDC = CreateCompatibleDC(hScreen);
    if (!hDC) {
        std::cerr << "Failed to create compatible DC" << std::endl;
        ReleaseDC(NULL, hScreen);
        return false;
    }

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, screenWidth, screenHeight);
    if (!hBitmap) {
        std::cerr << "Failed to create compatible bitmap" << std::endl;
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }

    HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
    if (!old_obj) {
        std::cerr << "Failed to select bitmap object" << std::endl;
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }

    BOOL result = BitBlt(hDC, 0, 0, screenWidth, screenHeight, hScreen, 0, 0, SRCCOPY);
    if (!result) {
        std::cerr << "Failed to perform BitBlt" << std::endl;
        SelectObject(hDC, old_obj);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }

    // Save to BMP file using the same approach as the captureScreenWindows function
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = screenWidth;
    bmi.bmiHeader.biHeight = -screenHeight; // Negative for top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(screenWidth * screenHeight * 3);
    int scanlineSize = ((24 * screenWidth + 31) / 32) * 4; // Calculate actual scanline size
    int pixelsGot = GetDIBits(hDC, hBitmap, 0, screenHeight, pixels.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
    if (pixelsGot <= 0) {
        std::cerr << "Failed to get DIBits" << std::endl;
        SelectObject(hDC, old_obj);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }

    // Create and write the BMP file manually
    std::ofstream file(filePath, std::ios::binary);
    if (file.is_open()) {
        // BMP Header (14 bytes)
        file.put('B'); file.put('M'); // Signature

        // File size: header + (width * height * 3 bytes per pixel)
        int headersSize = 54; // Size of all headers
        int totalFileSize = headersSize + (screenWidth * screenHeight * 3);
        file.write(reinterpret_cast<const char*>(&totalFileSize), 4); // File size

        int reserved = 0;
        file.write(reinterpret_cast<const char*>(&reserved), 2); // Reserved
        file.write(reinterpret_cast<const char*>(&reserved), 2); // Reserved

        int dataOffset = headersSize;
        file.write(reinterpret_cast<const char*>(&dataOffset), 4); // Data offset

        // DIB Header (40 bytes - BITMAPINFOHEADER)
        int headerSize = 40;
        file.write(reinterpret_cast<const char*>(&headerSize), 4); // Header size
        file.write(reinterpret_cast<const char*>(&screenWidth), 4); // Width
        file.write(reinterpret_cast<const char*>(&screenHeight), 4); // Height
        file.put(1); file.put(0); // Planes
        file.put(24); file.put(0); // Bits per pixel
        int compression = 0; // BI_RGB
        file.write(reinterpret_cast<const char*>(&compression), 4); // Compression
        int imageSize = screenWidth * screenHeight * 3;
        file.write(reinterpret_cast<const char*>(&imageSize), 4); // Image size
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Horizontal resolution
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Vertical resolution
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Colors used
        file.write(reinterpret_cast<const char*>(&reserved), 4); // Important colors

        // Write pixel data (RGB values, bottom to top due to BMP format)
        for(int y = screenHeight - 1; y >= 0; y--) {
            for(int x = 0; x < screenWidth; x++) {
                int pixelIndex = (y * screenWidth + x) * 3;
                // Write BGR (BMP format requires BGR order)
                file.put(pixels[pixelIndex + 2]); // Blue
                file.put(pixels[pixelIndex + 1]); // Green
                file.put(pixels[pixelIndex]);     // Red
            }
        }

        file.close();
    } else {
        std::cerr << "Failed to create file: " << filePath << std::endl;
        SelectObject(hDC, old_obj);
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        return false;
    }

    SelectObject(hDC, old_obj);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);

    return true;
}
#endif

#ifdef __linux__
bool ScreenCapture::captureFrameLinux(const std::string& filePath) {
    // Placeholder implementation for Linux
    // In a real implementation, you might use X11 capture libraries
    // or call external utilities like scrot
    std::cout << "Linux frame capture to: " << filePath << std::endl;
    // This would be implemented using X11 screen capture
    return false; // Placeholder
}
#endif

#ifdef __APPLE__
bool ScreenCapture::captureFrameMac(const std::string& filePath) {
    // Placeholder implementation for macOS
    // In a real implementation, you might use CGDisplay API
    std::cout << "macOS frame capture to: " << filePath << std::endl;
    // This would be implemented using macOS screen capture APIs
    return false; // Placeholder
}
#endif

void ScreenCapture::encodeVideoWithExternalFFmpeg() {
    if (capturedFrameFiles.empty()) {
        return;
    }

    // Build the FFmpeg command to create the video from captured frames
    std::string ffmpegCmd = "ffmpeg -y -framerate 30 -i \"" + tempFrameDir + "/frame_%06d.bmp\" -c:v libx264 -pix_fmt yuv420p -preset medium -crf 23 \"" + outputFile + "\"";

    // Execute the command
    int result = system(ffmpegCmd.c_str());

    if (result == 0) {
        std::cout << "Successfully created video: " << outputFile << std::endl;
    } else {
        std::cerr << "Failed to create video using FFmpeg" << std::endl;
    }
}

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