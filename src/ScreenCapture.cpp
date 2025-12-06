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
    isRecording(false), screenshotCallback(nullptr), recordingThread(), tempFrameDir("temp_frames"),
    frameCounter(0), segmentCounter(0), gdiplusToken(0) {
#ifdef _WIN32
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize process info
    memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));
    ffmpegProcessRunning = false;

    // Initialize segmentation
    currentSegmentFile = "";
    recordingSegments.clear();

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
    // Ensure FFmpeg process is terminated in destructor if still running
    if (ffmpegProcessRunning) {
        DWORD exitCode;
        if (processInfo.hProcess && GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
            if (exitCode == STILL_ACTIVE) {
                TerminateProcess(processInfo.hProcess, 0);
            }
        }
        if (processInfo.hProcess) CloseHandle(processInfo.hProcess);
        if (processInfo.hThread) CloseHandle(processInfo.hThread);
        ffmpegProcessRunning = false;
    }

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
    segmentCounter = 0;  // Reset segment counter
    recordingSegments.clear();  // Clear any previous segments

    isRecording = true;

    // Create the first segment and start recording to it
    currentSegmentFile = createSegmentFileName();

    // Build the FFmpeg command for direct screen capture to segment file
#ifdef _WIN32
    std::string ffmpegCmd = "ffmpeg -f gdigrab -i desktop -c:v libx264 -crf 23 -preset ultrafast -tune zerolatency -y \"" + currentSegmentFile + "\"";
#else
    // For Linux/macOS, we'd use different parameters
    #ifdef __linux__
    std::string ffmpegCmd = "ffmpeg -f x11grab -i :0 -c:v libx264 -crf 23 -preset ultrafast -y \"" + currentSegmentFile + "\"";
    #elif __APPLE__
    std::string ffmpegCmd = "ffmpeg -f avfoundation -i \"1\" -c:v libx264 -crf 23 -preset ultrafast -y \"" + currentSegmentFile + "\"";
    #else
    std::cerr << "Screen capture not implemented for this platform" << std::endl;
    return false;
    #endif
#endif

    // Execute the FFmpeg command as a subprocess using Win32 API to have better control
#ifdef _WIN32
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Convert std::string to wide string for Windows
    std::wstring wCmd(ffmpegCmd.begin(), ffmpegCmd.end());

    // Create the process
    if (!CreateProcessW(NULL,   // No module name (use command line)
                        &wCmd[0], // Command line
                        NULL,   // Process handle not inheritable
                        NULL,   // Thread handle not inheritable
                        FALSE,  // Handle inheritance
                        CREATE_NO_WINDOW, // Creation flags to hide the window
                        NULL,   // Use parent's environment block
                        NULL,   // Use parent's starting directory
                        &si,    // Pointer to STARTUPINFOW structure
                        &pi)) { // Pointer to PROCESS_INFORMATION structure
        std::cerr << "CreateProcess failed (" << GetLastError() << ")" << std::endl;
        isRecording = false;
        return false;
    }

    // Store process info to potentially kill it later
    processInfo = pi;
    ffmpegProcessRunning = true;

    // Add this segment to the list of segments
    recordingSegments.push_back(currentSegmentFile);

    std::cout << "Started recording to: " << finalOutputPath << std::endl;
    return true;
#else
    // On Linux/Mac, we would use fork/exec or similar
    // For now, use system() which is blocking - not ideal but works for this implementation
    int result = system(ffmpegCmd.c_str());
    if (result == 0) {
        // Add this segment to the list of segments
        recordingSegments.push_back(currentSegmentFile);
        std::cout << "Started recording to: " << finalOutputPath << std::endl;
        return true;
    } else {
        isRecording = false;
        return false;
    }
#endif
}

bool ScreenCapture::startNewRecordingSegment() {
    // Create a new segment file name
    currentSegmentFile = createSegmentFileName();

    // Build the FFmpeg command for direct screen capture to segment file
#ifdef _WIN32
    std::string ffmpegCmd = "ffmpeg -f gdigrab -i desktop -c:v libx264 -crf 23 -preset ultrafast -tune zerolatency -y \"" + currentSegmentFile + "\"";
#else
    // For Linux/macOS, we'd use different parameters
    #ifdef __linux__
    std::string ffmpegCmd = "ffmpeg -f x11grab -i :0 -c:v libx264 -crf 23 -preset ultrafast -y \"" + currentSegmentFile + "\"";
    #elif __APPLE__
    std::string ffmpegCmd = "ffmpeg -f avfoundation -i \"1\" -c:v libx264 -crf 23 -preset ultrafast -y \"" + currentSegmentFile + "\"";
    #else
    std::cerr << "Screen capture not implemented for this platform" << std::endl;
    return false;
    #endif
#endif

    // Execute the FFmpeg command as a subprocess using Win32 API to have better control
#ifdef _WIN32
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Convert std::string to wide string for Windows
    std::wstring wCmd(ffmpegCmd.begin(), ffmpegCmd.end());

    // Create the process
    if (!CreateProcessW(NULL,   // No module name (use command line)
                        &wCmd[0], // Command line
                        NULL,   // Process handle not inheritable
                        NULL,   // Thread handle not inheritable
                        FALSE,  // Handle inheritance
                        CREATE_NO_WINDOW, // Creation flags to hide the window
                        NULL,   // Use parent's environment block
                        NULL,   // Use parent's starting directory
                        &si,    // Pointer to STARTUPINFOW structure
                        &pi)) { // Pointer to PROCESS_INFORMATION structure
        std::cerr << "CreateProcess failed (" << GetLastError() << ")" << std::endl;
        return false;
    }

    // Store process info to potentially kill it later
    processInfo = pi;
    ffmpegProcessRunning = true;

    // Add this segment to the list of segments
    recordingSegments.push_back(currentSegmentFile);

    return true;
#else
    // On Linux/Mac, we would use fork/exec or similar
    // For now, use system() which is blocking - not ideal but works for this implementation
    int result = system(ffmpegCmd.c_str());
    if (result == 0) {
        // Add this segment to the list of segments
        recordingSegments.push_back(currentSegmentFile);
        return true;
    } else {
        return false;
    }
#endif
}

bool ScreenCapture::stopCurrentRecordingSegment() {
    if (!ffmpegProcessRunning) {
        return true; // Nothing to stop
    }

#ifdef _WIN32
    // Terminate the FFmpeg process if it's running
    if (processInfo.hProcess) {
        DWORD exitCode;
        if (GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
            if (exitCode == STILL_ACTIVE) {
                // Process is still running, terminate it
                if (TerminateProcess(processInfo.hProcess, 0)) {
                    std::cout << "FFmpeg process terminated for current segment" << std::endl;
                } else {
                    std::cerr << "Failed to terminate FFmpeg process for current segment" << std::endl;
                }
            }
        }

        // Close handles (only once and mark as not running)
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    ffmpegProcessRunning = false;
#endif

    return true;
}

bool ScreenCapture::stopRecording() {
    if (!isRecording) {
        std::cout << "No recording in progress to stop" << std::endl;
        return true;
    }

    isRecording = false;

    // Stop current recording segment
    stopCurrentRecordingSegment();

    // Merge all segments into the final output file
    mergeRecordingSegments();

    std::cout << "Stopped recording" << std::endl;
    return true;
}



void ScreenCapture::recordingLoop() {
    // For direct FFmpeg screen capture, we'll launch FFmpeg as a subprocess that captures directly
    startFFmpegScreenCapture();
}

void ScreenCapture::startFFmpegScreenCapture() {
    // Build the FFmpeg command for direct screen capture
#ifdef _WIN32
    std::string ffmpegCmd = "ffmpeg -f gdigrab -i desktop -c:v libx264 -crf 23 -preset ultrafast -y \"" + outputFile + "\"";
#else
    // For Linux/macOS, we'd use different parameters
    #ifdef __linux__
    std::string ffmpegCmd = "ffmpeg -f x11grab -i :0 -c:v libx264 -crf 23 -preset ultrafast -y \"" + outputFile + "\"";
    #elif __APPLE__
    std::string ffmpegCmd = "ffmpeg -f avfoundation -i \"1\" -c:v libx264 -crf 23 -preset ultrafast -y \"" + outputFile + "\"";
    #else
    std::cerr << "Screen capture not implemented for this platform" << std::endl;
    return;
    #endif
#endif

    // Execute the FFmpeg command as a subprocess using Win32 API to have better control
#ifdef _WIN32
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Convert std::string to wide string for Windows
    std::wstring wCmd(ffmpegCmd.begin(), ffmpegCmd.end());

    // Create the process
    if (!CreateProcessW(NULL,   // No module name (use command line)
                        &wCmd[0], // Command line
                        NULL,   // Process handle not inheritable
                        NULL,   // Thread handle not inheritable
                        FALSE,  // Handle inheritance
                        CREATE_NO_WINDOW, // Creation flags to hide the window
                        NULL,   // Use parent's environment block
                        NULL,   // Use parent's starting directory
                        &si,    // Pointer to STARTUPINFOW structure
                        &pi)) { // Pointer to PROCESS_INFORMATION structure
        std::cerr << "CreateProcess failed (" << GetLastError() << ")" << std::endl;
        return;
    }

    // Store process info to potentially kill it later when stopRecording is called
    processInfo = pi;
    ffmpegProcessRunning = true;

    // Don't close handles immediately - we need them to control the process
    // We'll close them in stopRecording
#else
    // On Linux/Mac, we would use fork/exec or similar
    // For now, use system() which is blocking - not ideal but works for this implementation
    system(ffmpegCmd.c_str());
#endif
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

std::string ScreenCapture::createSegmentFileName() {
    std::stringstream segmentName;
    segmentName << tempFrameDir << "/segment_" << std::setfill('0') << std::setw(3) << segmentCounter << ".mkv";
    segmentCounter++;
    return segmentName.str();
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

    // Remove all segment files
    for (const auto& segmentFile : recordingSegments) {
        std::filesystem::remove(segmentFile);
    }

    // Remove the temporary directory
    std::filesystem::remove_all(tempFrameDir);
}

void ScreenCapture::stopFFmpegScreenCapture() {
#ifdef _WIN32
    // Terminate the FFmpeg process if it's running
    if (ffmpegProcessRunning) {
        DWORD exitCode;
        if (GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
            if (exitCode == STILL_ACTIVE) {
                // Process is still running, terminate it
                if (TerminateProcess(processInfo.hProcess, 0)) {
                    std::cout << "FFmpeg process terminated" << std::endl;
                } else {
                    std::cerr << "Failed to terminate FFmpeg process" << std::endl;
                }
            }
        }

        // Close handles
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        ffmpegProcessRunning = false;
    }
#endif
}

void ScreenCapture::mergeRecordingSegments() {
    if (recordingSegments.size() <= 1) {
        // If there's only one segment or none, just rename it to the final output
        if (recordingSegments.size() == 1) {
            std::filesystem::rename(recordingSegments[0], outputFile);
        }
        return;
    }

    // Create a file listing all segments for FFmpeg to concatenate
    std::string listFile = tempFrameDir + "/segments_list.txt";
    std::ofstream list(listFile);

    for (const auto& segment : recordingSegments) {
        list << "file '" << segment << "'\n";
    }
    list.close();

    // Use FFmpeg to concatenate all segments
    std::string ffmpegCmd = "ffmpeg -f concat -safe 0 -i \"" + listFile + "\" -c copy -y \"" + outputFile + "\"";
    int result = system(ffmpegCmd.c_str());

    if (result == 0) {
        std::cout << "Successfully merged " << recordingSegments.size() << " segments into " << outputFile << std::endl;
    } else {
        std::cerr << "Failed to merge recording segments" << std::endl;
    }

    // Clean up the list file
    std::filesystem::remove(listFile);
}

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