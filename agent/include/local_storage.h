#pragma once

#include "metrics.h"
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace blinky {
namespace agent {

class LocalStorage {
public:
    LocalStorage(const std::string& storage_path = "/var/lib/blinky/metrics",
                 size_t max_files = 100,
                 size_t max_file_size_mb = 10)
        : storage_path_(storage_path)
        , max_files_(max_files)
        , max_file_size_bytes_(max_file_size_mb * 1024 * 1024)
        , current_file_size_(0) {
        
        initialize_storage();
    }

    bool store(const metrics::SystemMetrics& metrics) {
        try {
            if (should_rotate()) {
                rotate_files();
            }

            std::string json = metrics.toJSON();
            std::string line = json + "\n";

            std::ofstream file(get_current_file_path(), std::ios::app);
            if (!file.is_open()) {
                return false;
            }

            file << line;
            current_file_size_ += line.size();
            file.close();

            return true;
        } catch (...) {
            return false;
        }
    }

    std::vector<metrics::SystemMetrics> get_latest(size_t count = 100) {
        std::vector<metrics::SystemMetrics> result;
        
        try {
            auto files = get_metric_files();
            
            for (auto it = files.rbegin(); it != files.rend() && result.size() < count; ++it) {
                auto metrics = read_file(*it, count - result.size());
                result.insert(result.end(), metrics.begin(), metrics.end());
            }
        } catch (...) {
            // Return empty vector on error
        }

        return result;
    }

    std::vector<metrics::SystemMetrics> get_range(time_t start_time, time_t end_time) {
        std::vector<metrics::SystemMetrics> result;
        
        try {
            auto files = get_metric_files();
            
            for (const auto& file : files) {
                auto metrics = read_file(file, 0);
                for (const auto& m : metrics) {
                    if (m.timestamp >= start_time && m.timestamp <= end_time) {
                        result.push_back(m);
                    }
                }
            }
        } catch (...) {
            // Return empty vector on error
        }

        return result;
    }

    std::string get_latest_json(size_t count = 1) {
        auto metrics = get_latest(count);
        if (metrics.empty()) {
            return "{}";
        }
        
        if (count == 1) {
            return metrics[0].toJSON();
        }

        std::string result = "[";
        for (size_t i = 0; i < metrics.size(); ++i) {
            if (i > 0) result += ",";
            result += metrics[i].toJSON();
        }
        result += "]";
        
        return result;
    }

    size_t get_total_metrics_count() {
        size_t count = 0;
        try {
            auto files = get_metric_files();
            for (const auto& file : files) {
                std::ifstream f(file);
                count += std::count(std::istreambuf_iterator<char>(f),
                                   std::istreambuf_iterator<char>(), '\n');
            }
        } catch (...) {
            // Return 0 on error
        }
        return count;
    }

    void cleanup_old_files() {
        try {
            auto files = get_metric_files();
            
            while (files.size() > max_files_) {
                std::filesystem::remove(files.front());
                files.erase(files.begin());
            }
        } catch (...) {
            // Ignore cleanup errors
        }
    }

    std::string get_storage_path() const {
        return storage_path_;
    }

private:
    std::string storage_path_;
    size_t max_files_;
    size_t max_file_size_bytes_;
    size_t current_file_size_;

    void initialize_storage() {
        try {
            std::filesystem::create_directories(storage_path_);
            
            auto current_file = get_current_file_path();
            if (std::filesystem::exists(current_file)) {
                current_file_size_ = std::filesystem::file_size(current_file);
            }
        } catch (...) {
            // Ignore initialization errors
        }
    }

    std::string get_current_file_path() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d", std::localtime(&time));
        
        return storage_path_ + "/metrics-" + std::string(buffer) + ".jsonl";
    }

    bool should_rotate() const {
        return current_file_size_ >= max_file_size_bytes_;
    }

    void rotate_files() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", std::localtime(&time));
        
        std::string new_file = storage_path_ + "/metrics-" + std::string(buffer) + ".jsonl";
        current_file_size_ = 0;
        
        cleanup_old_files();
    }

    std::vector<std::string> get_metric_files() const {
        std::vector<std::string> files;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(storage_path_)) {
                if (entry.is_regular_file() && 
                    entry.path().filename().string().find("metrics-") == 0) {
                    files.push_back(entry.path().string());
                }
            }
            
            std::sort(files.begin(), files.end());
        } catch (...) {
            // Return empty vector on error
        }

        return files;
    }

    std::vector<metrics::SystemMetrics> read_file(const std::string& file_path, size_t max_count) {
        std::vector<metrics::SystemMetrics> result;
        
        try {
            std::ifstream file(file_path);
            if (!file.is_open()) {
                return result;
            }

            std::string line;
            while (std::getline(file, line) && (max_count == 0 || result.size() < max_count)) {
                if (line.empty()) continue;
                
                try {
                    metrics::SystemMetrics m = metrics::SystemMetrics::fromJSON(line);
                    result.push_back(m);
                } catch (...) {
                    // Skip invalid JSON lines
                }
            }
        } catch (...) {
            // Return partial results on error
        }

        return result;
    }
};

}
}
