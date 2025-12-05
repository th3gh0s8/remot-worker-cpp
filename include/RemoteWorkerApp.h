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

// Forward declaration for tray library (if using C library)
#ifdef USE_TRAY_LIBRARY
struct tray_menu;
#endif

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

#ifdef USE_TRAY_LIBRARY
    // System tray functionality
    void setupSystemTray();
    static void trayCallback(struct tray_menu* menu);
    struct tray_menu* trayMenu;
#endif
};