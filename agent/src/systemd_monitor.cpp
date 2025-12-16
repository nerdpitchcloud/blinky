#include "collector.h"
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>

namespace blinky {
namespace agent {

SystemdMonitor::SystemdMonitor(metrics::SystemMetrics& metrics)
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

void SystemdMonitor::collect() {
    std::string output = exec("systemctl list-units --type=service --all --no-pager --no-legend 2>/dev/null");
    
    if (output.empty()) {
        return;
    }
    
    std::istringstream iss(output);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.empty()) {
            continue;
        }
        
        std::istringstream line_stream(line);
        std::string unit_name, load, active, sub;
        
        line_stream >> unit_name >> load >> active >> sub;
        
        if (unit_name.empty()) {
            continue;
        }
        
        metrics::SystemdServiceMetrics service;
        service.name = unit_name;
        service.state = active;
        service.sub_state = sub;
        service.active = (active == "active");
        
        std::string is_enabled = exec(("systemctl is-enabled " + unit_name + " 2>/dev/null").c_str());
        service.enabled = (is_enabled.find("enabled") != std::string::npos);
        
        metrics_.systemd_services.push_back(service);
    }
}

}
}
