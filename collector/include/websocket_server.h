#ifndef BLINKY_COLLECTOR_WEBSOCKET_SERVER_H
#define BLINKY_COLLECTOR_WEBSOCKET_SERVER_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace blinky {
namespace collector {

struct Client {
    int socket_fd;
    std::string hostname;
    uint64_t last_seen;
    bool authenticated;
};

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();
    
    bool start();
    void stop();
    bool isRunning() const;
    
    void setOnMessage(std::function<void(const Client&, const std::string&)> callback);
    void setOnClientConnected(std::function<void(const Client&)> callback);
    void setOnClientDisconnected(std::function<void(const Client&)> callback);
    
    std::vector<Client> getConnectedClients() const;
    
private:
    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    
    std::vector<Client> clients_;
    mutable std::mutex clients_mutex_;
    
    std::thread accept_thread_;
    std::vector<std::thread> client_threads_;
    
    std::function<void(const Client&, const std::string&)> on_message_;
    std::function<void(const Client&)> on_client_connected_;
    std::function<void(const Client&)> on_client_disconnected_;
    
    void acceptLoop();
    void handleClient(int client_fd);
    bool performHandshake(int client_fd);
    std::string receiveFrame(int client_fd);
    void removeClient(int client_fd);
};

}
}

#endif
