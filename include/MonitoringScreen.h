#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "AppState.h"  // Include to get MonitoringState definition

// Forward declaration to avoid circular dependencies
class ScreenCapture;

class MonitoringScreen {
public:
    MonitoringScreen();
    ~MonitoringScreen();

    void render();

    void setUserId(const std::string& userId);

    // Public methods to control monitoring externally (for system tray)
    void triggerStartMonitoring();
    void triggerStopMonitoring();
    void startBackgroundMonitoring();  // Start only background monitoring (activity/network) without recording/screenshots

private:
    std::string userId;
    std::string statusMessage;
    MonitoringState currentState;
    bool isRecording;

    // For the random screenshot timer
    std::thread screenshotTimerThread;
    std::atomic<bool> timerRunning;

    // Mutex to protect shared resources between threads
    std::mutex screenCaptureMutex;

    // For screen recording functionality
    ScreenCapture* screenCapture;

    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();

    void startRandomScreenshotTimer();
    void stopRandomScreenshotTimer();
};