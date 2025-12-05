#include "RemoteWorkerApp.h"

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
#include <memory>

int main(int, char**) {
    RemoteWorkerApp app;
    app.run();
    return 0;
}