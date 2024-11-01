#include "zigbee/model/enums/attributes/lock_state.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct LockStateData {
    LockState id;
    const char* name;
};

static const LockStateData lockStateData[] = {
    { LockState::NotFullyLocked, Resources::Fields::Attribute::Formats::DoorLockCluster::LockState::NotFullyLocked },
    { LockState::Locked, Resources::Fields::Attribute::Formats::DoorLockCluster::LockState::Locked },
    { LockState::Unlocked, Resources::Fields::Attribute::Formats::DoorLockCluster::LockState::Unlocked },

};

const char* toString( LockState id ) {
    for( const auto& data : lockStateData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    return Resources::Fields::Attribute::Formats::Unknown;
}

LockState lockStateFromString( const std::string& name ) {
    for( const auto& data : lockStateData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    return LockState::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
