#include "websocket_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace blinky {
namespace agent {

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

WebSocketClient::WebSocketClient(const std::string& host, int port)
    : host_(host), port_(port), socket_fd_(-1), connected_(false) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect() {
    struct addrinfo hints, *result, *rp;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string port_str = std::to_string(port_);
    
    if (getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &result) != 0) {
        if (on_error_) {
            on_error_("Failed to resolve host: " + host_);
        }
        return false;
    }
    
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        socket_fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd_ == -1) {
            continue;
        }
        
        if (::connect(socket_fd_, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }
        
        close(socket_fd_);
        socket_fd_ = -1;
    }
    
    freeaddrinfo(result);
    
    if (socket_fd_ == -1) {
        if (on_error_) {
            on_error_("Failed to connect to " + host_ + ":" + std::to_string(port_));
        }
        return false;
    }
    
    if (!performHandshake()) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    return true;
}

void WebSocketClient::disconnect() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

bool WebSocketClient::send(const std::string& data) {
    if (!connected_ || socket_fd_ == -1) {
        return false;
    }
    
    size_t data_len = data.length();
    std::vector<unsigned char> frame;
    
    frame.push_back(0x81);
    
    if (data_len <= 125) {
        frame.push_back(0x80 | static_cast<unsigned char>(data_len));
    } else if (data_len <= 65535) {
        frame.push_back(0x80 | 126);
        frame.push_back((data_len >> 8) & 0xFF);
        frame.push_back(data_len & 0xFF);
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((data_len >> (i * 8)) & 0xFF);
        }
    }
    
    unsigned char mask[4];
    for (int i = 0; i < 4; ++i) {
        mask[i] = rand() % 256;
        frame.push_back(mask[i]);
    }
    
    for (size_t i = 0; i < data_len; ++i) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }
    
    ssize_t sent = ::send(socket_fd_, frame.data(), frame.size(), 0);
    if (sent < 0 || static_cast<size_t>(sent) != frame.size()) {
        connected_ = false;
        return false;
    }
    
    return true;
}

bool WebSocketClient::isConnected() const {
    return connected_;
}

void WebSocketClient::setOnMessage(std::function<void(const std::string&)> callback) {
    on_message_ = callback;
}

void WebSocketClient::setOnError(std::function<void(const std::string&)> callback) {
    on_error_ = callback;
}

bool WebSocketClient::performHandshake() {
    std::string key = "x3JJHMbDL1EzLkh9GBhXDw==";
    
    std::string handshake = createHandshakeRequest();
    
    ssize_t sent = ::send(socket_fd_, handshake.c_str(), handshake.length(), 0);
    if (sent < 0) {
        if (on_error_) {
            on_error_("Failed to send handshake");
        }
        return false;
    }
    
    char buffer[4096];
    ssize_t received = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        if (on_error_) {
            on_error_("Failed to receive handshake response");
        }
        return false;
    }
    
    buffer[received] = '\0';
    std::string response(buffer);
    
    if (response.find("101") == std::string::npos && 
        response.find("Switching Protocols") == std::string::npos) {
        if (on_error_) {
            on_error_("Invalid handshake response");
        }
        return false;
    }
    
    return true;
}

std::string WebSocketClient::createHandshakeRequest() {
    std::ostringstream request;
    
    request << "GET /ws HTTP/1.1\r\n";
    request << "Host: " << host_ << ":" << port_ << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    
    return request.str();
}

}
}
