#ifndef BLINKY_PROTOCOL_H
#define BLINKY_PROTOCOL_H

#include <string>
#include <cstdint>
#include <vector>

namespace blinky {
namespace protocol {

enum class MessageType : uint8_t {
    HEARTBEAT = 0x01,
    METRICS = 0x02,
    ALERT = 0x03,
    COMMAND = 0x04,
    RESPONSE = 0x05
};

struct Message {
    MessageType type;
    uint64_t timestamp;
    std::string hostname;
    std::string payload;
    
    std::string serialize() const;
    static Message deserialize(const std::string& data);
};

}
}

#endif
