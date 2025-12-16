#ifndef BLINKY_AGENT_COLLECTOR_H
#define BLINKY_AGENT_COLLECTOR_H

#include "metrics.h"
#include <memory>

namespace blinky {
namespace agent {

class Monitor {
public:
    virtual ~Monitor() = default;
    virtual void collect() = 0;
};

class CPUMonitor : public Monitor {
public:
    CPUMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
    uint64_t prev_total_;
    uint64_t prev_idle_;
};

class MemoryMonitor : public Monitor {
public:
    MemoryMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
};

class DiskMonitor : public Monitor {
public:
    DiskMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
};

class SmartMonitor : public Monitor {
public:
    SmartMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
};

class NetworkMonitor : public Monitor {
public:
    NetworkMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
};

class SystemdMonitor : public Monitor {
public:
    SystemdMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
};

class ContainerMonitor : public Monitor {
public:
    ContainerMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
    bool checkDocker();
    bool checkPodman();
};

class KubernetesMonitor : public Monitor {
public:
    KubernetesMonitor(metrics::SystemMetrics& metrics);
    void collect() override;
private:
    metrics::SystemMetrics& metrics_;
    bool detectK8s();
    bool detectK3s();
};

class MetricsCollector {
public:
    MetricsCollector();
    ~MetricsCollector();
    
    void initialize();
    metrics::SystemMetrics collectAll();
    
private:
    metrics::SystemMetrics current_metrics_;
    std::vector<std::unique_ptr<Monitor>> monitors_;
};

}
}

#endif
