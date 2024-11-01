#include "zigbee/model/enums/commands/ias_ace_arm_command_mode.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct IasAceArmCommandModeData {
    IasAceArmCommandMode id;
    const char* name;
};

static const IasAceArmCommandModeData iasAceArmCommandModeData[] = {
    { IasAceArmCommandMode::Disarm, Resources::Fields::Command::Formats::IasAce::ArmMode::Disarm },
    { IasAceArmCommandMode::ArmDayHomeZonesOnly,
      Resources::Fields::Command::Formats::IasAce::ArmMode::ArmDayHomeZonesOnly },
    { IasAceArmCommandMode::ArmNightSleepZonesOnly,
      Resources::Fields::Command::Formats::IasAce::ArmMode::ArmNightSleepZonesOnly },
    { IasAceArmCommandMode::ArmAllZones, Resources::Fields::Command::Formats::IasAce::ArmMode::ArmAllZones }
};

const char* toString( IasAceArmCommandMode id ) {
    for( const auto& data : iasAceArmCommandModeData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Ias Ace Arm command mode" );

    return Resources::Fields::Command::Formats::Unknown;
}

IasAceArmCommandMode iasAceArmCommandModeFromString( const std::string& name ) {
    for( const auto& data : iasAceArmCommandModeData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Ias Ace Arm command mode name" );

    return IasAceArmCommandMode::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
