#include "get_alerts_responce_source.h"
#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {
namespace AlertNotificationPresenceRecovery {

struct AlertNotificationPresenceRecoverySourceData {
    PresenceRecovery id;
    const char* name;
};

static const struct AlertNotificationPresenceRecoverySourceData presenceRecovery[] = {
    { PresenceRecovery::Presence, Resources::Fields::Command::Formats::AlertStructure::PresenceRecovery::Presence },
    { PresenceRecovery::Recovery, Resources::Fields::Command::Formats::AlertStructure::PresenceRecovery::Recovery },
    { PresenceRecovery::Reserved, Resources::Fields::Command::Formats::AlertStructure::Reserved }
};

const char* toString( PresenceRecovery id ) {
    for( const auto& data : presenceRecovery ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Command::Formats::AlertStructure::Reserved;
}

PresenceRecovery AlertNotificationPresenceRecoverySourceFromString( const std::string& name ) {
    for( const auto& data : presenceRecovery ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return HubPlatform::Zigbee::AlertNotificationPresenceRecovery::PresenceRecovery::Reserved;
}

}  // namespace AlertNotificationPresenceRecovery
}  // namespace Zigbee
}  // namespace HubPlatform
