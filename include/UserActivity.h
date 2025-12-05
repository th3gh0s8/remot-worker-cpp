#pragma once

#include <chrono>

class UserActivity {
public:
    UserActivity();
    ~UserActivity();
    
    // Check if user is idle (no mouse/keyboard activity for specified time)
    bool isUserIdle(int idleThresholdSeconds = 300); // Default 5 minutes
    
    // Get idle time in seconds
    int getIdleTimeSeconds();
    
    // Reset idle timer (call when user has activity)
    void resetIdleTimer();
    
private:
    std::chrono::steady_clock::time_point lastActivityTime;

#ifdef _WIN32
    bool isUserIdleWindows(int idleThresholdSeconds);
    int getIdleTimeSecondsWindows();
#endif
};