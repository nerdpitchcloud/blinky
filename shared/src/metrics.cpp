#include "metrics.h"
#include <sstream>
#include <iomanip>

namespace blinky {
namespace metrics {

std::string SystemMetrics::toJSON() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(2);
    
    json << "{";
    json << "\"timestamp\":" << timestamp << ",";
    json << "\"hostname\":\"" << hostname << "\",";
    json << "\"uptime\":" << uptime_seconds << ",";
    
    json << "\"system_info\":{";
    json << "\"hostname\":\"" << system_info.hostname << "\",";
    json << "\"os_name\":\"" << system_info.os_name << "\",";
    json << "\"os_version\":\"" << system_info.os_version << "\",";
    json << "\"kernel\":\"" << system_info.kernel_version << "\",";
    json << "\"architecture\":\"" << system_info.architecture << "\",";
    json << "\"cpu_model\":\"" << system_info.cpu_model << "\",";
    json << "\"cpu_cores\":" << system_info.cpu_cores << ",";
    json << "\"cpu_threads\":" << system_info.cpu_threads << ",";
    json << "\"total_memory\":" << system_info.total_memory_bytes;
    json << "},";
    
    json << "\"cpu\":{";
    json << "\"usage\":" << cpu.usage_percent << ",";
    json << "\"load_1\":" << cpu.load_1min << ",";
    json << "\"load_5\":" << cpu.load_5min << ",";
    json << "\"load_15\":" << cpu.load_15min << ",";
    json << "\"cores\":" << cpu.core_count;
    json << "},";
    
    json << "\"memory\":{";
    json << "\"total\":" << memory.total_bytes << ",";
    json << "\"used\":" << memory.used_bytes << ",";
    json << "\"available\":" << memory.available_bytes << ",";
    json << "\"cached\":" << memory.cached_bytes << ",";
    json << "\"usage\":" << memory.usage_percent;
    json << "},";
    
    json << "\"disks\":[";
    for (size_t i = 0; i < disks.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"device\":\"" << disks[i].device << "\",";
        json << "\"mount\":\"" << disks[i].mount_point << "\",";
        json << "\"total\":" << disks[i].total_bytes << ",";
        json << "\"used\":" << disks[i].used_bytes << ",";
        json << "\"available\":" << disks[i].available_bytes << ",";
        json << "\"usage\":" << disks[i].usage_percent << ",";
        json << "\"read_bytes\":" << disks[i].read_bytes << ",";
        json << "\"write_bytes\":" << disks[i].write_bytes << ",";
        json << "\"read_ops\":" << disks[i].read_ops << ",";
        json << "\"write_ops\":" << disks[i].write_ops << ",";
        json << "\"read_bytes_per_sec\":" << disks[i].read_bytes_per_sec << ",";
        json << "\"write_bytes_per_sec\":" << disks[i].write_bytes_per_sec << ",";
        json << "\"read_ops_per_sec\":" << disks[i].read_ops_per_sec << ",";
        json << "\"write_ops_per_sec\":" << disks[i].write_ops_per_sec;
        json << "}";
    }
    json << "],";
    
    json << "\"smart\":[";
    for (size_t i = 0; i < smart_data.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"device\":\"" << smart_data[i].device << "\",";
        json << "\"temperature\":" << smart_data[i].temperature << ",";
        json << "\"power_on_hours\":" << smart_data[i].power_on_hours << ",";
        json << "\"reallocated_sectors\":" << smart_data[i].reallocated_sectors << ",";
        json << "\"pending_sectors\":" << smart_data[i].pending_sectors << ",";
        json << "\"health\":\"" << smart_data[i].health_status << "\",";
        json << "\"passed\":" << (smart_data[i].passed ? "true" : "false");
        json << "}";
    }
    json << "],";
    
    json << "\"network\":[";
    for (size_t i = 0; i < network.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"interface\":\"" << network[i].interface << "\",";
        json << "\"rx_bytes\":" << network[i].rx_bytes << ",";
        json << "\"tx_bytes\":" << network[i].tx_bytes << ",";
        json << "\"rx_packets\":" << network[i].rx_packets << ",";
        json << "\"tx_packets\":" << network[i].tx_packets << ",";
        json << "\"rx_errors\":" << network[i].rx_errors << ",";
        json << "\"tx_errors\":" << network[i].tx_errors << ",";
        json << "\"rx_bytes_per_sec\":" << network[i].rx_bytes_per_sec << ",";
        json << "\"tx_bytes_per_sec\":" << network[i].tx_bytes_per_sec << ",";
        json << "\"rx_packets_per_sec\":" << network[i].rx_packets_per_sec << ",";
        json << "\"tx_packets_per_sec\":" << network[i].tx_packets_per_sec;
        json << "}";
    }
    json << "],";
    
    json << "\"systemd\":[";
    for (size_t i = 0; i < systemd_services.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"name\":\"" << systemd_services[i].name << "\",";
        json << "\"state\":\"" << systemd_services[i].state << "\",";
        json << "\"sub_state\":\"" << systemd_services[i].sub_state << "\",";
        json << "\"active\":" << (systemd_services[i].active ? "true" : "false") << ",";
        json << "\"enabled\":" << (systemd_services[i].enabled ? "true" : "false");
        json << "}";
    }
    json << "],";
    
    json << "\"containers\":[";
    for (size_t i = 0; i < containers.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"id\":\"" << containers[i].id << "\",";
        json << "\"name\":\"" << containers[i].name << "\",";
        json << "\"runtime\":\"" << containers[i].runtime << "\",";
        json << "\"state\":\"" << containers[i].state << "\",";
        json << "\"image\":\"" << containers[i].image << "\",";
        json << "\"cpu_percent\":" << containers[i].cpu_percent << ",";
        json << "\"memory_bytes\":" << containers[i].memory_bytes << ",";
        json << "\"memory_limit\":" << containers[i].memory_limit << ",";
        json << "\"memory_percent\":" << containers[i].memory_percent << ",";
        json << "\"memory_cache\":" << containers[i].memory_cache << ",";
        json << "\"network_rx_bytes\":" << containers[i].network_rx_bytes << ",";
        json << "\"network_tx_bytes\":" << containers[i].network_tx_bytes << ",";
        json << "\"network_rx_packets\":" << containers[i].network_rx_packets << ",";
        json << "\"network_tx_packets\":" << containers[i].network_tx_packets << ",";
        json << "\"network_rx_errors\":" << containers[i].network_rx_errors << ",";
        json << "\"network_tx_errors\":" << containers[i].network_tx_errors << ",";
        json << "\"network_rx_bytes_per_sec\":" << containers[i].network_rx_bytes_per_sec << ",";
        json << "\"network_tx_bytes_per_sec\":" << containers[i].network_tx_bytes_per_sec << ",";
        json << "\"block_read_bytes\":" << containers[i].block_read_bytes << ",";
        json << "\"block_write_bytes\":" << containers[i].block_write_bytes << ",";
        json << "\"block_read_bytes_per_sec\":" << containers[i].block_read_bytes_per_sec << ",";
        json << "\"block_write_bytes_per_sec\":" << containers[i].block_write_bytes_per_sec << ",";
        json << "\"pids\":" << containers[i].pids;
        json << "}";
    }
    json << "],";
    
    json << "\"kubernetes\":{";
    json << "\"type\":\"" << kubernetes.cluster_type << "\",";
    json << "\"detected\":" << (kubernetes.detected ? "true" : "false") << ",";
    json << "\"pods\":" << kubernetes.pod_count << ",";
    json << "\"nodes\":" << kubernetes.node_count << ",";
    json << "\"namespaces\":[";
    for (size_t i = 0; i < kubernetes.namespaces.size(); ++i) {
        if (i > 0) json << ",";
        json << "\"" << kubernetes.namespaces[i] << "\"";
    }
    json << "]";
    json << "},";
    
    json << "\"temperatures\":[";
    for (size_t i = 0; i < temperatures.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"sensor\":\"" << temperatures[i].sensor_name << "\",";
        json << "\"type\":\"" << temperatures[i].sensor_type << "\",";
        json << "\"label\":\"" << temperatures[i].label << "\",";
        json << "\"temp\":" << temperatures[i].temperature;
        if (temperatures[i].max > 0) {
            json << ",\"max\":" << temperatures[i].max;
        }
        if (temperatures[i].critical > 0) {
            json << ",\"critical\":" << temperatures[i].critical;
        }
        json << "}";
    }
    json << "]";
    
    json << "}";
    
    return json.str();
}

SystemMetrics SystemMetrics::fromJSON(const std::string& json) {
    SystemMetrics metrics;
    // TODO: Implement JSON parsing when needed
    return metrics;
}

}
}
