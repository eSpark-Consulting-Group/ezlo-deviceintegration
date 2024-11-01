#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class NodeType { Unknown = 0, Coordinator = 1, Router = 2, EndDevice = 3, SleepyEndDevice = 4 };

const char* toString( NodeType type );
NodeType nodeTypeFromString( const std::string& type );

}  // namespace Zigbee
}  // namespace HubPlatform
