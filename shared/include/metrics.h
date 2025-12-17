#ifndef BLINKY_METRICS_H
#define BLINKY_METRICS_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace blinky {
namespace metrics {

struct SystemInfo {
    std::string hostname;
    std::string os_name;
    std::string os_version;
    std::string kernel_version;
    std::string architecture;
    std::string cpu_model;
    uint32_t cpu_cores = 0;
    uint32_t cpu_threads = 0;
    uint64_t total_memory_bytes = 0;
};

struct CPUMetrics {
    double usage_percent;
    double load_1min;
    double load_5min;
    double load_15min;
    uint32_t core_count;
};

struct MemoryMetrics {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t available_bytes;
    uint64_t cached_bytes;
    double usage_percent;
};

struct DiskMetrics {
    std::string device;
    std::string mount_point;
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t available_bytes;
    double usage_percent;
};

struct SmartMetrics {
    std::string device;
    int temperature;
    uint64_t power_on_hours;
    uint64_t reallocated_sectors;
    uint64_t pending_sectors;
    std::string health_status;
    bool passed;
};

struct NetworkMetrics {
    std::string interface;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_errors;
    uint64_t tx_errors;
};

struct SystemdServiceMetrics {
    std::string name;
    std::string state;
    std::string sub_state;
    bool active;
    bool enabled;
};

struct ContainerMetrics {
    std::string id;
    std::string name;
    std::string runtime;
    std::string state;
    double cpu_percent;
    uint64_t memory_bytes;
    uint64_t memory_limit;
};

struct KubernetesMetrics {
    std::string cluster_type;
    bool detected;
    int pod_count;
    int node_count;
    std::vector<std::string> namespaces;
};

struct SystemMetrics {
    uint64_t timestamp;
    std::string hostname;
    uint64_t uptime_seconds;
    
    SystemInfo system_info;
    CPUMetrics cpu;
    MemoryMetrics memory;
    std::vector<DiskMetrics> disks;
    std::vector<SmartMetrics> smart_data;
    std::vector<NetworkMetrics> network;
    std::vector<SystemdServiceMetrics> systemd_services;
    std::vector<ContainerMetrics> containers;
    KubernetesMetrics kubernetes;
    
    std::string toJSON() const;
    static SystemMetrics fromJSON(const std::string& json);
};

}
}

#endif
