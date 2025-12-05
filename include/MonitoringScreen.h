#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "AppState.h"  // Include to get MonitoringState definition

class MonitoringScreen {
public:
    MonitoringScreen();
    ~MonitoringScreen();

    void render();

    void setUserId(const std::string& userId);

private:
    std::string userId;
    std::string statusMessage;
    MonitoringState currentState;
    bool isRecording;

    // For the random screenshot timer
    std::thread screenshotTimerThread;
    std::atomic<bool> timerRunning;

    void startMonitoring();
    void stopMonitoring();
    void pauseMonitoring();
    void resumeMonitoring();

    void startRandomScreenshotTimer();
    void stopRandomScreenshotTimer();
};