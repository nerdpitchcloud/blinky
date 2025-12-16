#include "collector.h"
#include "websocket_client.h"
#include "protocol.h"
#include "version.h"
#include "config.h"
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
              << "  -c, --config FILE       Config file path (default: auto-detect)\n"
              << "  -s, --server HOST       Collector server host (overrides config)\n"
              << "  -p, --port PORT         Collector server port (overrides config)\n"
              << "  -i, --interval SECONDS  Collection interval (overrides config)\n"
              << "\n"
              << "Config file locations (in order of precedence):\n"
              << "  /etc/blinky/config.toml\n"
              << "  ~/.blinky/config.toml\n"
              << "  ./config.toml\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    Config config;
    std::string config_file;
    std::string server_host_override;
    int server_port_override = -1;
    int interval_override = -1;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
            return 0;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        } else if ((arg == "-s" || arg == "--server") && i + 1 < argc) {
            server_host_override = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            server_port_override = std::atoi(argv[++i]);
        } else if ((arg == "-i" || arg == "--interval") && i + 1 < argc) {
            interval_override = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (!config_file.empty()) {
        if (!config.load_from_file(config_file)) {
            std::cerr << "Failed to load config file: " << config_file << std::endl;
            return 1;
        }
        std::cout << "Loaded config from: " << config_file << std::endl;
    } else {
        if (config.load()) {
            std::cout << "Loaded config from: " << config.get_config_path() << std::endl;
        } else {
            std::cout << "No config file found, using defaults" << std::endl;
        }
    }
    
    std::string server_host = server_host_override.empty() 
        ? config.get_string("collector.host", "localhost")
        : server_host_override;
    
    int server_port = server_port_override != -1
        ? server_port_override
        : config.get_int("collector.port", 9090);
    
    int interval_seconds = interval_override != -1
        ? interval_override
        : config.get_int("agent.interval", 5);
    
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
