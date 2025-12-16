#include "version.h"
#include <sstream>
#include <iomanip>

namespace blinky {
namespace version {

std::string Version::toString() const {
    std::ostringstream oss;
    oss << major << "." << minor << "." << patch;
    return oss.str();
}

uint32_t Version::toNumber() const {
    return (major << 16) | (minor << 8) | patch;
}

bool Version::isCompatible(const Version& other) const {
    return major == other.major;
}

bool Version::isNewer(const Version& other) const {
    return toNumber() > other.toNumber();
}

Version Version::current() {
    Version v;
    v.major = MAJOR;
    v.minor = MINOR;
    v.patch = PATCH;
    v.build_date = std::string(BUILD_DATE) + " " + std::string(BUILD_TIME);
    v.git_commit = "";
    return v;
}

Version Version::fromString(const std::string& version_str) {
    Version v;
    v.major = 0;
    v.minor = 0;
    v.patch = 0;
    
    std::istringstream iss(version_str);
    char dot;
    
    iss >> v.major;
    if (iss.peek() == '.') {
        iss >> dot >> v.minor;
        if (iss.peek() == '.') {
            iss >> dot >> v.patch;
        }
    }
    
    return v;
}

std::string getVersionString() {
    std::ostringstream oss;
    oss << MAJOR << "." << MINOR << "." << PATCH;
    return oss.str();
}

std::string getFullVersionString() {
    std::ostringstream oss;
    oss << "v" << MAJOR << "." << MINOR << "." << PATCH;
    oss << " (built " << BUILD_DATE << " " << BUILD_TIME << ")";
    return oss.str();
}

uint32_t getVersionNumber() {
    return (MAJOR << 16) | (MINOR << 8) | PATCH;
}

}
}
