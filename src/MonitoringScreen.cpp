#include "MonitoringScreen.h"
#include "AppState.h"
#include "ScreenCapture.h"
#include "UserActivity.h"
#include "NetworkMonitor.h"
#include "FileUploader.h"
#include "DatabaseManager.h"

#include "imgui.h"
#include <chrono>
#include <thread>
#include <random>
#include <iostream>

MonitoringScreen::MonitoringScreen() : timerRunning(false) {
    // Initialize monitoring components
}

MonitoringScreen::~MonitoringScreen() {
    if (timerRunning) {
        stopRandomScreenshotTimer();
    }
}

void MonitoringScreen::render() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);
    
    ImGui::Begin("Work Monitoring Dashboard", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    
    static MonitoringState currentState = MonitoringState::STOPPED;
    
    // Control buttons
    if (currentState == MonitoringState::STOPPED) {
        if (ImGui::Button("Start Monitoring")) {
            startMonitoring();
            currentState = MonitoringState::RUNNING;
        }
    } else if (currentState == MonitoringState::RUNNING) {
        if (ImGui::Button("Pause")) {
            pauseMonitoring();
            currentState = MonitoringState::PAUSED;
        }
        if (ImGui::Button("Stop")) {
            stopMonitoring();
            currentState = MonitoringState::STOPPED;
        }
    } else if (currentState == MonitoringState::PAUSED) {
        if (ImGui::Button("Resume")) {
            resumeMonitoring();
            currentState = MonitoringState::RUNNING;
        }
        if (ImGui::Button("Stop")) {
            stopMonitoring();
            currentState = MonitoringState::STOPPED;
        }
    }
    
    // Status information
    ImGui::Separator();
    ImGui::Text("Status: %s", 
        currentState == MonitoringState::RUNNING ? "Running" : 
        currentState == MonitoringState::PAUSED ? "Paused" : "Stopped");
    
    if (!statusMessage.empty()) {
        ImGui::Text("Info: %s", statusMessage.c_str());
    }
    
    // Show some stats
    UserActivity userActivity;
    bool isIdle = userActivity.isUserIdle(300); // 5 minutes threshold
    ImGui::Text("User Status: %s", isIdle ? "Idle" : "Active");
    
    NetworkMonitor networkMonitor;
    auto networkUsage = networkMonitor.getNetworkUsage();
    ImGui::Text("Network Usage - Sent: %llu bytes, Received: %llu bytes", 
                networkUsage.bytesSent, networkUsage.bytesReceived);
    
    ImGui::End();
}

void MonitoringScreen::setUserId(const std::string& id) {
    userId = id;
}

void MonitoringScreen::startMonitoring() {
    statusMessage = "Monitoring started...";
    
    // Start random screenshot timer
    startRandomScreenshotTimer();
    
    // Here you would start other monitoring components
    std::cout << "Monitoring started for user: " << userId << std::endl;
}

void MonitoringScreen::stopMonitoring() {
    statusMessage = "Monitoring stopped.";
    
    // Stop random screenshot timer
    stopRandomScreenshotTimer();
    
    std::cout << "Monitoring stopped for user: " << userId << std::endl;
}

void MonitoringScreen::pauseMonitoring() {
    statusMessage = "Monitoring paused.";
    std::cout << "Monitoring paused for user: " << userId << std::endl;
}

void MonitoringScreen::resumeMonitoring() {
    statusMessage = "Monitoring resumed.";
    
    // Restart random screenshot timer if needed
    startRandomScreenshotTimer();
    
    std::cout << "Monitoring resumed for user: " << userId << std::endl;
}

void MonitoringScreen::startRandomScreenshotTimer() {
    if (timerRunning) return;
    
    timerRunning = true;
    screenshotTimerThread = std::thread([this]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        // Random interval between 10-30 minutes (600-1800 seconds)
        std::uniform_int_distribution<> dis(600, 1800);
        
        while (timerRunning) {
            int interval = dis(gen);
            
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            
            if (!timerRunning) break;
            
            // Take screenshot
            ScreenCapture screenCapture;
            std::string screenshotPath = screenCapture.captureScreen();
            
            if (!screenshotPath.empty()) {
                // Upload screenshot
                FileUploader uploader;
                uploader.setServerCredentials("localhost", "root", "");
                uploader.uploadFile(screenshotPath, "/screenshots/" + userId + "/");
                
                // Record to database
                DatabaseManager dbManager;
                dbManager.connect("localhost", "root", "", "worker_db");
                dbManager.insertActivityData(userId, "screenshot_taken");
                
                statusMessage = "Screenshot taken and uploaded: " + screenshotPath;
            }
        }
    });
}

void MonitoringScreen::stopRandomScreenshotTimer() {
    if (!timerRunning) return;
    
    timerRunning = false;
    if (screenshotTimerThread.joinable()) {
        screenshotTimerThread.join();
    }
}