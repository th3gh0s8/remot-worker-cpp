#include "AppState.h"
#include <chrono>

AppStateManager::AppStateManager() : monitoringState(MonitoringState::STOPPED) {
    setSessionStartTime();
}

AppStateManager::~AppStateManager() = default;

void AppStateManager::setUserId(const std::string& id) {
    std::lock_guard<std::mutex> lock(stateMutex);
    userId = id;
}

std::string AppStateManager::getUserId() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return userId;
}

void AppStateManager::setMonitoringState(MonitoringState state) {
    std::lock_guard<std::mutex> lock(stateMutex);
    monitoringState = state;
}

MonitoringState AppStateManager::getMonitoringState() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return monitoringState;
}

void AppStateManager::setSessionStartTime() {
    std::lock_guard<std::mutex> lock(stateMutex);
    sessionStartTime = std::chrono::steady_clock::now();
}

double AppStateManager::getSessionDuration() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - sessionStartTime);
    return duration.count() / 1000.0; // Convert to seconds
}

void AppStateManager::lock() {
    stateMutex.lock();
}

void AppStateManager::unlock() {
    stateMutex.unlock();
}