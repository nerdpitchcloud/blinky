#ifndef BLINKY_COLLECTOR_METRICS_STORE_H
#define BLINKY_COLLECTOR_METRICS_STORE_H

#include "metrics.h"
#include <map>
#include <vector>
#include <mutex>
#include <deque>

namespace blinky {
namespace collector {

struct HostMetricsHistory {
    std::string hostname;
    std::string agent_version;
    metrics::SystemMetrics latest;
    std::deque<metrics::SystemMetrics> history;
    uint64_t last_update;
    bool online;
    bool version_mismatch;
    
    static const size_t MAX_HISTORY = 1000;
};

class MetricsStore {
public:
    MetricsStore();
    ~MetricsStore();
    
    void storeMetrics(const metrics::SystemMetrics& metrics, const std::string& agent_version);
    
    HostMetricsHistory getHostMetrics(const std::string& hostname) const;
    std::vector<std::string> getHostnames() const;
    std::map<std::string, HostMetricsHistory> getAllHosts() const;
    
    void markHostOffline(const std::string& hostname);
    void cleanupOldData(uint64_t max_age_seconds = 3600);
    
    size_t getHostCount() const;
    
private:
    std::map<std::string, HostMetricsHistory> hosts_;
    mutable std::mutex mutex_;
};

}
}

#endif
