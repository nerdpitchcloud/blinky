#include "collector.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <array>

namespace blinky {
namespace agent {

SmartMonitor::SmartMonitor(metrics::SystemMetrics& metrics)
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

void SmartMonitor::collect() {
    std::ifstream diskstats("/proc/diskstats");
    if (!diskstats.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(diskstats, line)) {
        std::istringstream iss(line);
        int major, minor;
        std::string device;
        
        iss >> major >> minor >> device;
        
        if (device.find("loop") != std::string::npos || 
            device.find("ram") != std::string::npos ||
            device.find("sr") != std::string::npos) {
            continue;
        }
        
        if (device.length() > 3 && std::isdigit(device.back())) {
            continue;
        }
        
        std::string cmd = "smartctl -H -A /dev/" + device + " 2>/dev/null";
        std::string output = exec(cmd.c_str());
        
        if (output.empty()) {
            continue;
        }
        
        metrics::SmartMetrics smart;
        smart.device = "/dev/" + device;
        smart.temperature = 0;
        smart.power_on_hours = 0;
        smart.reallocated_sectors = 0;
        smart.pending_sectors = 0;
        smart.health_status = "UNKNOWN";
        smart.passed = false;
        
        std::istringstream output_stream(output);
        std::string output_line;
        while (std::getline(output_stream, output_line)) {
            if (output_line.find("PASSED") != std::string::npos) {
                smart.health_status = "PASSED";
                smart.passed = true;
            } else if (output_line.find("FAILED") != std::string::npos) {
                smart.health_status = "FAILED";
                smart.passed = false;
            }
            
            if (output_line.find("Temperature_Celsius") != std::string::npos ||
                output_line.find("Airflow_Temperature") != std::string::npos) {
                std::istringstream line_stream(output_line);
                std::string token;
                int col = 0;
                while (line_stream >> token && col < 10) {
                    col++;
                    if (col == 10) {
                        smart.temperature = std::stoi(token);
                    }
                }
            } else if (output_line.find("Power_On_Hours") != std::string::npos) {
                std::istringstream line_stream(output_line);
                std::string token;
                int col = 0;
                while (line_stream >> token && col < 10) {
                    col++;
                    if (col == 10) {
                        smart.power_on_hours = std::stoull(token);
                    }
                }
            } else if (output_line.find("Reallocated_Sector") != std::string::npos) {
                std::istringstream line_stream(output_line);
                std::string token;
                int col = 0;
                while (line_stream >> token && col < 10) {
                    col++;
                    if (col == 10) {
                        smart.reallocated_sectors = std::stoull(token);
                    }
                }
            } else if (output_line.find("Current_Pending_Sector") != std::string::npos) {
                std::istringstream line_stream(output_line);
                std::string token;
                int col = 0;
                while (line_stream >> token && col < 10) {
                    col++;
                    if (col == 10) {
                        smart.pending_sectors = std::stoull(token);
                    }
                }
            }
        }
        
        if (smart.health_status != "UNKNOWN") {
            metrics_.smart_data.push_back(smart);
        }
    }
}

}
}
