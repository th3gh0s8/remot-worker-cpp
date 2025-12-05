#pragma once

#include <string>
#include <memory>

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    bool connect(const std::string& host, const std::string& user, 
                 const std::string& password, const std::string& database, int port = 3306);
    
    bool validateUser(const std::string& userId);
    
    bool insertActivityData(const std::string& userId, const std::string& activityData);
    
    bool insertNetworkUsage(const std::string& userId, long bytesSent, long bytesReceived);
    
    void disconnect();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};