#include "collector.h"
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <sys/stat.h>
#include <fstream>

namespace blinky {
namespace agent {

KubernetesMonitor::KubernetesMonitor(metrics::SystemMetrics& metrics)
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

bool KubernetesMonitor::detectK8s() {
    struct stat buffer;
    if (stat("/etc/kubernetes", &buffer) == 0) {
        return true;
    }
    
    std::ifstream cgroup("/proc/1/cgroup");
    if (cgroup.is_open()) {
        std::string line;
        while (std::getline(cgroup, line)) {
            if (line.find("kubepods") != std::string::npos) {
                return true;
            }
        }
    }
    
    return false;
}

bool KubernetesMonitor::detectK3s() {
    struct stat buffer;
    if (stat("/etc/rancher/k3s", &buffer) == 0) {
        return true;
    }
    
    std::string output = exec("which k3s 2>/dev/null");
    return !output.empty();
}

void KubernetesMonitor::collect() {
    metrics_.kubernetes.detected = false;
    metrics_.kubernetes.cluster_type = "none";
    metrics_.kubernetes.pod_count = 0;
    metrics_.kubernetes.node_count = 0;
    metrics_.kubernetes.namespaces.clear();
    
    bool is_k3s = detectK3s();
    bool is_k8s = detectK8s();
    
    if (!is_k3s && !is_k8s) {
        return;
    }
    
    metrics_.kubernetes.detected = true;
    metrics_.kubernetes.cluster_type = is_k3s ? "k3s" : "k8s";
    
    std::string kubectl_cmd = is_k3s ? "k3s kubectl" : "kubectl";
    
    std::string kubeconfig_check = exec((kubectl_cmd + " version --client 2>/dev/null").c_str());
    if (kubeconfig_check.empty()) {
        return;
    }
    
    std::string pods_output = exec((kubectl_cmd + " get pods --all-namespaces --no-headers 2>/dev/null | wc -l").c_str());
    if (!pods_output.empty()) {
        metrics_.kubernetes.pod_count = std::stoi(pods_output);
    }
    
    std::string nodes_output = exec((kubectl_cmd + " get nodes --no-headers 2>/dev/null | wc -l").c_str());
    if (!nodes_output.empty()) {
        metrics_.kubernetes.node_count = std::stoi(nodes_output);
    }
    
    std::string ns_output = exec((kubectl_cmd + " get namespaces --no-headers 2>/dev/null").c_str());
    if (!ns_output.empty()) {
        std::istringstream iss(ns_output);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (line.empty()) {
                continue;
            }
            
            std::istringstream line_stream(line);
            std::string ns_name;
            line_stream >> ns_name;
            
            if (!ns_name.empty()) {
                metrics_.kubernetes.namespaces.push_back(ns_name);
            }
        }
    }
}

}
}
