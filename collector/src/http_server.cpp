#include "http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace blinky {
namespace collector {

HttpServer::HttpServer(int port, MetricsStore& store)
    : port_(port), server_fd_(-1), running_(false), store_(store) {
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "Failed to create HTTP socket" << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set HTTP socket options" << std::endl;
        close(server_fd_);
        return false;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind HTTP socket" << std::endl;
        close(server_fd_);
        return false;
    }
    
    if (listen(server_fd_, 10) < 0) {
        std::cerr << "Failed to listen on HTTP socket" << std::endl;
        close(server_fd_);
        return false;
    }
    
    running_ = true;
    accept_thread_ = std::thread(&HttpServer::acceptLoop, this);
    
    return true;
}

void HttpServer::stop() {
    running_ = false;
    
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

bool HttpServer::isRunning() const {
    return running_;
}

void HttpServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept HTTP client" << std::endl;
            }
            continue;
        }
        
        std::thread(&HttpServer::handleClient, this, client_fd).detach();
    }
}

void HttpServer::handleClient(int client_fd) {
    char buffer[4096];
    ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (received <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[received] = '\0';
    std::string request(buffer);
    
    std::string response = handleRequest(request);
    
    send(client_fd, response.c_str(), response.length(), 0);
    close(client_fd);
}

std::string HttpServer::handleRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    std::string content;
    std::string content_type = "text/html";
    
    if (path == "/" || path == "/dashboard") {
        content = generateDashboard();
    } else if (path.find("/host/") == 0) {
        std::string hostname = path.substr(6);
        content = generateHostDetails(hostname);
    } else if (path == "/api/metrics") {
        content = generateAPIResponse();
        content_type = "application/json";
    } else {
        content = "<html><body><h1>404 Not Found</h1></body></html>";
    }
    
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;
    
    return response.str();
}

std::string HttpServer::generateDashboard() {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html><head>\n";
    html << "<title>Blinky Monitoring Dashboard</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html << "h1 { color: #333; }\n";
    html << ".host-card { background: white; padding: 20px; margin: 10px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html << ".status-online { color: #22c55e; font-weight: bold; }\n";
    html << ".status-offline { color: #ef4444; font-weight: bold; }\n";
    html << ".metric { display: inline-block; margin: 10px 20px 10px 0; }\n";
    html << ".metric-label { color: #666; font-size: 0.9em; }\n";
    html << ".metric-value { font-size: 1.5em; font-weight: bold; color: #333; }\n";
    html << ".progress-bar { width: 200px; height: 20px; background: #e5e7eb; border-radius: 10px; overflow: hidden; display: inline-block; vertical-align: middle; }\n";
    html << ".progress-fill { height: 100%; background: #3b82f6; transition: width 0.3s; }\n";
    html << ".progress-fill.warning { background: #f59e0b; }\n";
    html << ".progress-fill.danger { background: #ef4444; }\n";
    html << "</style>\n";
    html << "<meta http-equiv='refresh' content='5'>\n";
    html << "</head><body>\n";
    
    html << "<h1>Blinky Monitoring Dashboard</h1>\n";
    html << "<p>Total Hosts: " << store_.getHostCount() << "</p>\n";
    
    auto hosts = store_.getAllHosts();
    
    if (hosts.empty()) {
        html << "<p>No hosts connected yet.</p>\n";
    } else {
        for (const auto& pair : hosts) {
            const auto& host = pair.second;
            const auto& metrics = host.latest;
            
            html << "<div class='host-card'>\n";
            html << "<h2>" << host.hostname << " ";
            if (host.online) {
                html << "<span class='status-online'>● ONLINE</span>";
            } else {
                html << "<span class='status-offline'>● OFFLINE</span>";
            }
            html << "</h2>\n";
            
            if (host.online) {
                html << "<div class='metric'>\n";
                html << "<div class='metric-label'>CPU Usage</div>\n";
                html << "<div class='metric-value'>" << std::fixed << std::setprecision(1) << metrics.cpu.usage_percent << "%</div>\n";
                std::string cpu_class = metrics.cpu.usage_percent > 80 ? "danger" : (metrics.cpu.usage_percent > 60 ? "warning" : "");
                html << "<div class='progress-bar'><div class='progress-fill " << cpu_class << "' style='width: " << metrics.cpu.usage_percent << "%'></div></div>\n";
                html << "</div>\n";
                
                html << "<div class='metric'>\n";
                html << "<div class='metric-label'>Memory Usage</div>\n";
                html << "<div class='metric-value'>" << std::fixed << std::setprecision(1) << metrics.memory.usage_percent << "%</div>\n";
                std::string mem_class = metrics.memory.usage_percent > 80 ? "danger" : (metrics.memory.usage_percent > 60 ? "warning" : "");
                html << "<div class='progress-bar'><div class='progress-fill " << mem_class << "' style='width: " << metrics.memory.usage_percent << "%'></div></div>\n";
                html << "</div>\n";
                
                html << "<div class='metric'>\n";
                html << "<div class='metric-label'>Load Average</div>\n";
                html << "<div class='metric-value'>" << std::fixed << std::setprecision(2) << metrics.cpu.load_1min << "</div>\n";
                html << "</div>\n";
                
                html << "<div class='metric'>\n";
                html << "<div class='metric-label'>Uptime</div>\n";
                uint64_t days = metrics.uptime_seconds / 86400;
                uint64_t hours = (metrics.uptime_seconds % 86400) / 3600;
                html << "<div class='metric-value'>" << days << "d " << hours << "h</div>\n";
                html << "</div>\n";
                
                if (!metrics.disks.empty()) {
                    html << "<h3>Disks</h3>\n";
                    for (const auto& disk : metrics.disks) {
                        html << "<div class='metric'>\n";
                        html << "<div class='metric-label'>" << disk.mount_point << "</div>\n";
                        html << "<div class='metric-value'>" << std::fixed << std::setprecision(1) << disk.usage_percent << "%</div>\n";
                        std::string disk_class = disk.usage_percent > 80 ? "danger" : (disk.usage_percent > 60 ? "warning" : "");
                        html << "<div class='progress-bar'><div class='progress-fill " << disk_class << "' style='width: " << disk.usage_percent << "%'></div></div>\n";
                        html << "</div>\n";
                    }
                }
                
                if (!metrics.containers.empty()) {
                    html << "<h3>Containers (" << metrics.containers.size() << ")</h3>\n";
                    for (const auto& container : metrics.containers) {
                        html << "<p>" << container.runtime << ": " << container.name << " (" << container.state << ")</p>\n";
                    }
                }
                
                if (metrics.kubernetes.detected) {
                    html << "<h3>Kubernetes</h3>\n";
                    html << "<p>Type: " << metrics.kubernetes.cluster_type << " | Pods: " << metrics.kubernetes.pod_count << " | Nodes: " << metrics.kubernetes.node_count << "</p>\n";
                }
            }
            
            html << "</div>\n";
        }
    }
    
    html << "</body></html>\n";
    
    return html.str();
}

std::string HttpServer::generateHostDetails(const std::string& hostname) {
    auto host = store_.getHostMetrics(hostname);
    
    std::ostringstream html;
    html << "<html><body>\n";
    html << "<h1>Host Details: " << hostname << "</h1>\n";
    html << "<p>Status: " << (host.online ? "Online" : "Offline") << "</p>\n";
    html << "</body></html>\n";
    
    return html.str();
}

std::string HttpServer::generateAPIResponse() {
    std::ostringstream json;
    json << "{\"hosts\":[";
    
    auto hosts = store_.getAllHosts();
    bool first = true;
    
    for (const auto& pair : hosts) {
        if (!first) json << ",";
        first = false;
        
        const auto& host = pair.second;
        json << "{";
        json << "\"hostname\":\"" << host.hostname << "\",";
        json << "\"online\":" << (host.online ? "true" : "false") << ",";
        json << "\"metrics\":" << host.latest.toJSON();
        json << "}";
    }
    
    json << "]}";
    
    return json.str();
}

}
}
