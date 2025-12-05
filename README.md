# Remote Worker Monitoring Application

A cross-platform desktop application that monitors user work patterns using screen recording, 
screenshot capture, idle detection, and network usage monitoring.

## Features

- User authentication with remote MySQL database
- Screen recording and periodic screenshot capture
- User idle detection
- Network usage monitoring
- Remote data storage for activities and screenshots
- ImGui-based cross-platform GUI

## Dependencies

This application requires the following dependencies:

- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI
- [GLFW](https://github.com/glfw/glfw) - Window management
- [OpenGL](https://www.opengl.org/) - Graphics rendering
- [MySQL Connector/C++](https://dev.mysql.com/downloads/connector/cpp/) - Database connectivity
- Platform-specific libraries:
  - Windows: GDI+, WinMM, WS2_32
  - Linux: X11 libraries
  - macOS: Cocoa, IOKit frameworks

## Building

### Prerequisites

1. CMake 3.10 or higher
2. C++17 compatible compiler
3. Git for submodules

### Setup Instructions

1. Clone the repository with submodules:
   ```bash
   git clone --recursive https://github.com/your-username/remot-worker-cpp.git
   ```

2. Add ImGui as a submodule if not already included:
   ```bash
   cd remot-worker-cpp
   git submodule add https://github.com/ocornut/imgui.git libs/imgui
   ```

3. Create build directory:
   ```bash
   mkdir build && cd build
   ```

4. Configure with CMake:
   ```bash
   cmake ..
   ```

5. Build the project:
   ```bash
   cmake --build .
   ```

## Configuration

Before running the application, you need to configure the following in the source code:

1. Update database connection parameters in `src/LoginScreen.cpp`
2. Update file server credentials in `src/MonitoringScreen.cpp`
3. Adjust idle detection threshold in `include/UserActivity.h`

## Usage

1. Run the application
2. Enter your user ID in the login screen
3. Click "Login" to authenticate with the remote server
4. Use the "Start" button to begin monitoring
5. Use "Pause"/"Resume" to temporarily stop and resume monitoring
6. Use "Stop" to end the monitoring session

## Architecture

The application is organized into several components:

- `RemoteWorkerApp`: Main application class that manages the GUI and state
- `LoginScreen`: Handles user authentication
- `MonitoringScreen`: Main monitoring interface with controls
- `DatabaseManager`: Handles MySQL database operations
- `ScreenCapture`: Manages screen recording and screenshots
- `UserActivity`: Detects user idle state
- `NetworkMonitor`: Tracks network usage
- `FileUploader`: Uploads files to remote server
- `AppState`: Manages application state

## Security Considerations

This application has the capability to capture screenshots and record the screen.
Ensure that appropriate privacy notices are provided to users and that the application
complies with local privacy laws and regulations.

Database credentials should be properly secured and not hardcoded in production builds.

## Platform Support

The application is designed to work on:
- Windows
- Linux
- macOS

Platform-specific code is abstracted through conditional compilation.

## License

[Specify your license here]