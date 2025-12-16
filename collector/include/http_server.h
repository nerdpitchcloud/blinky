#ifndef BLINKY_COLLECTOR_HTTP_SERVER_H
#define BLINKY_COLLECTOR_HTTP_SERVER_H

#include "metrics_store.h"
#include <string>
#include <thread>
#include <atomic>

namespace blinky {
namespace collector {

class HttpServer {
public:
    HttpServer(int port, MetricsStore& store);
    ~HttpServer();
    
    bool start();
    void stop();
    bool isRunning() const;
    
private:
    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    MetricsStore& store_;
    
    std::thread accept_thread_;
    
    void acceptLoop();
    void handleClient(int client_fd);
    
    std::string handleRequest(const std::string& request);
    std::string generateDashboard();
    std::string generateHostDetails(const std::string& hostname);
    std::string generateAPIResponse();
};

}
}

#endif
