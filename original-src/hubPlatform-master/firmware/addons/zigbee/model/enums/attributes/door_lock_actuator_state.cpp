#include "zigbee/model/enums/attributes/door_lock_actuator_state.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct DoorLockActuatorStateData {
    DoorLockActuatorState id;
    const char* name;
};

static const DoorLockActuatorStateData doorLockActuatorStateData[] = {
    { DoorLockActuatorState::Disabled,
      Resources::Fields::Attribute::Formats::DoorLockCluster::DoorLockActuatorState::Disabled },
    { DoorLockActuatorState::Enabled,
      Resources::Fields::Attribute::Formats::DoorLockCluster::DoorLockActuatorState::Enabled }

};

const char* toString( DoorLockActuatorState id ) {
    for( const auto& data : doorLockActuatorStateData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Door Lock Actuator State" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

DoorLockActuatorState doorLockActuatorStateFromString( const std::string& name ) {
    for( const auto& data : doorLockActuatorStateData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Door Lock Actuator State Name" );

    return DoorLockActuatorState::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
