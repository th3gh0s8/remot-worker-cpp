# This project implements a cross-platform desktop application that monitors user work patterns.
# The application has the following key components:

## Project Structure
```
remot-worker-cpp/
├── CMakeLists.txt          # Build configuration
├── README.md              # Project documentation
├── build.bat             # Windows build script
├── config.json           # Configuration file
├── include/              # Header files
│   ├── AppState.h
│   ├── DatabaseManager.h
│   ├── FileUploader.h
│   ├── LoginScreen.h
│   ├── MonitoringScreen.h
│   ├── NetworkMonitor.h
│   ├── RemoteWorkerApp.h
│   ├── ScreenCapture.h
│   └── UserActivity.h
├── src/                  # Source files
│   ├── AppState.cpp
│   ├── DatabaseManager.cpp
│   ├── FileUploader.cpp
│   ├── LoginScreen.cpp
│   ├── main.cpp
│   ├── MonitoringScreen.cpp
│   ├── NetworkMonitor.cpp
│   ├── RemoteWorkerApp.cpp
│   ├── ScreenCapture.cpp
│   └── UserActivity.cpp
└── libs/                 # External libraries (submodules)
    └── imgui/            # Dear ImGui library
```

## Implemented Features

✓ **Cross-platform GUI with ImGui**
- Login screen for user authentication
- Main monitoring dashboard with start/stop/pause controls
- Status display for user activity and network usage

✓ **User Authentication**
- Connects to remote MySQL database
- Validates user ID against stored credentials
- Secure credential handling

✓ **Screen Monitoring**
- Screenshot capture functionality (cross-platform)
- Random interval screenshot capture (10-30 minutes)
- Screen recording capabilities

✓ **User Activity Detection**
- Idle state detection (with configurable thresholds)
- Session duration tracking
- Activity status monitoring

✓ **Network Usage Monitoring**
- Real-time network usage statistics
- Data sent/received tracking
- Cross-platform implementation

✓ **Remote Data Handling**
- MySQL database integration
- File server upload capabilities
- Secure transmission of data

✓ **Main Application Flow**
1. Welcome/login screen
2. User ID validation against remote DB
3. Main monitoring interface with controls
4. Start/Stop/Pause functionality
5. Automatic screenshot uploads
6. Activity logging to remote DB

## Build Process
The application can be built using CMake on all supported platforms (Windows, Linux, macOS).

## Dependencies
- ImGui for GUI
- MySQL Connector for database operations
- Platform-specific APIs for screen capture and idle detection
- OpenGL for rendering