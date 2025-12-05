cmake_minimum_required(VERSION 3.10)
project(RemoteWorkerCpp)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# Find or install GLFW
include(FetchContent)

# Option 1: Try to find GLFW first
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    # If not found, fetch and build it
    message(STATUS "GLFW not found, downloading...")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)
endif()

# Add executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/LoginScreen.cpp
    src/MonitoringScreen.cpp
    src/DatabaseManager.cpp
    src/ScreenCapture.cpp
    src/UserActivity.cpp
    src/NetworkMonitor.cpp
    src/FileUploader.cpp
    src/AppState.cpp
    src/RemoteWorkerApp.cpp
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs/imgui
    ${PROJECT_SOURCE_DIR}/libs/mysql-connector-c/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
    Threads::Threads
    OpenGL::GL
    glfw
)

# Platform specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        winmm
        ws2_32
        opengl32
        gdi32
        user32
    )
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa" "-framework IOKit" "-framework Carbon")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(${PROJECT_NAME} PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})
endif()