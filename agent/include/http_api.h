#pragma once

#include "local_storage.h"
#include <string>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace blinky {
namespace agent {

class HttpApi {
public:
    HttpApi(LocalStorage& storage, int port = 9092)
        : storage_(storage)
        , port_(port)
        , running_(false)
        , server_fd_(-1) {
    }

    ~HttpApi() {
        stop();
    }

    bool start() {
        if (running_) {
            return false;
        }

        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            return false;
        }

        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_fd_);
            return false;
        }

        if (listen(server_fd_, 10) < 0) {
            close(server_fd_);
            return false;
        }

        running_ = true;
        server_thread_ = std::thread(&HttpApi::run, this);

        return true;
    }

    void stop() {
        if (!running_) {
            return;
        }

        running_ = false;
        
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    bool is_running() const {
        return running_;
    }

    int get_port() const {
        return port_;
    }

private:
    LocalStorage& storage_;
    int port_;
    std::atomic<bool> running_;
    int server_fd_;
    std::thread server_thread_;

    void run() {
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (running_) {
                    continue;
                } else {
                    break;
                }
            }

            handle_request(client_fd);
            close(client_fd);
        }
    }

    void handle_request(int client_fd) {
        char buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            return;
        }

        buffer[bytes_read] = '\0';
        std::string request(buffer);

        std::string method, path;
        parse_request_line(request, method, path);

        if (method == "GET") {
            handle_get(client_fd, path);
        } else {
            send_response(client_fd, 405, "Method Not Allowed", "text/plain");
        }
    }

    void parse_request_line(const std::string& request, std::string& method, std::string& path) {
        size_t first_space = request.find(' ');
        size_t second_space = request.find(' ', first_space + 1);
        
        if (first_space != std::string::npos && second_space != std::string::npos) {
            method = request.substr(0, first_space);
            path = request.substr(first_space + 1, second_space - first_space - 1);
        }
    }

    void handle_get(int client_fd, const std::string& path) {
        if (path == "/" || path == "/metrics") {
            std::string json = storage_.get_latest_json(1);
            send_response(client_fd, 200, json, "application/json");
        } else if (path.find("/metrics/latest") == 0) {
            size_t count = 100;
            size_t pos = path.find("?count=");
            if (pos != std::string::npos) {
                count = std::stoi(path.substr(pos + 7));
            }
            std::string json = storage_.get_latest_json(count);
            send_response(client_fd, 200, json, "application/json");
        } else if (path == "/health") {
            send_response(client_fd, 200, "{\"status\":\"ok\"}", "application/json");
        } else if (path == "/stats") {
            std::ostringstream oss;
            oss << "{"
                << "\"total_metrics\":" << storage_.get_total_metrics_count() << ","
                << "\"storage_path\":\"" << storage_.get_storage_path() << "\""
                << "}";
            send_response(client_fd, 200, oss.str(), "application/json");
        } else {
            send_response(client_fd, 404, "Not Found", "text/plain");
        }
    }

    void send_response(int client_fd, int status_code, const std::string& body, const std::string& content_type) {
        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " " << get_status_text(status_code) << "\r\n";
        response << "Content-Type: " << content_type << "\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;

        std::string response_str = response.str();
        send(client_fd, response_str.c_str(), response_str.length(), 0);
    }

    std::string get_status_text(int status_code) {
        switch (status_code) {
            case 200: return "OK";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 500: return "Internal Server Error";
            default: return "Unknown";
        }
    }
};

}
}
