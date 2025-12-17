#pragma once

#include "collector.h"
#include "metrics.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>

namespace blinky {
namespace agent {

class TemperatureMonitor : public Monitor {
public:
    TemperatureMonitor(metrics::SystemMetrics& metrics) : metrics_(metrics) {}

    void collect() override {
        metrics_.temperatures.clear();
        
        // Collect from /sys/class/thermal
        collectThermalZones();
        
        // Collect from /sys/class/hwmon
        collectHwmon();
        
        // Collect NVMe temperatures
        collectNVMe();
    }

private:
    metrics::SystemMetrics& metrics_;

    void collectThermalZones() {
        std::filesystem::path thermal_path = "/sys/class/thermal";
        
        if (!std::filesystem::exists(thermal_path)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(thermal_path)) {
            if (entry.path().filename().string().find("thermal_zone") == 0) {
                std::string zone_name = entry.path().filename().string();
                
                // Read temperature
                std::ifstream temp_file(entry.path() / "temp");
                if (temp_file.is_open()) {
                    int temp_millidegrees;
                    temp_file >> temp_millidegrees;
                    double temp_celsius = temp_millidegrees / 1000.0;
                    
                    // Read type/name
                    std::string type_name = zone_name;
                    std::ifstream type_file(entry.path() / "type");
                    if (type_file.is_open()) {
                        std::getline(type_file, type_name);
                    }
                    
                    metrics::TemperatureMetrics temp_metric;
                    temp_metric.sensor_name = type_name;
                    temp_metric.sensor_type = "thermal_zone";
                    temp_metric.temperature = temp_celsius;
                    temp_metric.label = zone_name;
                    
                    // Try to read critical temperature
                    std::ifstream crit_file(entry.path() / "trip_point_0_temp");
                    if (crit_file.is_open()) {
                        int crit_millidegrees;
                        crit_file >> crit_millidegrees;
                        temp_metric.critical = crit_millidegrees / 1000.0;
                    }
                    
                    metrics_.temperatures.push_back(temp_metric);
                }
            }
        }
    }

    void collectHwmon() {
        std::filesystem::path hwmon_path = "/sys/class/hwmon";
        
        if (!std::filesystem::exists(hwmon_path)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(hwmon_path)) {
            std::string hwmon_name = entry.path().filename().string();
            
            // Read device name
            std::string device_name = hwmon_name;
            std::ifstream name_file(entry.path() / "name");
            if (name_file.is_open()) {
                std::getline(name_file, device_name);
            }
            
            // Find all temp*_input files
            for (const auto& temp_entry : std::filesystem::directory_iterator(entry.path())) {
                std::string filename = temp_entry.path().filename().string();
                
                if (filename.find("temp") == 0 && filename.find("_input") != std::string::npos) {
                    // Extract temp number (e.g., temp1_input -> 1)
                    std::string temp_num = filename.substr(4, filename.find("_") - 4);
                    
                    // Read temperature
                    std::ifstream temp_file(temp_entry.path());
                    if (temp_file.is_open()) {
                        int temp_millidegrees;
                        temp_file >> temp_millidegrees;
                        double temp_celsius = temp_millidegrees / 1000.0;
                        
                        // Read label if available
                        std::string label = "temp" + temp_num;
                        std::ifstream label_file(entry.path() / ("temp" + temp_num + "_label"));
                        if (label_file.is_open()) {
                            std::getline(label_file, label);
                        }
                        
                        metrics::TemperatureMetrics temp_metric;
                        temp_metric.sensor_name = device_name;
                        temp_metric.sensor_type = "hwmon";
                        temp_metric.temperature = temp_celsius;
                        temp_metric.label = label;
                        
                        // Try to read critical temperature
                        std::ifstream crit_file(entry.path() / ("temp" + temp_num + "_crit"));
                        if (crit_file.is_open()) {
                            int crit_millidegrees;
                            crit_file >> crit_millidegrees;
                            temp_metric.critical = crit_millidegrees / 1000.0;
                        }
                        
                        // Try to read max temperature
                        std::ifstream max_file(entry.path() / ("temp" + temp_num + "_max"));
                        if (max_file.is_open()) {
                            int max_millidegrees;
                            max_file >> max_millidegrees;
                            temp_metric.max = max_millidegrees / 1000.0;
                        }
                        
                        metrics_.temperatures.push_back(temp_metric);
                    }
                }
            }
        }
    }

    void collectNVMe() {
        // NVMe temperatures are often in hwmon, but we can also check nvme-cli output
        // For now, hwmon should catch most NVMe devices
        // We could add smartctl parsing here for more detailed NVMe info
        
        std::filesystem::path nvme_path = "/sys/class/nvme";
        
        if (!std::filesystem::exists(nvme_path)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(nvme_path)) {
            std::string nvme_name = entry.path().filename().string();
            
            // Check if there's a hwmon subdirectory
            std::filesystem::path hwmon_dir = entry.path() / "device" / "hwmon";
            if (std::filesystem::exists(hwmon_dir)) {
                for (const auto& hwmon_entry : std::filesystem::directory_iterator(hwmon_dir)) {
                    // Read temperature from hwmon
                    std::ifstream temp_file(hwmon_entry.path() / "temp1_input");
                    if (temp_file.is_open()) {
                        int temp_millidegrees;
                        temp_file >> temp_millidegrees;
                        double temp_celsius = temp_millidegrees / 1000.0;
                        
                        metrics::TemperatureMetrics temp_metric;
                        temp_metric.sensor_name = nvme_name;
                        temp_metric.sensor_type = "nvme";
                        temp_metric.temperature = temp_celsius;
                        temp_metric.label = "Composite";
                        
                        // NVMe critical temps
                        std::ifstream crit_file(hwmon_entry.path() / "temp1_crit");
                        if (crit_file.is_open()) {
                            int crit_millidegrees;
                            crit_file >> crit_millidegrees;
                            temp_metric.critical = crit_millidegrees / 1000.0;
                        }
                        
                        std::ifstream max_file(hwmon_entry.path() / "temp1_max");
                        if (max_file.is_open()) {
                            int max_millidegrees;
                            max_file >> max_millidegrees;
                            temp_metric.max = max_millidegrees / 1000.0;
                        }
                        
                        metrics_.temperatures.push_back(temp_metric);
                    }
                }
            }
        }
    }
};

}
}
