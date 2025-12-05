@echo off
REM Build script for Remote Worker Monitoring Application

echo Installing dependencies via vcpkg (if not already installed)...
echo Make sure you have vcpkg installed or GLFW/OpenGL libraries available

REM Create build directory if it doesn't exist
if not exist "build" mkdir build

REM Navigate to build directory
cd build

REM Configure the project with CMake for Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the project
if errorlevel 1 (
    echo CMake configuration failed
    echo Please make sure you have Visual Studio with C++ development tools installed
    pause
    exit /b 1
)

cmake --build . --config Release

if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

echo Build completed successfully!
echo You can find the executable in build\Release\RemoteWorkerCpp.exe
echo.
echo Before running, make sure to update your database credentials in:
echo - src/LoginScreen.cpp
echo - src/MonitoringScreen.cpp
echo.
pause