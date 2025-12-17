#include "collector.h"
#include "websocket_client.h"
#include "protocol.h"
#include "version.h"
#include "config.h"
#include "local_storage.h"
#include "http_api.h"
#include "upgrade.h"
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
    std::cout << "Usage: " << program << " [options|command]\n"
              << "\n"
              << "Commands:\n"
              << "  upgrade                 Check for updates and upgrade to latest version\n"
              << "\n"
              << "Options:\n"
              << "  -h, --help              Show this help message\n"
              << "  -v, --version           Show version information\n"
              << "  -c, --config FILE       Config file path (default: auto-detect)\n"
              << "  -m, --mode MODE         Operating mode: local, push, pull, hybrid\n"
              << "  -s, --server HOST       Collector server host (overrides config)\n"
              << "  -p, --port PORT         Collector server port (overrides config)\n"
              << "  -i, --interval SECONDS  Collection interval (overrides config)\n"
              << "\n"
              << "Operating Modes:\n"
              << "  local   - Store metrics locally only\n"
              << "  pull    - Local storage + HTTP API (port 9092) [default]\n"
              << "  push    - Push metrics to collector\n"
              << "  hybrid  - Both push and local storage with API\n"
              << "\n"
              << "Config file locations (in order of precedence):\n"
              << "  /etc/blinky/config.toml\n"
              << "  ~/.blinky/config.toml\n"
              << "  ./config.toml\n"
              << "\n"
              << "Examples:\n"
              << "  " << program << "                    # Start agent with default config\n"
              << "  " << program << " upgrade            # Upgrade to latest version\n"
              << "  " << program << " -m pull            # Run in pull mode\n"
              << "  " << program << " -m push -s host    # Push to collector\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string first_arg = argv[1];
        if (first_arg == "upgrade") {
            agent::Upgrader upgrader;
            return upgrader.upgrade() ? 0 : 1;
        }
    }
    
    Config config;
    std::string config_file;
    std::string mode_override;
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
        } else if ((arg == "-m" || arg == "--mode") && i + 1 < argc) {
            mode_override = argv[++i];
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
    
    std::string mode = mode_override.empty()
        ? config.get_string("agent.mode", "local")
        : mode_override;
    
    std::string server_host = server_host_override.empty() 
        ? config.get_string("collector.host", "localhost")
        : server_host_override;
    
    int server_port = server_port_override != -1
        ? server_port_override
        : config.get_int("collector.port", 9090);
    
    int interval_seconds = interval_override != -1
        ? interval_override
        : config.get_int("agent.interval", 5);
    
    std::string storage_path = config.get_string("storage.path", "/var/lib/blinky/metrics");
    size_t max_files = config.get_int("storage.max_files", 100);
    size_t max_file_size_mb = config.get_int("storage.max_file_size_mb", 10);
    
    bool api_enabled = config.get_bool("api.enabled", true);
    int api_port = config.get_int("api.port", 9092);
    
    bool collector_enabled = (mode == "push" || mode == "hybrid") || 
                            config.get_bool("collector.enabled", false);
    bool storage_enabled = (mode == "local" || mode == "pull" || mode == "hybrid");
    bool http_api_enabled = (mode == "pull" || mode == "hybrid") && api_enabled;
    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
    std::cout << "Operating mode: " << mode << std::endl;
    std::cout << "Collection interval: " << interval_seconds << " seconds" << std::endl;
    
    agent::MetricsCollector collector;
    collector.initialize();
    
    agent::LocalStorage* storage = nullptr;
    if (storage_enabled) {
        storage = new agent::LocalStorage(storage_path, max_files, max_file_size_mb);
        std::cout << "Local storage: " << storage_path << std::endl;
    }
    
    agent::HttpApi* http_api = nullptr;
    if (http_api_enabled && storage) {
        http_api = new agent::HttpApi(*storage, api_port);
        if (http_api->start()) {
            std::cout << "HTTP API listening on port " << api_port << std::endl;
            std::cout << "  GET http://localhost:" << api_port << "/metrics - Latest metrics" << std::endl;
            std::cout << "  GET http://localhost:" << api_port << "/metrics/latest?count=N - Last N metrics" << std::endl;
            std::cout << "  GET http://localhost:" << api_port << "/health - Health check" << std::endl;
            std::cout << "  GET http://localhost:" << api_port << "/stats - Storage stats" << std::endl;
        } else {
            std::cerr << "Failed to start HTTP API on port " << api_port << std::endl;
            delete http_api;
            http_api = nullptr;
        }
    }
    
    agent::WebSocketClient* ws_client = nullptr;
    if (collector_enabled) {
        ws_client = new agent::WebSocketClient(server_host, server_port);
        ws_client->setOnError([](const std::string& error) {
            std::cerr << "WebSocket error: " << error << std::endl;
        });
        std::cout << "Collector: " << server_host << ":" << server_port << std::endl;
    }
    
    std::cout << "\nAgent is running..." << std::endl;
    
    while (running) {
        auto metrics = collector.collectAll();
        
        if (storage) {
            if (!storage->store(metrics)) {
                std::cerr << "Failed to store metrics locally" << std::endl;
            }
        }
        
        if (ws_client) {
            if (!ws_client->isConnected()) {
                if (ws_client->connect()) {
                    std::cout << "Connected to collector" << std::endl;
                }
            }
            
            if (ws_client->isConnected()) {
                protocol::Message msg;
                msg.type = protocol::MessageType::METRICS;
                msg.timestamp = metrics.timestamp;
                msg.hostname = metrics.hostname;
                msg.version = version::getVersionString();
                msg.payload = metrics.toJSON();
                
                if (!ws_client->send(msg.serialize())) {
                    std::cerr << "Failed to send metrics to collector" << std::endl;
                    ws_client->disconnect();
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
    
    std::cout << "\nShutting down agent..." << std::endl;
    
    if (http_api) {
        http_api->stop();
        delete http_api;
    }
    
    if (ws_client) {
        ws_client->disconnect();
        delete ws_client;
    }
    
    if (storage) {
        delete storage;
    }
    
    return 0;
}
