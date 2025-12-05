#pragma once

#include <string>

class FileUploader {
public:
    FileUploader();
    ~FileUploader();

    bool uploadFile(const std::string& localFilePath, const std::string& remotePath);

    bool setServerCredentials(const std::string& server, const std::string& username,
                              const std::string& password, int port = 21);

private:
    std::string server;
    std::string username;
    std::string password;
    int port;

    bool uploadToLocalHtdocs(const std::string& localFilePath, const std::string& remotePath);
    bool uploadViaFTP(const std::string& localFilePath, const std::string& remotePath);
    bool uploadViaHTTP(const std::string& localFilePath, const std::string& remotePath);
};