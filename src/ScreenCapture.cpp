#include "ScreenCapture.h"

#include <string>
#include <functional>
#include <chrono>
#include <sstream>
#include <iostream>

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
    
    // In a real implementation, you would save the bitmap to file
    // For now, just return the filename as if it was saved
    std::cout << "Captured screenshot: " << filename.str() << std::endl;
    
    // Call callback if set
    if (screenshotCallback) {
        screenshotCallback(filename.str());
    }
    
    SelectObject(hDC, old_obj);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
    DeleteObject(hBitmap);
    
    return filename.str();
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