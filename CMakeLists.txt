cmake_minimum_required(VERSION 3.10)
project(RemoteWorkerCpp)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# Option to enable FFmpeg support
option(ENABLE_FFMPEG "Enable FFmpeg support" ON)

set(FFMPEG_FOUND FALSE)
if(ENABLE_FFMPEG)
    # Try to find FFmpeg first
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(AVCODEC QUIET libavcodec)
        pkg_check_modules(AVFORMAT QUIET libavformat)
        pkg_check_modules(AVUTIL QUIET libavutil)
        pkg_check_modules(SWSCALE QUIET libswscale)
        if(AVCODEC_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
            set(FFMPEG_FOUND TRUE)
            set(WITH_FFMPEG ON)
        endif()
    endif()

    # If pkg-config didn't work, try CMake's find_package for FFmpeg
    if(NOT FFMPEG_FOUND)
        find_package(FFMPEG QUIET COMPONENTS avcodec avformat avutil swscale)
        if(FFMPEG_FOUND)
            set(AVCODEC_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
            set(AVFORMAT_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
            set(AVUTIL_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
            set(SWSCALE_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
            set(AVCODEC_LIBRARIES ${FFMPEG_avcodec_LIBRARIES})
            set(AVFORMAT_LIBRARIES ${FFMPEG_avformat_LIBRARIES})
            set(AVUTIL_LIBRARIES ${FFMPEG_avutil_LIBRARIES})
            set(SWSCALE_LIBRARIES ${FFMPEG_swscale_LIBRARIES})
            set(WITH_FFMPEG ON)
        endif()
    endif()

    # If no FFmpeg found, try looking for it in common locations
    if(NOT FFMPEG_FOUND)
        # Check for FFmpeg in common install locations
        find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
            PATHS
                ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/include
                $ENV{FFMPEG_ROOT}/include
                $ENV{PROGRAMFILES}/ffmpeg/include
                C:/ffmpeg/include
                /usr/local/include
                /usr/include
        )

        if(FFMPEG_INCLUDE_DIR)
            set(FFMPEG_FOUND TRUE)
            set(WITH_FFMPEG ON)
            set(AVCODEC_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
            set(AVFORMAT_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
            set(AVUTIL_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
            set(SWSCALE_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})

            # Try to find libraries
            find_library(AVCODEC_LIBRARY avcodec
                PATHS
                    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/lib
                    $ENV{FFMPEG_ROOT}/lib
                    $ENV{PROGRAMFILES}/ffmpeg/lib
                    C:/ffmpeg/lib
                    /usr/local/lib
                    /usr/lib
            )
            find_library(AVFORMAT_LIBRARY avformat
                PATHS
                    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/lib
                    $ENV{FFMPEG_ROOT}/lib
                    $ENV{PROGRAMFILES}/ffmpeg/lib
                    C:/ffmpeg/lib
                    /usr/local/lib
                    /usr/lib
            )
            find_library(AVUTIL_LIBRARY avutil
                PATHS
                    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/lib
                    $ENV{FFMPEG_ROOT}/lib
                    $ENV{PROGRAMFILES}/ffmpeg/lib
                    C:/ffmpeg/lib
                    /usr/local/lib
                    /usr/lib
            )
            find_library(SWSCALE_LIBRARY swscale
                PATHS
                    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg/lib
                    $ENV{FFMPEG_ROOT}/lib
                    $ENV{PROGRAMFILES}/ffmpeg/lib
                    C:/ffmpeg/lib
                    /usr/local/lib
                    /usr/lib
            )

            set(AVCODEC_LIBRARIES ${AVCODEC_LIBRARY})
            set(AVFORMAT_LIBRARIES ${AVFORMAT_LIBRARY})
            set(AVUTIL_LIBRARIES ${AVUTIL_LIBRARY})
            set(SWSCALE_LIBRARIES ${SWSCALE_LIBRARY})
        else()
            message(STATUS "FFmpeg not found in common locations, screen recording will be disabled")
            set(WITH_FFMPEG OFF)
        endif()
    endif()
else()
    message(STATUS "FFmpeg support disabled by user option")
    set(WITH_FFMPEG OFF)
endif()

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

# Define NO_TRAY_LIBRARY to disable tray functionality until properly configured
target_compile_definitions(${PROJECT_NAME} PRIVATE NO_TRAY_LIBRARY)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs/imgui
    ${PROJECT_SOURCE_DIR}/libs/mysql-connector-c/include
)

# Platform specific includes
if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/build/_deps/glfw-src/include
    )
endif()

# Define WITH_FFMPEG compile definition
if(DEFINED WITH_FFMPEG AND WITH_FFMPEG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WITH_FFMPEG)
endif()

# FFmpeg include directories
if(WIN32 AND FFMPEG_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
elseif(NOT WIN32 AND PkgConfig_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
        ${SWSCALE_INCLUDE_DIRS}
        ${AVFILTER_INCLUDE_DIRS}
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
    Threads::Threads
    OpenGL::GL
    glfw
)

# Add FFmpeg libraries based on platform
if(WIN32 AND FFMPEG_FOUND)
    target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
elseif(NOT WIN32 AND PkgConfig_FOUND)
    target_link_libraries(${PROJECT_NAME}
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        ${AVFILTER_LIBRARIES}
    )
endif()

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