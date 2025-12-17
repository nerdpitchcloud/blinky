#pragma once

#include "version.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>

namespace blinky {
namespace agent {

class Upgrader {
public:
    Upgrader(const std::string& repo = "nerdpitchcloud/blinky")
        : repo_(repo) {
    }

    struct VersionInfo {
        std::string version;
        std::string download_url;
        bool is_newer;
    };

    VersionInfo check_for_updates() {
        VersionInfo info;
        info.is_newer = false;

        std::string current_version = version::getVersionString();
        std::cout << "Current version: " << current_version << std::endl;

        std::string arch = detect_architecture();
        std::cout << "Architecture: " << arch << std::endl;

        std::string api_url = "https://api.github.com/repos/" + repo_ + "/releases/latest";
        std::string response = fetch_url(api_url);

        if (response.empty()) {
            std::cerr << "Failed to fetch release information from GitHub" << std::endl;
            return info;
        }

        info.version = parse_version_from_json(response);
        info.download_url = parse_download_url_from_json(response, arch);

        if (info.version.empty() || info.download_url.empty()) {
            std::cerr << "Failed to parse release information" << std::endl;
            return info;
        }

        std::cout << "Latest version: " << info.version << std::endl;

        info.is_newer = compare_versions(current_version, info.version);

        return info;
    }

    bool upgrade() {
        std::cout << "Checking for updates..." << std::endl;
        std::cout << std::endl;

        auto info = check_for_updates();

        if (!info.is_newer) {
            std::cout << "You are already running the latest version!" << std::endl;
            return true;
        }

        std::cout << std::endl;
        std::cout << "New version available: " << info.version << std::endl;
        std::cout << "Download URL: " << info.download_url << std::endl;
        std::cout << std::endl;

        if (geteuid() != 0) {
            std::cerr << "Error: Upgrade requires root privileges" << std::endl;
            std::cerr << "Please run: sudo blinky-agent upgrade" << std::endl;
            return false;
        }

        std::cout << "Downloading new version..." << std::endl;
        std::string tmp_dir = "/tmp/blinky-upgrade-" + std::to_string(getpid());
        
        if (!download_and_extract(info.download_url, tmp_dir)) {
            std::cerr << "Failed to download or extract update" << std::endl;
            cleanup_temp(tmp_dir);
            return false;
        }

        std::cout << "Installing new version..." << std::endl;
        if (!install_binary(tmp_dir)) {
            std::cerr << "Failed to install update" << std::endl;
            cleanup_temp(tmp_dir);
            return false;
        }

        cleanup_temp(tmp_dir);

        std::cout << std::endl;
        std::cout << "✓ Upgrade successful!" << std::endl;
        std::cout << "Upgraded from " << version::getVersionString() << " to " << info.version << std::endl;
        std::cout << std::endl;
        std::cout << "If running as a service, restart it:" << std::endl;
        std::cout << "  sudo systemctl restart blinky-agent" << std::endl;

        return true;
    }

private:
    std::string repo_;

    std::string detect_architecture() {
        #if defined(__x86_64__) || defined(_M_X64)
            return "amd64";
        #elif defined(__aarch64__) || defined(_M_ARM64)
            return "arm64";
        #else
            return "unknown";
        #endif
    }

    std::string fetch_url(const std::string& url) {
        std::string cmd = "curl -s -L '" + url + "' 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            return "";
        }

        std::stringstream ss;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            ss << buffer;
        }

        pclose(pipe);
        return ss.str();
    }

    std::string parse_version_from_json(const std::string& json) {
        size_t tag_pos = json.find("\"tag_name\"");
        if (tag_pos == std::string::npos) {
            return "";
        }

        size_t start = json.find("\"", tag_pos + 11);
        if (start == std::string::npos) {
            return "";
        }
        start++;

        size_t end = json.find("\"", start);
        if (end == std::string::npos) {
            return "";
        }

        return json.substr(start, end - start);
    }

    std::string parse_download_url_from_json(const std::string& json, const std::string& arch) {
        std::string search_pattern = "blinky-agent-";
        std::string arch_pattern = "-linux-" + arch + ".tar.gz";

        size_t pos = 0;
        while ((pos = json.find("browser_download_url", pos)) != std::string::npos) {
            size_t url_start = json.find("\"", pos + 21);
            if (url_start == std::string::npos) break;
            url_start++;

            size_t url_end = json.find("\"", url_start);
            if (url_end == std::string::npos) break;

            std::string url = json.substr(url_start, url_end - url_start);
            
            if (url.find(search_pattern) != std::string::npos && 
                url.find(arch_pattern) != std::string::npos) {
                return url;
            }

            pos = url_end;
        }

        return "";
    }

    bool compare_versions(const std::string& current, const std::string& latest) {
        auto parse_version = [](const std::string& v) -> std::tuple<int, int, int> {
            std::string ver = v;
            if (ver[0] == 'v') ver = ver.substr(1);

            int major = 0, minor = 0, patch = 0;
            size_t pos1 = ver.find('.');
            size_t pos2 = ver.find('.', pos1 + 1);

            if (pos1 != std::string::npos) {
                major = std::stoi(ver.substr(0, pos1));
                if (pos2 != std::string::npos) {
                    minor = std::stoi(ver.substr(pos1 + 1, pos2 - pos1 - 1));
                    patch = std::stoi(ver.substr(pos2 + 1));
                } else {
                    minor = std::stoi(ver.substr(pos1 + 1));
                }
            }

            return {major, minor, patch};
        };

        auto [c_major, c_minor, c_patch] = parse_version(current);
        auto [l_major, l_minor, l_patch] = parse_version(latest);

        if (l_major > c_major) return true;
        if (l_major < c_major) return false;
        if (l_minor > c_minor) return true;
        if (l_minor < c_minor) return false;
        return l_patch > c_patch;
    }

    bool download_and_extract(const std::string& url, const std::string& tmp_dir) {
        std::filesystem::create_directories(tmp_dir);

        std::string tar_file = tmp_dir + "/blinky-agent.tar.gz";
        std::string cmd = "curl -L -f -o '" + tar_file + "' '" + url + "' 2>/dev/null";
        
        if (system(cmd.c_str()) != 0) {
            return false;
        }

        cmd = "cd '" + tmp_dir + "' && tar -xzf blinky-agent.tar.gz 2>/dev/null";
        if (system(cmd.c_str()) != 0) {
            return false;
        }

        return std::filesystem::exists(tmp_dir + "/blinky-agent");
    }

    bool install_binary(const std::string& tmp_dir) {
        std::string source = tmp_dir + "/blinky-agent";
        std::string dest = "/usr/local/bin/blinky-agent";
        std::string backup = dest + ".backup";

        if (std::filesystem::exists(dest)) {
            std::filesystem::copy_file(dest, backup, 
                std::filesystem::copy_options::overwrite_existing);
        }

        try {
            std::filesystem::copy_file(source, dest,
                std::filesystem::copy_options::overwrite_existing);
            
            chmod(dest.c_str(), 0755);

            if (std::filesystem::exists(backup)) {
                std::filesystem::remove(backup);
            }

            return true;
        } catch (...) {
            if (std::filesystem::exists(backup)) {
                std::filesystem::copy_file(backup, dest,
                    std::filesystem::copy_options::overwrite_existing);
                std::filesystem::remove(backup);
            }
            return false;
        }
    }

    void cleanup_temp(const std::string& tmp_dir) {
        try {
            std::filesystem::remove_all(tmp_dir);
        } catch (...) {
            // Ignore cleanup errors
        }
    }
};

}
}
