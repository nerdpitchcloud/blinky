#include "collector.h"
#include "websocket_client.h"
#include "protocol.h"
#include "version.h"
#include "config.h"
#include "local_storage.h"
#include "http_api.h"
#include "upgrade.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace blinky;

static volatile bool running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
    }
}

bool daemonize() {
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << "Failed to fork process" << std::endl;
        return false;
    }
    
    if (pid > 0) {
        exit(0);
    }
    
    if (setsid() < 0) {
        std::cerr << "Failed to create new session" << std::endl;
        return false;
    }
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    
    if (pid < 0) {
        std::cerr << "Failed to fork second time" << std::endl;
        return false;
    }
    
    if (pid > 0) {
        exit(0);
    }
    
    umask(0);
    
    if (chdir("/") < 0) {
        return false;
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
    
    return true;
}

void writePidFile(const std::string& pid_file) {
    std::ofstream file(pid_file);
    if (file.is_open()) {
        file << getpid() << std::endl;
        file.close();
    }
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options|command]\n"
              << "\n"
              << "Commands:\n"
              << "  version                 Show detailed version information\n"
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
              << "  -d, --daemon            Run as daemon in background (default)\n"
              << "  -f, --foreground        Run in foreground with output\n"
              << "  --pid-file FILE         PID file location (default: /var/run/blinky-agent.pid)\n"
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
              << "  " << program << "                    # Start agent in background (daemon)\n"
              << "  " << program << " -f                 # Run in foreground\n"
              << "  " << program << " version            # Show version information\n"
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
        } else if (first_arg == "version") {
            std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
            std::cout << std::endl;
            std::cout << "Build Information:" << std::endl;
            std::cout << "  Architecture: " << 
#if defined(__x86_64__) || defined(_M_X64)
                "amd64 (x86_64)"
#elif defined(__aarch64__) || defined(_M_ARM64)
                "arm64 (aarch64)"
#else
                "unknown"
#endif
                << std::endl;
            std::cout << "  Compiler: "
#if defined(__clang__)
                << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__
#elif defined(__GNUC__)
                << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
#else
                << "Unknown"
#endif
                << std::endl;
            std::cout << "  Platform: "
#if defined(__linux__)
                << "Linux"
#elif defined(__APPLE__)
                << "macOS"
#elif defined(_WIN32)
                << "Windows"
#else
                << "Unknown"
#endif
                << std::endl;
            std::cout << std::endl;
            std::cout << "Repository: https://github.com/nerdpitchcloud/blinky" << std::endl;
            return 0;
        }
    }
    
    Config config;
    std::string config_file;
    std::string mode_override;
    std::string server_host_override;
    int server_port_override = -1;
    int interval_override = -1;
    bool run_as_daemon = true;
    std::string pid_file = "/var/run/blinky-agent.pid";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
            std::cout << std::endl;
            std::cout << "Build Information:" << std::endl;
            std::cout << "  Architecture: " << 
#if defined(__x86_64__) || defined(_M_X64)
                "amd64 (x86_64)"
#elif defined(__aarch64__) || defined(_M_ARM64)
                "arm64 (aarch64)"
#else
                "unknown"
#endif
                << std::endl;
            std::cout << "  Compiler: "
#if defined(__clang__)
                << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__
#elif defined(__GNUC__)
                << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
#else
                << "Unknown"
#endif
                << std::endl;
            std::cout << "  Platform: "
#if defined(__linux__)
                << "Linux"
#elif defined(__APPLE__)
                << "macOS"
#elif defined(_WIN32)
                << "Windows"
#else
                << "Unknown"
#endif
                << std::endl;
            std::cout << std::endl;
            std::cout << "Repository: https://github.com/nerdpitchcloud/blinky" << std::endl;
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
        } else if (arg == "-d" || arg == "--daemon") {
            run_as_daemon = true;
        } else if (arg == "-f" || arg == "--foreground") {
            run_as_daemon = false;
        } else if (arg == "--pid-file" && i + 1 < argc) {
            pid_file = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (run_as_daemon) {
        if (!daemonize()) {
            std::cerr << "Failed to daemonize process" << std::endl;
            return 1;
        }
        writePidFile(pid_file);
    }
    
    if (!config_file.empty()) {
        if (!config.load_from_file(config_file)) {
            return 1;
        }
    } else {
        config.load();
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
    
    if (!run_as_daemon) {
        std::cout << "Blinky Agent " << version::getFullVersionString() << std::endl;
        std::cout << "Operating mode: " << mode << std::endl;
        std::cout << "Collection interval: " << interval_seconds << " seconds" << std::endl;
    }
    
    agent::MetricsCollector collector;
    collector.initialize();
    
    agent::LocalStorage* storage = nullptr;
    if (storage_enabled) {
        storage = new agent::LocalStorage(storage_path, max_files, max_file_size_mb);
        if (!run_as_daemon) {
            std::cout << "Local storage: " << storage_path << std::endl;
        }
    }
    
    agent::HttpApi* http_api = nullptr;
    if (http_api_enabled && storage) {
        http_api = new agent::HttpApi(*storage, api_port);
        if (http_api->start()) {
            if (!run_as_daemon) {
                std::cout << "HTTP API listening on port " << api_port << std::endl;
                std::cout << "  GET http://localhost:" << api_port << "/metrics - Latest metrics" << std::endl;
                std::cout << "  GET http://localhost:" << api_port << "/metrics/latest?count=N - Last N metrics" << std::endl;
                std::cout << "  GET http://localhost:" << api_port << "/health - Health check" << std::endl;
                std::cout << "  GET http://localhost:" << api_port << "/stats - Storage stats" << std::endl;
            }
        } else {
            delete http_api;
            http_api = nullptr;
        }
    }
    
    agent::WebSocketClient* ws_client = nullptr;
    if (collector_enabled) {
        ws_client = new agent::WebSocketClient(server_host, server_port);
        ws_client->setOnError([](const std::string&) {});
        if (!run_as_daemon) {
            std::cout << "Collector: " << server_host << ":" << server_port << std::endl;
        }
    }
    
    if (!run_as_daemon) {
        std::cout << "\nAgent is running..." << std::endl;
    }
    
    while (running) {
        auto metrics = collector.collectAll();
        
        if (storage) {
            storage->store(metrics);
        }
        
        if (ws_client) {
            if (!ws_client->isConnected()) {
                ws_client->connect();
            }
            
            if (ws_client->isConnected()) {
                protocol::Message msg;
                msg.type = protocol::MessageType::METRICS;
                msg.timestamp = metrics.timestamp;
                msg.hostname = metrics.hostname;
                msg.version = version::getVersionString();
                msg.payload = metrics.toJSON();
                
                if (!ws_client->send(msg.serialize())) {
                    ws_client->disconnect();
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
    
    if (run_as_daemon) {
        unlink(pid_file.c_str());
    }
    
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
