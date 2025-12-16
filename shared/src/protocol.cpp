#include "protocol.h"
#include <sstream>
#include <ctime>

namespace blinky {
namespace protocol {

std::string Message::serialize() const {
    std::ostringstream oss;
    oss << static_cast<int>(type) << "|"
        << timestamp << "|"
        << hostname << "|"
        << payload;
    return oss.str();
}

Message Message::deserialize(const std::string& data) {
    Message msg;
    std::istringstream iss(data);
    std::string token;
    
    std::getline(iss, token, '|');
    msg.type = static_cast<MessageType>(std::stoi(token));
    
    std::getline(iss, token, '|');
    msg.timestamp = std::stoull(token);
    
    std::getline(iss, msg.hostname, '|');
    std::getline(iss, msg.payload);
    
    return msg;
}

}
}
