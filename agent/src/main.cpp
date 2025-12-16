#include "collector.h"
#include "websocket_client.h"
#include "protocol.h"
#include "version.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <cstring>
#include <unistd.h>

using namespace blinky;

static volatile bool running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
    }
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "Options:\n"
              << "  -h, --help              Show this help message\n"
              << "  -v, --version           Show version information\n"
              << "  -s, --server HOST       Collector server host (default: localhost)\n"
              << "  -p, --port PORT         Collector server port (default: 9090)\n"
              << "  -i, --interval SECONDS  Collection interval (default: 5)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::string server_host = "localhost";
    int server_port = 9090;
    int interval_seconds = 5;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
            return 0;
        } else if ((arg == "-s" || arg == "--server") && i + 1 < argc) {
            server_host = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            server_port = std::atoi(argv[++i]);
        } else if ((arg == "-i" || arg == "--interval") && i + 1 < argc) {
            interval_seconds = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
    std::cout << "Connecting to collector at " << server_host << ":" << server_port << std::endl;
    std::cout << "Collection interval: " << interval_seconds << " seconds" << std::endl;
    
    agent::MetricsCollector collector;
    collector.initialize();
    
    agent::WebSocketClient ws_client(server_host, server_port);
    
    ws_client.setOnError([](const std::string& error) {
        std::cerr << "WebSocket error: " << error << std::endl;
    });
    
    while (running) {
        if (!ws_client.isConnected()) {
            std::cout << "Attempting to connect to collector..." << std::endl;
            if (ws_client.connect()) {
                std::cout << "Connected to collector" << std::endl;
            } else {
                std::cerr << "Failed to connect, retrying in 10 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(10));
                continue;
            }
        }
        
        auto metrics = collector.collectAll();
        
        protocol::Message msg;
        msg.type = protocol::MessageType::METRICS;
        msg.timestamp = metrics.timestamp;
        msg.hostname = metrics.hostname;
        msg.version = version::getVersionString();
        msg.payload = metrics.toJSON();
        
        if (!ws_client.send(msg.serialize())) {
            std::cerr << "Failed to send metrics" << std::endl;
            ws_client.disconnect();
        } else {
            std::cout << "Metrics sent successfully" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
    
    std::cout << "\nShutting down agent..." << std::endl;
    ws_client.disconnect();
    
    return 0;
}
