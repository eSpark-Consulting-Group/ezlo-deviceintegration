#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {
namespace AlertNotificationPresenceRecovery {

enum class PresenceRecovery : uint8_t { Recovery = 0x00, Presence = 0x01, Reserved = 0x02 };

const char* toString( PresenceRecovery id );
PresenceRecovery AlertNotificationPresenceRecoverySourceFromString( const std::string& id );

}  // namespace AlertNotificationPresenceRecovery
}  // namespace Zigbee
}  // namespace HubPlatform
