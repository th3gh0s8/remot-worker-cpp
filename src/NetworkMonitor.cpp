#include "NetworkMonitor.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <iprtrmib.h>
#pragma comment(lib, "iphlpapi.lib")
#elif __linux__
#include <sys/socket.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#elif __APPLE__
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#endif

#include <iostream>

NetworkMonitor::NetworkMonitor() {
    lastUsage = getNetworkUsage();
}

NetworkMonitor::~NetworkMonitor() = default;

NetworkUsage NetworkMonitor::getNetworkUsage() {
#ifdef _WIN32
    return getNetworkUsageWindows();
#elif __linux__
    return getNetworkUsageLinux();
#elif __APPLE__
    return getNetworkUsageMac();
#else
    // Fallback for unsupported platforms
    return {0, 0};
#endif
}

NetworkUsage NetworkMonitor::getNetworkUsageDiff() {
    NetworkUsage current = getNetworkUsage();
    NetworkUsage diff = {
        current.bytesSent - lastUsage.bytesSent,
        current.bytesReceived - lastUsage.bytesReceived
    };
    
    lastUsage = current;
    return diff;
}

#ifdef _WIN32
NetworkUsage NetworkMonitor::getNetworkUsageWindows() {
    // Use GetIfTable API instead of GetIpStatistics to get interface statistics
    // This is simpler and more reliable
    return {0, 0};  // Placeholder - actual implementation would query specific network interface
}
#endif

#ifdef __linux__
NetworkUsage NetworkMonitor::getNetworkUsageLinux() {
    // Placeholder for Linux implementation
    // A real implementation would parse /proc/net/dev or similar
    return {0, 0};
}
#endif

#ifdef __APPLE__
NetworkUsage NetworkMonitor::getNetworkUsageMac() {
    // Placeholder for macOS implementation
    // A real implementation would use system calls to get network stats
    return {0, 0};
}
#endif