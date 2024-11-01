#include "zigbee/model/enums/attributes/door_state.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct DoorStateData {
    DoorState id;
    const char* name;
};

static const DoorStateData doorStateData[] = {
    { DoorState::Open, Resources::Fields::Attribute::Formats::DoorLockCluster::DoorState::Open },
    { DoorState::Closed, Resources::Fields::Attribute::Formats::DoorLockCluster::DoorState::Closed },
    { DoorState::ErrorJammed, Resources::Fields::Attribute::Formats::DoorLockCluster::DoorState::ErrorJammed },
    { DoorState::ErrorForcedOpen, Resources::Fields::Attribute::Formats::DoorLockCluster::DoorState::ErrorForcedOpen },
    { DoorState::ErrorUnspecified, Resources::Fields::Attribute::Formats::DoorLockCluster::DoorState::ErrorUnspecified }
};

const char* toString( DoorState id ) {
    for( const auto& data : doorStateData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Attribute::Formats::Unknown;
}

DoorState doorStateFromString( const std::string& name ) {
    for( const auto& data : doorStateData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return DoorState::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
