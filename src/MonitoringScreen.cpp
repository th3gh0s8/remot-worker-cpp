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

MonitoringScreen::MonitoringScreen() : timerRunning(false), currentState(MonitoringState::STOPPED), isRecording(false), screenCapture(nullptr) {
    // Initialize monitoring components
    screenCapture = new ScreenCapture();
}

MonitoringScreen::~MonitoringScreen() {
    if (timerRunning) {
        stopRandomScreenshotTimer();
    }

    // Stop any ongoing recording before destroying the screenCapture object
    if (isRecording && screenCapture) {
        screenCapture->stopRecording();
        isRecording = false;
    }

    // Clean up the screen capture object
    if (screenCapture) {
        delete screenCapture;
        screenCapture = nullptr;
    }
}

void MonitoringScreen::render() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);

    ImGui::Begin("Work Monitoring Dashboard", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Control buttons
    if (currentState == MonitoringState::STOPPED) {
        if (ImGui::Button("Start")) {
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
        }
    } else if (currentState == MonitoringState::PAUSED) {
        if (ImGui::Button("Resume")) {
            resumeMonitoring();
            currentState = MonitoringState::RUNNING;
        }
        if (ImGui::Button("Stop")) {
            stopMonitoring();
        }
    }

    // Additional functionality buttons
    ImGui::Separator();
    if (ImGui::Button("Take Screenshot Now")) {
        std::string screenshotPath = screenCapture->captureScreen();

        if (!screenshotPath.empty()) {
            // Upload screenshot
            FileUploader uploader;
            uploader.setServerCredentials("localhost", "root", "");
            uploader.uploadFile(screenshotPath, "/screenshots/" + userId + "/");

            // Record to database
            DatabaseManager dbManager;
            dbManager.connect("localhost", "root", "", "worker_db");
            dbManager.insertActivityData(userId, "manual_screenshot_taken");

            statusMessage = "Manual screenshot taken: " + screenshotPath;
        } else {
            statusMessage = "Failed to take screenshot";
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

void MonitoringScreen::triggerStartMonitoring() {
    if (currentState == MonitoringState::STOPPED) {
        startMonitoring();
        currentState = MonitoringState::RUNNING;
    }
}

void MonitoringScreen::triggerStopMonitoring() {
    if (currentState != MonitoringState::STOPPED) {
        stopMonitoring();
        currentState = MonitoringState::STOPPED;
    }
}

void MonitoringScreen::startBackgroundMonitoring() {
    statusMessage = "Background monitoring started (activity, network, and screenshots)...";

    // Start random screenshot timer (but no recording)
    startRandomScreenshotTimer();

    std::cout << "Background monitoring started for user: " << userId << std::endl;
}

void MonitoringScreen::startMonitoring() {
    statusMessage = "Monitoring started...";

    // Start random screenshot timer
    startRandomScreenshotTimer();

    // Start recording when monitoring starts
    std::string recordingPath = "monitoring_recording_" + userId + ".mkv";
    if (screenCapture->startRecording(recordingPath)) {
        isRecording = true;
        statusMessage = "Started monitoring and recording: " + recordingPath;
    } else {
        statusMessage = "Started monitoring, but failed to start recording";
    }

    std::cout << "Monitoring started for user: " << userId << std::endl;
}

void MonitoringScreen::stopMonitoring() {
    statusMessage = "Recording and screenshoting stopped, monitoring continues.";

    // Stop random screenshot timer
    stopRandomScreenshotTimer();

    // Stop recording when monitoring stops, using mutex for safety
    {
        std::lock_guard<std::mutex> lock(screenCaptureMutex);
        if (isRecording && screenCapture) {
            // Additional safety check to ensure screenCapture is valid
            ScreenCapture* tempCapture = screenCapture;  // Copy pointer to local variable
            if (tempCapture) {
                tempCapture->stopRecording();
            }
            isRecording = false;  // Ensure we don't try to stop again
        }
    }

    // Keep monitoring state as RUNNING - only stop recording/screenshoting
    std::cout << "Recording and screenshoting stopped for user: " << userId << " (monitoring continues)" << std::endl;
}


void MonitoringScreen::pauseMonitoring() {
    statusMessage = "Monitoring paused.";

    // Pause recording by stopping the current segment
    if (isRecording && screenCapture) {
        screenCapture->stopCurrentRecordingSegment();
    }

    std::cout << "Monitoring paused for user: " << userId << std::endl;
}

void MonitoringScreen::resumeMonitoring() {
    statusMessage = "Monitoring resumed.";

    // Resume recording by starting a new segment
    if (isRecording && screenCapture) {
        screenCapture->startNewRecordingSegment();
    }

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
            
            // Take screenshot using the member ScreenCapture instance with mutex protection
            std::string screenshotPath;
            {
                std::lock_guard<std::mutex> lock(screenCaptureMutex);
                if (screenCapture) {
                    screenshotPath = screenCapture->captureScreen();
                }
            }
            
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