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

private:
    std::string userId;
    std::string statusMessage;
    MonitoringState currentState;
    bool isRecording;

    // For the random screenshot timer
    std::thread screenshotTimerThread;
    std::atomic<bool> timerRunning;

    // For screen recording functionality
    ScreenCapture* screenCapture;

    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();

    void startRandomScreenshotTimer();
    void stopRandomScreenshotTimer();
};