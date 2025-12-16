#include "websocket_server.h"
#include "metrics_store.h"
#include "http_server.h"
#include "protocol.h"
#include "version.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

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
              << "  -w, --ws-port PORT      WebSocket port (default: 9090)\n"
              << "  -p, --http-port PORT    HTTP dashboard port (default: 9091)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    int ws_port = 9090;
    int http_port = 9091;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Blinky Collector " << version::getFullVersionString() << std::endl;
            return 0;
        } else if ((arg == "-w" || arg == "--ws-port") && i + 1 < argc) {
            ws_port = std::atoi(argv[++i]);
        } else if ((arg == "-p" || arg == "--http-port") && i + 1 < argc) {
            http_port = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << "Blinky Collector " << version::getFullVersionString() << std::endl;
    std::cout << "WebSocket server port: " << ws_port << std::endl;
    std::cout << "HTTP dashboard port: " << http_port << std::endl;
    
    collector::MetricsStore store;
    collector::WebSocketServer ws_server(ws_port);
    collector::HttpServer http_server(http_port, store);
    
    ws_server.setOnMessage([&store](const collector::Client& client, const std::string& data) {
        try {
            protocol::Message msg = protocol::Message::deserialize(data);
            
            if (msg.type == protocol::MessageType::METRICS) {
                metrics::SystemMetrics metrics = metrics::SystemMetrics::fromJSON(msg.payload);
                metrics.hostname = msg.hostname;
                metrics.timestamp = msg.timestamp;
                
                store.storeMetrics(metrics, msg.version);
                
                std::cout << "Received metrics from " << msg.hostname 
                          << " v" << msg.version
                          << " (CPU: " << metrics.cpu.usage_percent << "%)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing message: " << e.what() << std::endl;
        }
    });
    
    ws_server.setOnClientConnected([](const collector::Client& client) {
        std::cout << "Agent connected: " << client.hostname << std::endl;
    });
    
    ws_server.setOnClientDisconnected([&store](const collector::Client& client) {
        std::cout << "Agent disconnected: " << client.hostname << std::endl;
        store.markHostOffline(client.hostname);
    });
    
    if (!ws_server.start()) {
        std::cerr << "Failed to start WebSocket server" << std::endl;
        return 1;
    }
    
    if (!http_server.start()) {
        std::cerr << "Failed to start HTTP server" << std::endl;
        ws_server.stop();
        return 1;
    }
    
    std::cout << "\nCollector is running..." << std::endl;
    std::cout << "Dashboard available at: http://localhost:" << http_port << "/" << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;
    
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        store.cleanupOldData(3600);
    }
    
    std::cout << "\nShutting down collector..." << std::endl;
    http_server.stop();
    ws_server.stop();
    
    return 0;
}
