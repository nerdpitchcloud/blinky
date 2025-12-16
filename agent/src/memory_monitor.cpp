#include "collector.h"
#include <fstream>
#include <sstream>
#include <string>

namespace blinky {
namespace agent {

MemoryMonitor::MemoryMonitor(metrics::SystemMetrics& metrics)
    : metrics_(metrics) {
}

void MemoryMonitor::collect() {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return;
    }
    
    uint64_t mem_total = 0, mem_free = 0, mem_available = 0;
    uint64_t buffers = 0, cached = 0, slab = 0;
    
    std::string line;
    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        std::string unit;
        
        iss >> key >> value >> unit;
        
        value *= 1024;
        
        if (key == "MemTotal:") {
            mem_total = value;
        } else if (key == "MemFree:") {
            mem_free = value;
        } else if (key == "MemAvailable:") {
            mem_available = value;
        } else if (key == "Buffers:") {
            buffers = value;
        } else if (key == "Cached:") {
            cached = value;
        } else if (key == "Slab:") {
            slab = value;
        }
    }
    
    metrics_.memory.total_bytes = mem_total;
    metrics_.memory.available_bytes = mem_available;
    metrics_.memory.used_bytes = mem_total - mem_available;
    metrics_.memory.cached_bytes = cached + buffers + slab;
    
    if (mem_total > 0) {
        metrics_.memory.usage_percent = 100.0 * metrics_.memory.used_bytes / mem_total;
    }
}

}
}
