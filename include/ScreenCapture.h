#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <vector>

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
    bool startNewRecordingSegment();  // For segmented recording
    bool stopCurrentRecordingSegment();  // For segmented recording

    // Set callback for when a screenshot is taken
    void setScreenshotCallback(std::function<void(const std::string&)> callback);

private:
    std::atomic<bool> isRecording;
    std::thread recordingThread;
    std::function<void(const std::string&)> screenshotCallback;

    // Screen dimensions (to be captured during initialization)
    int screenWidth;
    int screenHeight;

    // For external FFmpeg recording
    std::string outputFile;
    std::string tempFrameDir;
    int frameCounter;
    std::vector<std::string> capturedFrameFiles;
    std::vector<std::string> recordingSegments;  // For segmented recording
    std::string currentSegmentFile;  // Current segment being recorded
    int segmentCounter;  // Counter for segment files

    // GDI+ variables
    ULONG_PTR gdiplusToken;

#ifdef _WIN32
    // Process information for managing FFmpeg subprocess on Windows
    PROCESS_INFORMATION processInfo;
    bool ffmpegProcessRunning;
#endif

    // Platform-specific implementation
    std::string captureScreenWindows();
    std::string captureScreenLinux();
    std::string captureScreenMac();

    // Recording functions using external FFmpeg
    void recordingLoop();
    void startFFmpegScreenCapture();
    void stopFFmpegScreenCapture();
    void encodeVideoWithExternalFFmpeg();
    void createTempFrameDirectory();
    void cleanupTempFiles();
    void mergeRecordingSegments();
    std::string createSegmentFileName();

    // Windows-specific functions
    bool captureFrameWindows(const std::string& filePath);
    bool captureFrameLinux(const std::string& filePath);
    bool captureFrameMac(const std::string& filePath);

#ifdef _WIN32
    // Helper function for getting encoder CLSID
    friend int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
#endif
};