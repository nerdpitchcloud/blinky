#ifndef BLINKY_VERSION_H
#define BLINKY_VERSION_H

#include <string>
#include <cstdint>

namespace blinky {
namespace version {

constexpr int MAJOR = 0;
constexpr int MINOR = 1;
constexpr int PATCH = 23;

constexpr const char* BUILD_DATE = __DATE__;
constexpr const char* BUILD_TIME = __TIME__;

struct Version {
    int major;
    int minor;
    int patch;
    std::string build_date;
    std::string git_commit;
    
    std::string toString() const;
    uint32_t toNumber() const;
    bool isCompatible(const Version& other) const;
    bool isNewer(const Version& other) const;
    
    static Version current();
    static Version fromString(const std::string& version_str);
};

std::string getVersionString();
std::string getFullVersionString();
uint32_t getVersionNumber();

}
}

#endif
