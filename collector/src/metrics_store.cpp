#include "metrics_store.h"
#include <ctime>
#include <algorithm>

namespace blinky {
namespace collector {

MetricsStore::MetricsStore() {
}

MetricsStore::~MetricsStore() {
}

void MetricsStore::storeMetrics(const metrics::SystemMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& host = hosts_[metrics.hostname];
    host.hostname = metrics.hostname;
    host.latest = metrics;
    host.last_update = metrics.timestamp;
    host.online = true;
    
    host.history.push_back(metrics);
    
    if (host.history.size() > HostMetricsHistory::MAX_HISTORY) {
        host.history.pop_front();
    }
}

HostMetricsHistory MetricsStore::getHostMetrics(const std::string& hostname) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = hosts_.find(hostname);
    if (it != hosts_.end()) {
        return it->second;
    }
    
    return HostMetricsHistory();
}

std::vector<std::string> MetricsStore::getHostnames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> hostnames;
    for (const auto& pair : hosts_) {
        hostnames.push_back(pair.first);
    }
    
    return hostnames;
}

std::map<std::string, HostMetricsHistory> MetricsStore::getAllHosts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hosts_;
}

void MetricsStore::markHostOffline(const std::string& hostname) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = hosts_.find(hostname);
    if (it != hosts_.end()) {
        it->second.online = false;
    }
}

void MetricsStore::cleanupOldData(uint64_t max_age_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint64_t current_time = static_cast<uint64_t>(std::time(nullptr));
    
    for (auto& pair : hosts_) {
        auto& host = pair.second;
        
        if (current_time - host.last_update > max_age_seconds) {
            host.online = false;
        }
        
        host.history.erase(
            std::remove_if(host.history.begin(), host.history.end(),
                [current_time, max_age_seconds](const metrics::SystemMetrics& m) {
                    return current_time - m.timestamp > max_age_seconds;
                }),
            host.history.end()
        );
    }
}

size_t MetricsStore::getHostCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hosts_.size();
}

}
}
