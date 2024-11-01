#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class CommandType {
    Generated,
    Received,

    Unknown
};

const char* toString( CommandType type );
CommandType commandTypeFromString( const std::string& type );

}  // namespace Zigbee
}  // namespace HubPlatform
