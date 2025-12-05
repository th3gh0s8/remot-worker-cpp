#pragma once

#include <string>
#include <mutex>

enum class MonitoringState {
    STOPPED,
    RUNNING,
    PAUSED
};

class AppStateManager {
public:
    AppStateManager();
    ~AppStateManager();
    
    void setUserId(const std::string& userId);
    std::string getUserId() const;
    
    void setMonitoringState(MonitoringState state);
    MonitoringState getMonitoringState() const;
    
    void setSessionStartTime();
    double getSessionDuration() const; // in seconds
    
    void lock();
    void unlock();
    
private:
    std::string userId;
    MonitoringState monitoringState;
    std::chrono::steady_clock::time_point sessionStartTime;
    mutable std::mutex stateMutex;
};