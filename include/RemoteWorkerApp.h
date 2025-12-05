#pragma once

#include <string>
#include <memory>

struct GLFWwindow;

enum class AppState {
    LOGIN,
    MONITORING
};

// Forward declarations for screen classes
class LoginScreen;
class MonitoringScreen;

class RemoteWorkerApp {
public:
    RemoteWorkerApp();
    ~RemoteWorkerApp();

    void run();

private:
    void initialize();
    void render();
    void cleanup();

    GLFWwindow* window;
    AppState currentState;

    std::unique_ptr<LoginScreen> loginScreen;
    std::unique_ptr<MonitoringScreen> monitoringScreen;
};