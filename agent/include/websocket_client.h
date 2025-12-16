#ifndef BLINKY_AGENT_WEBSOCKET_CLIENT_H
#define BLINKY_AGENT_WEBSOCKET_CLIENT_H

#include <string>
#include <functional>

namespace blinky {
namespace agent {

class WebSocketClient {
public:
    WebSocketClient(const std::string& host, int port);
    ~WebSocketClient();
    
    bool connect();
    void disconnect();
    bool send(const std::string& data);
    bool isConnected() const;
    
    void setOnMessage(std::function<void(const std::string&)> callback);
    void setOnError(std::function<void(const std::string&)> callback);
    
private:
    std::string host_;
    int port_;
    int socket_fd_;
    bool connected_;
    
    std::function<void(const std::string&)> on_message_;
    std::function<void(const std::string&)> on_error_;
    
    bool performHandshake();
    std::string createHandshakeRequest();
};

}
}

#endif
