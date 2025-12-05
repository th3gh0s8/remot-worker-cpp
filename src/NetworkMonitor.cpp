#include "NetworkMonitor.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <ipifcons.h>  // Required for MIB_IFTABLE
#pragma comment(lib, "iphlpapi.lib")
#elif __linux__
#include <sys/socket.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <fstream>
#include <sstream>
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
    // Use GetIfTable to get network interface statistics
    DWORD dwSize = 0;
    MIB_IFTABLE *pIfTable = NULL;
    DWORD dwRetVal = 0;

    // First call to get the required buffer size
    dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = (MIB_IFTABLE *) malloc(dwSize);
        if (pIfTable == NULL) {
            std::cerr << "Memory allocation failed for GetIfTable" << std::endl;
            return {0, 0};
        }
    }

    // Second call to actually get the interface table
    dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE);
    if (dwRetVal != NO_ERROR) {
        std::cerr << "GetIfTable failed with error: " << dwRetVal << std::endl;
        if (pIfTable) {
            free(pIfTable);
        }
        return {0, 0};
    }

    // Sum up all interface statistics (excluding loopback)
    unsigned long long totalBytesSent = 0;
    unsigned long long totalBytesReceived = 0;

    for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
        MIB_IFROW *pIfRow = &pIfTable->table[i];

        // Skip loopback interface (typically interface 1)
        if (pIfRow->dwIndex != 1) {
            totalBytesReceived += pIfRow->dwInOctets;
            totalBytesSent += pIfRow->dwOutOctets;
        }
    }

    if (pIfTable) {
        free(pIfTable);
    }

    return {totalBytesSent, totalBytesReceived};
}
#endif

#ifdef __linux__
NetworkUsage NetworkMonitor::getNetworkUsageLinux() {
    // Parse /proc/net/dev to get network statistics
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        return {0, 0};
    }

    std::string line;
    unsigned long long totalBytesSent = 0;
    unsigned long long totalBytesReceived = 0;

    // Skip header lines
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string interface_name;
        iss >> interface_name;

        // Remove the colon after interface name
        if (interface_name.back() == ':') {
            interface_name.pop_back();
        }

        // Skip loopback interface
        if (interface_name == "lo") {
            continue;
        }

        unsigned long long rx_bytes, tx_bytes;
        // Skip interface name, then read rx bytes
        for (int i = 0; i < 1; i++) {
            std::string temp;
            iss >> temp;
        }
        iss >> rx_bytes;

        // Skip additional rx fields (packets, errs, etc.)
        for (int i = 0; i < 6; i++) {
            std::string temp;
            iss >> temp;
        }

        // Read tx bytes
        iss >> tx_bytes;

        totalBytesReceived += rx_bytes;
        totalBytesSent += tx_bytes;
    }

    return {totalBytesSent, totalBytesReceived};
}
#endif

#ifdef __APPLE__
NetworkUsage NetworkMonitor::getNetworkUsageMac() {
    // For macOS, we would need to use system calls to get network statistics
    // This is a simplified implementation that just returns 0,
    // A real implementation would use getifaddrs or similar
    return {0, 0};
}
#endif