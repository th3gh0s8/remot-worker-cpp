#pragma once

#include <string>

struct NetworkUsage {
    unsigned long long bytesSent;
    unsigned long long bytesReceived;
};

class NetworkMonitor {
public:
    NetworkMonitor();
    ~NetworkMonitor();
    
    NetworkUsage getNetworkUsage();
    
    // Get usage difference since last call
    NetworkUsage getNetworkUsageDiff();
    
private:
    NetworkUsage lastUsage;
    
    NetworkUsage getNetworkUsageWindows();
    NetworkUsage getNetworkUsageLinux();
    NetworkUsage getNetworkUsageMac();
};