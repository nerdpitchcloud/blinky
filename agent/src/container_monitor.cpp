#include "collector.h"
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace blinky {
namespace agent {

ContainerMonitor::ContainerMonitor(metrics::SystemMetrics& metrics)
    : metrics_(metrics) {
}

static std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool ContainerMonitor::checkDocker() {
    struct stat buffer;
    return (stat("/var/run/docker.sock", &buffer) == 0);
}

bool ContainerMonitor::checkPodman() {
    std::string output = exec("which podman 2>/dev/null");
    return !output.empty();
}

void ContainerMonitor::collect() {
    if (checkDocker()) {
        std::string output = exec("docker ps --format '{{.ID}}|{{.Names}}|{{.State}}' 2>/dev/null");
        
        if (!output.empty()) {
            std::istringstream iss(output);
            std::string line;
            
            while (std::getline(iss, line)) {
                if (line.empty()) {
                    continue;
                }
                
                std::istringstream line_stream(line);
                std::string id, name, state;
                
                std::getline(line_stream, id, '|');
                std::getline(line_stream, name, '|');
                std::getline(line_stream, state, '|');
                
                metrics::ContainerMetrics container;
                container.id = id;
                container.name = name;
                container.runtime = "docker";
                container.state = state;
                
                std::string stats = exec(("docker stats --no-stream --format '{{.CPUPerc}}|{{.MemUsage}}' " + id + " 2>/dev/null").c_str());
                
                if (!stats.empty()) {
                    std::istringstream stats_stream(stats);
                    std::string cpu_str, mem_str;
                    
                    std::getline(stats_stream, cpu_str, '|');
                    std::getline(stats_stream, mem_str, '|');
                    
                    if (!cpu_str.empty() && cpu_str.back() == '%') {
                        cpu_str.pop_back();
                        container.cpu_percent = std::stod(cpu_str);
                    }
                    
                    std::istringstream mem_stream(mem_str);
                    std::string used, slash, limit;
                    mem_stream >> used >> slash >> limit;
                    
                    double used_val = std::stod(used);
                    double limit_val = std::stod(limit);
                    
                    if (mem_str.find("GiB") != std::string::npos) {
                        used_val *= 1024 * 1024 * 1024;
                        limit_val *= 1024 * 1024 * 1024;
                    } else if (mem_str.find("MiB") != std::string::npos) {
                        used_val *= 1024 * 1024;
                        limit_val *= 1024 * 1024;
                    }
                    
                    container.memory_bytes = static_cast<uint64_t>(used_val);
                    container.memory_limit = static_cast<uint64_t>(limit_val);
                }
                
                metrics_.containers.push_back(container);
            }
        }
    }
    
    if (checkPodman()) {
        std::string output = exec("podman ps --format '{{.ID}}|{{.Names}}|{{.State}}' 2>/dev/null");
        
        if (!output.empty()) {
            std::istringstream iss(output);
            std::string line;
            
            while (std::getline(iss, line)) {
                if (line.empty()) {
                    continue;
                }
                
                std::istringstream line_stream(line);
                std::string id, name, state;
                
                std::getline(line_stream, id, '|');
                std::getline(line_stream, name, '|');
                std::getline(line_stream, state, '|');
                
                metrics::ContainerMetrics container;
                container.id = id;
                container.name = name;
                container.runtime = "podman";
                container.state = state;
                container.cpu_percent = 0.0;
                container.memory_bytes = 0;
                container.memory_limit = 0;
                
                metrics_.containers.push_back(container);
            }
        }
    }
}

}
}
