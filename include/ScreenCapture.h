#pragma once

#include <string>
#include <functional>

class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture();
    
    // Take a single screenshot
    std::string captureScreen();
    
    // Start/stop screen recording
    bool startRecording(const std::string& outputFilePath);
    bool stopRecording();
    
    // Set callback for when a screenshot is taken
    void setScreenshotCallback(std::function<void(const std::string&)> callback);
    
private:
    bool isRecording;
    std::function<void(const std::string&)> screenshotCallback;
    
    // Platform-specific implementation
    std::string captureScreenWindows();
    std::string captureScreenLinux();
    std::string captureScreenMac();
};