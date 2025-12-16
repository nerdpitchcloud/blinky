#include "collector.h"
#include <sys/statvfs.h>
#include <fstream>
#include <sstream>
#include <string>

namespace blinky {
namespace agent {

DiskMonitor::DiskMonitor(metrics::SystemMetrics& metrics)
    : metrics_(metrics) {
}

void DiskMonitor::collect() {
    std::ifstream mounts("/proc/mounts");
    if (!mounts.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(mounts, line)) {
        std::istringstream iss(line);
        std::string device, mount_point, fs_type;
        
        iss >> device >> mount_point >> fs_type;
        
        if (device[0] != '/' || fs_type == "tmpfs" || fs_type == "devtmpfs" || 
            fs_type == "sysfs" || fs_type == "proc" || fs_type == "devpts" ||
            fs_type == "cgroup" || fs_type == "cgroup2" || fs_type == "overlay") {
            continue;
        }
        
        struct statvfs stat;
        if (statvfs(mount_point.c_str(), &stat) != 0) {
            continue;
        }
        
        metrics::DiskMetrics disk;
        disk.device = device;
        disk.mount_point = mount_point;
        disk.total_bytes = stat.f_blocks * stat.f_frsize;
        disk.available_bytes = stat.f_bavail * stat.f_frsize;
        disk.used_bytes = disk.total_bytes - (stat.f_bfree * stat.f_frsize);
        
        if (disk.total_bytes > 0) {
            disk.usage_percent = 100.0 * disk.used_bytes / disk.total_bytes;
        }
        
        metrics_.disks.push_back(disk);
    }
}

}
}
