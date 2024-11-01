#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace Common {

struct MQTTMessage {
    std::string topic;
    std::vector< uint8_t > payload;
};

}  // namespace Common
}  // namespace HubPlatform
