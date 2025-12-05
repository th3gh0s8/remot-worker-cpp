#include "FileUploader.h"

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

FileUploader::FileUploader() : port(80) {} // Default to HTTP port

FileUploader::~FileUploader() = default;

bool FileUploader::uploadFile(const std::string& localFilePath, const std::string& remotePath) {
    // Determine if this is a local upload (to htdocs) or remote upload
    if (server == "localhost" || server == "127.0.0.1") {
        return uploadToLocalHtdocs(localFilePath, remotePath);
    } else {
        // Try different protocols based on configuration for remote uploads
        if (port == 21 || port == 22) {
            return uploadViaFTP(localFilePath, remotePath);
        } else {
            return uploadViaHTTP(localFilePath, remotePath);
        }
    }
}

bool FileUploader::setServerCredentials(const std::string& server, const std::string& username,
                                        const std::string& password, int port) {
    this->server = server;
    this->username = username;
    this->password = password;
    this->port = port;
    return true;
}

bool FileUploader::uploadToLocalHtdocs(const std::string& localFilePath, const std::string& remotePath) {
    try {
        // Build the destination path in htdocs
        // For XAMPP, this is typically C:\xampp\htdocs\ unless configured otherwise
#ifdef _WIN32
        std::string htdocsBase = "C:\\xampp\\htdocs\\";
#else
        std::string htdocsBase = "/opt/lampp/htdocs/"; // Default Linux XAMPP location
#endif

        // Create the full destination path
        std::string destinationPath = htdocsBase + remotePath;

        // Create directory if it doesn't exist
        std::filesystem::create_directories(destinationPath);

        // Extract the filename from the local path
        std::filesystem::path localPath(localFilePath);
        std::string filename = localPath.filename().string();

        // Create the full destination file path
        std::string destinationFile = destinationPath + filename;

        // Copy the file from local to htdocs
        std::filesystem::copy_file(localFilePath, destinationFile,
                                  std::filesystem::copy_options::overwrite_existing);

        std::cout << "Successfully copied " << localFilePath << " to " << destinationFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error uploading file to htdocs: " << e.what() << std::endl;
        return false;
    }
}

bool FileUploader::uploadViaFTP(const std::string& localFilePath, const std::string& remotePath) {
    std::cout << "Uploading " << localFilePath << " to FTP server " << server
              << " at path " << remotePath << std::endl;

    // In a real implementation, you would use an FTP library like libcurl
    return true; // Simulate successful upload
}

bool FileUploader::uploadViaHTTP(const std::string& localFilePath, const std::string& remotePath) {
    std::cout << "Uploading " << localFilePath << " to HTTP server " << server
              << " at path " << remotePath << std::endl;

    // In a real implementation, you would use an HTTP library like libcurl
    return true; // Simulate successful upload
}