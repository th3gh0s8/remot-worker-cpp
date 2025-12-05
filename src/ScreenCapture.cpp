#include "ScreenCapture.h"

#include <string>
#include <functional>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

ScreenCapture::ScreenCapture() : isRecording(false), screenshotCallback(nullptr) {
#ifdef _WIN32
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
#endif
}

ScreenCapture::~ScreenCapture() {
    if (isRecording) {
        stopRecording();
    }
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
    // Placeholder implementation
    isRecording = true;
    std::cout << "Started recording to: " << outputFilePath << std::endl;
    return true;
}

bool ScreenCapture::stopRecording() {
    // Placeholder implementation
    isRecording = false;
    std::cout << "Stopped recording" << std::endl;
    return true;
}

void ScreenCapture::setScreenshotCallback(std::function<void(const std::string&)> callback) {
    screenshotCallback = callback;
}

#ifdef _WIN32
std::string ScreenCapture::captureScreenWindows() {
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);

    int width = GetDeviceCaps(hScreen, HORZRES);
    int height = GetDeviceCaps(hScreen, VERTRES);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    HGDIOBJ old_obj = SelectObject(hDC, hBitmap);

    BitBlt(hDC, 0, 0, width, height, hScreen, 0, 0, SRCCOPY);

    // Generate filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream filename;
    filename << "screenshot_" << time_t << "_" << ms.count() << ".bmp";

    // Actually save the bitmap to a file in the current directory
    std::string filepath = filename.str();

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
        std::cout << "Saved screenshot: " << filepath << std::endl;
    } else {
        std::cerr << "Failed to create screenshot file: " << filepath << std::endl;
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);
        return "";
    }

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