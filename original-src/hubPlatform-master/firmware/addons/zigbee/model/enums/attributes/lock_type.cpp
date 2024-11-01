#include "zigbee/model/enums/attributes/lock_type.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct LockTypeData {
    LockType id;
    const char* name;
};

static const LockTypeData lockTypeData[] = {
    { LockType::DeadBolt, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::DeadBolt },
    { LockType::Magnetic, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::Magnetic },
    { LockType::Other, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::Other },
    { LockType::Mortise, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::Mortise },
    { LockType::Rim, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::Rim },
    { LockType::LatchBolt, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::LatchBolt },
    { LockType::CylindricalLock, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::CylindricalLock },
    { LockType::TubularLock, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::TubularLock },
    { LockType::InterconnectedLock,
      Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::InterconnectedLock },
    { LockType::DeadLatch, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::DeadLatch },
    { LockType::DoorFurniture, Resources::Fields::Attribute::Formats::DoorLockCluster::LockType::DoorFurniture }
};

const char* toString( LockType id ) {
    for( const auto& data : lockTypeData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Lock Type" );

    return Resources::Fields::Attribute::Formats::Unknown;
}

LockType lockTypeFromString( const std::string& name ) {
    for( const auto& data : lockTypeData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Lock Type name" );

    return LockType::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
