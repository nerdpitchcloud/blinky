#include "collector.h"
#include <fstream>
#include <sstream>
#include <string>

namespace blinky {
namespace agent {

NetworkMonitor::NetworkMonitor(metrics::SystemMetrics& metrics)
    : metrics_(metrics) {
}

void NetworkMonitor::collect() {
    std::ifstream net_dev("/proc/net/dev");
    if (!net_dev.is_open()) {
        return;
    }
    
    std::string line;
    std::getline(net_dev, line);
    std::getline(net_dev, line);
    
    while (std::getline(net_dev, line)) {
        std::istringstream iss(line);
        std::string interface;
        
        iss >> interface;
        
        if (interface.empty()) {
            continue;
        }
        
        if (interface.back() == ':') {
            interface.pop_back();
        }
        
        if (interface == "lo") {
            continue;
        }
        
        metrics::NetworkMetrics net;
        net.interface = interface;
        
        iss >> net.rx_bytes >> net.rx_packets;
        
        uint64_t rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
        iss >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> rx_compressed >> rx_multicast;
        net.rx_errors = rx_errs;
        
        iss >> net.tx_bytes >> net.tx_packets;
        
        uint64_t tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
        iss >> tx_errs >> tx_drop >> tx_fifo >> tx_colls >> tx_carrier >> tx_compressed;
        net.tx_errors = tx_errs;
        
        metrics_.network.push_back(net);
    }
}

}
}
