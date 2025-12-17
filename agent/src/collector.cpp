#include "collector.h"
#include "system_info.h"
#include <unistd.h>
#include <cstring>
#include <ctime>

namespace blinky {
namespace agent {

MetricsCollector::MetricsCollector() {
}

MetricsCollector::~MetricsCollector() {
}

void MetricsCollector::initialize() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        current_metrics_.hostname = hostname;
    } else {
        current_metrics_.hostname = "unknown";
    }
    
    // Collect system info once at initialization
    current_metrics_.system_info = SystemInfoCollector::collect();
    
    monitors_.push_back(std::make_unique<CPUMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<MemoryMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<DiskMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<SmartMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<NetworkMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<SystemdMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<ContainerMonitor>(current_metrics_));
    monitors_.push_back(std::make_unique<KubernetesMonitor>(current_metrics_));
}

metrics::SystemMetrics MetricsCollector::collectAll() {
    current_metrics_.timestamp = static_cast<uint64_t>(std::time(nullptr));
    
    current_metrics_.disks.clear();
    current_metrics_.smart_data.clear();
    current_metrics_.network.clear();
    current_metrics_.systemd_services.clear();
    current_metrics_.containers.clear();
    
    for (auto& monitor : monitors_) {
        monitor->collect();
    }
    
    return current_metrics_;
}

}
}
