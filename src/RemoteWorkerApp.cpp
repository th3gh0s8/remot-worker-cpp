#include "RemoteWorkerApp.h"
#include "LoginScreen.h"
#include "MonitoringScreen.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#ifdef _WIN32
#include <GLFW/glfw3.h>
#else
#include <GLFW/glfw3.h>
#endif

#include <iostream>

// Include tray library if available
#ifndef NO_TRAY_LIBRARY
#ifdef USE_TRAY_LIBRARY
#include "tray.h"
#endif
#endif

RemoteWorkerApp::RemoteWorkerApp() : window(nullptr), currentState(AppState::LOGIN) {
#ifndef NO_TRAY_LIBRARY
#ifdef USE_TRAY_LIBRARY
    trayMenu = nullptr;
#endif
#endif
    initialize();
}

RemoteWorkerApp::~RemoteWorkerApp() {
    cleanup();
}

void RemoteWorkerApp::initialize() {
    // Setup window
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "Glfw Error " << error << ": " << description << std::endl;
    });

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    // Decide GL+GLSL versions
#ifdef __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Create window with graphics context
    window = glfwCreateWindow(1280, 720, "Remote Worker Monitoring", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize screens
    loginScreen = std::make_unique<::LoginScreen>();
    monitoringScreen = std::make_unique<::MonitoringScreen>();

#ifndef NO_TRAY_LIBRARY
#ifdef USE_TRAY_LIBRARY
    setupSystemTray();
#endif
#endif
}

#ifndef NO_TRAY_LIBRARY
#ifdef USE_TRAY_LIBRARY
void RemoteWorkerApp::trayCallback(struct tray_menu* menu) {
    RemoteWorkerApp* app = static_cast<RemoteWorkerApp*>(menu->context);

    if (strcmp(menu->text, "Show Window") == 0) {
        glfwShowWindow(app->window);
    } else if (strcmp(menu->text, "Start Monitoring") == 0) {
        if (app->currentState == AppState::MONITORING) {
            app->monitoringScreen->triggerStartMonitoring();
        }
    } else if (strcmp(menu->text, "Stop Monitoring") == 0) {
        if (app->currentState == AppState::MONITORING) {
            app->monitoringScreen->triggerStopMonitoring();
        }
    } else if (strcmp(menu->text, "Exit") == 0) {
        glfwSetWindowShouldClose(app->window, GLFW_TRUE);
    }
}

void RemoteWorkerApp::setupSystemTray() {
    static struct tray_menu menu_items[] = {
        {.text = "Show Window", .cb = RemoteWorkerApp::trayCallback, .context = nullptr},
        {.text = "Start Monitoring", .cb = RemoteWorkerApp::trayCallback, .context = nullptr},
        {.text = "Stop Monitoring", .cb = RemoteWorkerApp::trayCallback, .context = nullptr},
        {.text = "-", .cb = nullptr, .context = nullptr},  // Separator
        {.text = "Exit", .cb = RemoteWorkerApp::trayCallback, .context = nullptr},
        {}
    };

    // Set the context to this instance so the callback can access the app
    for (int i = 0; i < 5; i++) {  // Update the first 5 elements
        if (menu_items[i].text && strlen(menu_items[i].text) > 0 && menu_items[i].text[0] != '-') {
            menu_items[i].context = this;
        }
    }

    if (tray_init(menu_items) != 0) {
        std::cerr << "Failed to initialize system tray" << std::endl;
    } else {
        std::cout << "System tray initialized successfully" << std::endl;
    }
}
#endif
#endif

void RemoteWorkerApp::render() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    switch(currentState) {
        case AppState::LOGIN:
            loginScreen->render();
            if (loginScreen->isLoginSuccessful()) {
                monitoringScreen->setUserId(loginScreen->getUserId());
                currentState = AppState::MONITORING;
            }
            break;
        case AppState::MONITORING:
            monitoringScreen->render();
            break;
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void RemoteWorkerApp::cleanup() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}


void RemoteWorkerApp::run() {
    std::cout << "Remote Worker App starting..." << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle window minimized
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        if (width > 0 && height > 0) {
            render();
        }
    }
}