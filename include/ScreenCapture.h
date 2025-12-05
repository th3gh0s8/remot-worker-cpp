#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

// Forward declarations for FFmpeg structures
extern "C" {
    struct AVFormatContext;
    struct AVCodecContext;
    struct AVFrame;
    struct AVCodec;
    struct SwsContext;
}

#ifdef _WIN32
#include <windows.h>
// Forward declaration for CLSID
struct _GUID;
typedef _GUID CLSID;
#endif

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
    std::atomic<bool> isRecording;
    std::thread recordingThread;
    std::function<void(const std::string&)> screenshotCallback;

    // FFmpeg related members for recording
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    AVFrame* videoFrame;
    AVCodec* videoCodec;
    SwsContext* swsContext;
    std::string outputFile;

    // Screen dimensions (to be captured during initialization)
    int screenWidth;
    int screenHeight;

    // Platform-specific implementation
    std::string captureScreenWindows();
    std::string captureScreenLinux();
    std::string captureScreenMac();

    // Recording functions
    bool initializeRecording(const std::string& outputFilePath);
    void recordingLoop();
    bool prepareVideoFrame();
    bool encodeVideoFrame();
    void cleanupRecording();

    // Windows-specific functions for continuous capture
    bool captureFrameWindows();

#ifdef _WIN32
    // Helper function for getting encoder CLSID
    friend int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
#endif
};