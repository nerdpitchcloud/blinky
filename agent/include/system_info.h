#pragma once

#include "metrics.h"
#include <fstream>
#include <sstream>
#include <sys/utsname.h>

namespace blinky {
namespace agent {

class SystemInfoCollector {
public:
    static metrics::SystemInfo collect() {
        metrics::SystemInfo info;
        
        struct utsname uts;
        if (uname(&uts) == 0) {
            info.hostname = uts.nodename;
            info.kernel_version = uts.release;
            info.architecture = uts.machine;
        }
        
        info.os_name = getOSName();
        info.os_version = getOSVersion();
        info.cpu_model = getCPUModel();
        info.cpu_cores = getCPUCores();
        info.cpu_threads = getCPUThreads();
        info.total_memory_bytes = getTotalMemory();
        
        return info;
    }

private:
    static std::string getOSName() {
        std::ifstream file("/etc/os-release");
        if (!file.is_open()) return "Linux";
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("PRETTY_NAME=") == 0) {
                size_t start = line.find('"');
                size_t end = line.rfind('"');
                if (start != std::string::npos && end != std::string::npos && start < end) {
                    return line.substr(start + 1, end - start - 1);
                }
            }
        }
        return "Linux";
    }
    
    static std::string getOSVersion() {
        std::ifstream file("/etc/os-release");
        if (!file.is_open()) return "";
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("VERSION_ID=") == 0) {
                size_t start = line.find('"');
                size_t end = line.rfind('"');
                if (start != std::string::npos && end != std::string::npos && start < end) {
                    return line.substr(start + 1, end - start - 1);
                }
                // No quotes, get value after =
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    return line.substr(eq + 1);
                }
            }
        }
        return "";
    }
    
    static std::string getCPUModel() {
        std::ifstream file("/proc/cpuinfo");
        if (!file.is_open()) return "Unknown";
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("model name") == 0) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string model = line.substr(colon + 1);
                    // Trim leading whitespace
                    size_t start = model.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        return model.substr(start);
                    }
                }
            }
        }
        return "Unknown";
    }
    
    static uint32_t getCPUCores() {
        std::ifstream file("/proc/cpuinfo");
        if (!file.is_open()) return 0;
        
        uint32_t cores = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("cpu cores") == 0) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    try {
                        cores = std::stoul(line.substr(colon + 1));
                        break;
                    } catch (...) {}
                }
            }
        }
        
        // If cpu cores not found, count physical processors
        if (cores == 0) {
            file.clear();
            file.seekg(0);
            while (std::getline(file, line)) {
                if (line.find("processor") == 0) {
                    cores++;
                }
            }
        }
        
        return cores;
    }
    
    static uint32_t getCPUThreads() {
        std::ifstream file("/proc/cpuinfo");
        if (!file.is_open()) return 0;
        
        uint32_t threads = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("processor") == 0) {
                threads++;
            }
        }
        return threads;
    }
    
    static uint64_t getTotalMemory() {
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()) return 0;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("MemTotal:") == 0) {
                std::istringstream iss(line);
                std::string label;
                uint64_t value;
                iss >> label >> value;
                return value * 1024; // Convert KB to bytes
            }
        }
        return 0;
    }
};

}
}
