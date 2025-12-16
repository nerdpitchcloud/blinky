#include "websocket_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace blinky {
namespace collector {

static std::string base64_encode(const unsigned char* buffer, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    std::string result(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(bio);

    return result;
}

WebSocketServer::WebSocketServer(int port)
    : port_(port), server_fd_(-1), running_(false) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_fd_);
        return false;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd_);
        return false;
    }
    
    if (listen(server_fd_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd_);
        return false;
    }
    
    running_ = true;
    accept_thread_ = std::thread(&WebSocketServer::acceptLoop, this);
    
    return true;
}

void WebSocketServer::stop() {
    running_ = false;
    
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    for (auto& thread : client_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& client : clients_) {
        close(client.socket_fd);
    }
    clients_.clear();
}

bool WebSocketServer::isRunning() const {
    return running_;
}

void WebSocketServer::setOnMessage(std::function<void(const Client&, const std::string&)> callback) {
    on_message_ = callback;
}

void WebSocketServer::setOnClientConnected(std::function<void(const Client&)> callback) {
    on_client_connected_ = callback;
}

void WebSocketServer::setOnClientDisconnected(std::function<void(const Client&)> callback) {
    on_client_disconnected_ = callback;
}

std::vector<Client> WebSocketServer::getConnectedClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_;
}

void WebSocketServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        client_threads_.push_back(std::thread(&WebSocketServer::handleClient, this, client_fd));
    }
}

void WebSocketServer::handleClient(int client_fd) {
    if (!performHandshake(client_fd)) {
        close(client_fd);
        return;
    }
    
    Client client;
    client.socket_fd = client_fd;
    client.hostname = "unknown";
    client.last_seen = std::time(nullptr);
    client.authenticated = true;
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.push_back(client);
    }
    
    if (on_client_connected_) {
        on_client_connected_(client);
    }
    
    while (running_) {
        std::string message = receiveFrame(client_fd);
        
        if (message.empty()) {
            break;
        }
        
        client.last_seen = std::time(nullptr);
        
        if (on_message_) {
            on_message_(client, message);
        }
    }
    
    if (on_client_disconnected_) {
        on_client_disconnected_(client);
    }
    
    removeClient(client_fd);
    close(client_fd);
}

bool WebSocketServer::performHandshake(int client_fd) {
    char buffer[4096];
    ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (received <= 0) {
        return false;
    }
    
    buffer[received] = '\0';
    std::string request(buffer);
    
    std::string key;
    std::istringstream iss(request);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("Sec-WebSocket-Key:") != std::string::npos) {
            size_t pos = line.find(":") + 1;
            key = line.substr(pos);
            
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            break;
        }
    }
    
    if (key.empty()) {
        return false;
    }
    
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string accept_key = key + magic;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(accept_key.c_str()), accept_key.length(), hash);
    
    std::string encoded = base64_encode(hash, SHA_DIGEST_LENGTH);
    
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << encoded << "\r\n";
    response << "\r\n";
    
    std::string response_str = response.str();
    ssize_t sent = send(client_fd, response_str.c_str(), response_str.length(), 0);
    
    return sent > 0;
}

std::string WebSocketServer::receiveFrame(int client_fd) {
    unsigned char header[2];
    ssize_t received = recv(client_fd, header, 2, 0);
    
    if (received != 2) {
        return "";
    }
    
    bool fin = (header[0] & 0x80) != 0;
    unsigned char opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    
    if (opcode == 0x08) {
        return "";
    }
    
    if (payload_len == 126) {
        unsigned char len_bytes[2];
        if (recv(client_fd, len_bytes, 2, 0) != 2) {
            return "";
        }
        payload_len = (len_bytes[0] << 8) | len_bytes[1];
    } else if (payload_len == 127) {
        unsigned char len_bytes[8];
        if (recv(client_fd, len_bytes, 8, 0) != 8) {
            return "";
        }
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | len_bytes[i];
        }
    }
    
    unsigned char mask[4] = {0};
    if (masked) {
        if (recv(client_fd, mask, 4, 0) != 4) {
            return "";
        }
    }
    
    std::vector<unsigned char> payload(payload_len);
    size_t total_received = 0;
    
    while (total_received < payload_len) {
        ssize_t n = recv(client_fd, payload.data() + total_received, payload_len - total_received, 0);
        if (n <= 0) {
            return "";
        }
        total_received += n;
    }
    
    if (masked) {
        for (size_t i = 0; i < payload_len; ++i) {
            payload[i] ^= mask[i % 4];
        }
    }
    
    return std::string(payload.begin(), payload.end());
}

void WebSocketServer::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(
        std::remove_if(clients_.begin(), clients_.end(),
            [client_fd](const Client& c) { return c.socket_fd == client_fd; }),
        clients_.end()
    );
}

}
}
