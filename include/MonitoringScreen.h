#pragma once

#include <string>
#include <thread>
#include <atomic>

class MonitoringScreen {
public:
    MonitoringScreen();
    ~MonitoringScreen();
    
    void render();
    
    void setUserId(const std::string& userId);
    
private:
    std::string userId;
    std::string statusMessage;
    
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