#include "LoginScreen.h"
#include "DatabaseManager.h"

#include "imgui.h"
#include <string>
#include <cstring>

LoginScreen::LoginScreen() : loginSuccessful(false), connecting(false) {
    memset(userIdBuffer, 0, sizeof(userIdBuffer));
}

LoginScreen::~LoginScreen() = default;

void LoginScreen::render() {
    // Center the login window
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
    
    ImGui::Begin("User Login", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("Please enter your User ID:");
    ImGui::InputText("User ID", userIdBuffer, sizeof(userIdBuffer));
    
    if (!errorMessage.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());
    }
    
    if (connecting) {
        ImGui::Text("Connecting to server...");
        ImGui::ProgressBar(3.0f / 5.0f, ImVec2(-1.0f, 0.0f), "Connecting...");
    } else {
        if (ImGui::Button("Login")) {
            std::string userId(userIdBuffer);
            if (!userId.empty()) {
                connecting = true;
                
                // Create database manager and try to validate user
                DatabaseManager dbManager;
                if (dbManager.connect("localhost", "root", "", "worker_db")) {
                    if (dbManager.validateUser(userId)) {
                        loginSuccessful = true;
                    } else {
                        errorMessage = "Invalid User ID";
                    }
                } else {
                    errorMessage = "Cannot connect to server";
                }
                
                connecting = false;
            } else {
                errorMessage = "Please enter a User ID";
            }
        }
    }
    
    ImGui::End();
}

bool LoginScreen::isLoginSuccessful() const {
    return loginSuccessful;
}

std::string LoginScreen::getUserId() const {
    return std::string(userIdBuffer);
}

void LoginScreen::reset() {
    loginSuccessful = false;
    memset(userIdBuffer, 0, sizeof(userIdBuffer));
    errorMessage.clear();
}