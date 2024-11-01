#include "zigbee/model/enums/commands/ias_ace_arm_response.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct IasAceArmResponseData {
    IasAceArmResponse id;
    const char* name;
};

static const IasAceArmResponseData iasAceArmResponseData[] = {
    { IasAceArmResponse::AllZonesDisarmed, Resources::Fields::Command::Formats::IasAce::ArmResponse::AllZonesDisarmed },
    { IasAceArmResponse::OnlyDayHomeZonesArmed,
      Resources::Fields::Command::Formats::IasAce::ArmResponse::OnlyDayHomeZonesArmed },
    { IasAceArmResponse::OnlyNightSleepZonesArmed,
      Resources::Fields::Command::Formats::IasAce::ArmResponse::OnlyNightSleepZonesArmed },
    { IasAceArmResponse::AllZonesArmed, Resources::Fields::Command::Formats::IasAce::ArmResponse::AllZonesArmed },
    { IasAceArmResponse::InvalidArmDisarmCode,
      Resources::Fields::Command::Formats::IasAce::ArmResponse::InvalidArmDisarmCode },
    { IasAceArmResponse::NotReadyToArm, Resources::Fields::Command::Formats::IasAce::ArmResponse::NotReadyToArm },
    { IasAceArmResponse::AlreadyDisarmed, Resources::Fields::Command::Formats::IasAce::ArmResponse::AlreadyDisarmed }
};

const char* toString( IasAceArmResponse id ) {
    for( const auto& data : iasAceArmResponseData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Ias Ace Arm response" );

    return Resources::Fields::Command::Formats::Unknown;
}

IasAceArmResponse iasAceArmResponseFromString( const std::string& name ) {
    for( const auto& data : iasAceArmResponseData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Ias Ace Arm response name" );

    return IasAceArmResponse::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
