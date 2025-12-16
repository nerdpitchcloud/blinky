#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace blinky {

class Config {
public:
    Config() {
        set_defaults();
    }

    bool load_from_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        std::string current_section;
        int line_number = 0;

        while (std::getline(file, line)) {
            line_number++;
            line = trim(line);

            if (line.empty() || line[0] == '#') {
                continue;
            }

            if (line[0] == '[' && line[line.length() - 1] == ']') {
                current_section = line.substr(1, line.length() - 2);
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) {
                continue;
            }

            std::string key = trim(line.substr(0, eq_pos));
            std::string value = trim(line.substr(eq_pos + 1));

            if (!current_section.empty()) {
                key = current_section + "." + key;
            }

            parse_value(key, value);
        }

        return true;
    }

    bool load() {
        std::vector<std::string> config_paths = {
            "/etc/blinky/config.toml",
            std::string(getenv("HOME") ? getenv("HOME") : "") + "/.blinky/config.toml",
            "./config.toml"
        };

        for (const auto& path : config_paths) {
            if (load_from_file(path)) {
                config_file_path = path;
                return true;
            }
        }

        return false;
    }

    std::string get_string(const std::string& key, const std::string& default_value = "") const {
        auto it = values.find(key);
        if (it != values.end()) {
            return unquote(it->second);
        }
        return default_value;
    }

    int get_int(const std::string& key, int default_value = 0) const {
        auto it = values.find(key);
        if (it != values.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    bool get_bool(const std::string& key, bool default_value = false) const {
        auto it = values.find(key);
        if (it != values.end()) {
            std::string val = it->second;
            std::transform(val.begin(), val.end(), val.begin(), ::tolower);
            return val == "true" || val == "1" || val == "yes";
        }
        return default_value;
    }

    double get_double(const std::string& key, double default_value = 0.0) const {
        auto it = values.find(key);
        if (it != values.end()) {
            try {
                return std::stod(it->second);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    std::vector<std::string> get_array(const std::string& key) const {
        auto it = values.find(key);
        if (it != values.end()) {
            return parse_array(it->second);
        }
        return {};
    }

    std::string get_config_path() const {
        return config_file_path;
    }

    void set_defaults() {
        values["agent.interval"] = "5";
        values["agent.monitors.cpu"] = "true";
        values["agent.monitors.memory"] = "true";
        values["agent.monitors.disk"] = "true";
        values["agent.monitors.smart"] = "true";
        values["agent.monitors.network"] = "true";
        values["agent.monitors.systemd"] = "true";
        values["agent.monitors.containers"] = "true";
        values["agent.monitors.kubernetes"] = "true";
        
        values["collector.host"] = "localhost";
        values["collector.port"] = "9090";
        values["collector.timeout"] = "10";
        
        values["collector.reconnect.enabled"] = "true";
        values["collector.reconnect.initial_delay"] = "5";
        values["collector.reconnect.max_delay"] = "300";
        values["collector.reconnect.backoff_multiplier"] = "2.0";
        values["collector.reconnect.max_attempts"] = "0";
        
        values["logging.level"] = "info";
        values["logging.output"] = "stdout";
        values["logging.timestamps"] = "true";
        values["logging.colors"] = "true";
        
        values["disk.min_size_gb"] = "0";
        
        values["smart.enabled"] = "true";
        values["smart.smartctl_path"] = "/usr/sbin/smartctl";
        
        values["network.include_virtual"] = "false";
        
        values["systemd.enabled"] = "true";
        values["systemd.only_failed"] = "false";
        
        values["containers.docker"] = "true";
        values["containers.docker_socket"] = "/var/run/docker.sock";
        values["containers.podman"] = "true";
        values["containers.podman_socket"] = "/run/podman/podman.sock";
        values["containers.include_stopped"] = "false";
        
        values["kubernetes.enabled"] = "true";
        values["kubernetes.include_system"] = "false";
        
        values["security.tls"] = "false";
        values["security.verify_cert"] = "true";
        
        values["performance.buffer_size"] = "100";
        values["performance.worker_threads"] = "4";
        values["performance.compression"] = "false";
        values["performance.aggregation_window"] = "0";
    }

private:
    std::map<std::string, std::string> values;
    std::string config_file_path;

    std::string trim(const std::string& str) const {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::string unquote(const std::string& str) const {
        if (str.length() >= 2) {
            if ((str.front() == '"' && str.back() == '"') ||
                (str.front() == '\'' && str.back() == '\'')) {
                return str.substr(1, str.length() - 2);
            }
        }
        return str;
    }

    std::vector<std::string> parse_array(const std::string& str) const {
        std::vector<std::string> result;
        std::string trimmed = trim(str);
        
        if (trimmed.empty() || trimmed.front() != '[' || trimmed.back() != ']') {
            return result;
        }

        std::string content = trimmed.substr(1, trimmed.length() - 2);
        std::stringstream ss(content);
        std::string item;

        while (std::getline(ss, item, ',')) {
            item = trim(item);
            if (!item.empty()) {
                result.push_back(unquote(item));
            }
        }

        return result;
    }

    void parse_value(const std::string& key, const std::string& value) {
        values[key] = value;
    }
};

}
