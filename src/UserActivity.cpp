#include "UserActivity.h"

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>
#elif __APPLE__
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#endif

#include <chrono>
#include <thread>

UserActivity::UserActivity() {
    resetIdleTimer();
}

UserActivity::~UserActivity() = default;

bool UserActivity::isUserIdle(int idleThresholdSeconds) {
#ifdef _WIN32
    return isUserIdleWindows(idleThresholdSeconds);
#else
    // For other platforms, use the timer-based approach
    auto now = std::chrono::steady_clock::now();
    auto idleDuration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivityTime);

    return idleDuration.count() > idleThresholdSeconds;
#endif
}

int UserActivity::getIdleTimeSeconds() {
#ifdef _WIN32
    return getIdleTimeSecondsWindows();
#else
    auto now = std::chrono::steady_clock::now();
    auto idleDuration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivityTime);

    return static_cast<int>(idleDuration.count());
#endif
}

void UserActivity::resetIdleTimer() {
    lastActivityTime = std::chrono::steady_clock::now();
}

#ifdef _WIN32
bool UserActivity::isUserIdleWindows(int idleThresholdSeconds) {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);

    if (GetLastInputInfo(&lii)) {
        DWORD idleTimeMs = GetTickCount() - lii.dwTime;
        DWORD idleTimeSec = idleTimeMs / 1000;
        return idleTimeSec > idleThresholdSeconds;
    }
    return true; // If we can't get the info, assume idle
}

int UserActivity::getIdleTimeSecondsWindows() {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);

    if (GetLastInputInfo(&lii)) {
        DWORD idleTimeMs = GetTickCount() - lii.dwTime;
        return static_cast<int>(idleTimeMs / 1000);
    }
    return -1; // Error
}
#endif