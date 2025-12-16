#include "collector.h"
#include <fstream>
#include <sstream>
#include <thread>

namespace blinky {
namespace agent {

CPUMonitor::CPUMonitor(metrics::SystemMetrics& metrics)
    : metrics_(metrics), prev_total_(0), prev_idle_(0) {
}

void CPUMonitor::collect() {
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        return;
    }
    
    std::string line;
    std::getline(stat_file, line);
    
    std::istringstream iss(line);
    std::string cpu_label;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    
    iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
    uint64_t idle_time = idle + iowait;
    
    if (prev_total_ > 0) {
        uint64_t total_diff = total - prev_total_;
        uint64_t idle_diff = idle_time - prev_idle_;
        
        if (total_diff > 0) {
            metrics_.cpu.usage_percent = 100.0 * (total_diff - idle_diff) / total_diff;
        }
    }
    
    prev_total_ = total;
    prev_idle_ = idle_time;
    
    std::ifstream loadavg_file("/proc/loadavg");
    if (loadavg_file.is_open()) {
        loadavg_file >> metrics_.cpu.load_1min 
                     >> metrics_.cpu.load_5min 
                     >> metrics_.cpu.load_15min;
    }
    
    metrics_.cpu.core_count = std::thread::hardware_concurrency();
    
    std::ifstream uptime_file("/proc/uptime");
    if (uptime_file.is_open()) {
        double uptime;
        uptime_file >> uptime;
        metrics_.uptime_seconds = static_cast<uint64_t>(uptime);
    }
}

}
}
