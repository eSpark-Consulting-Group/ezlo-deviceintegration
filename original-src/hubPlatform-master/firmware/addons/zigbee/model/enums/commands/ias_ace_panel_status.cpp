#include "zigbee/model/enums/commands/ias_ace_panel_status.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct IasAcePanelStatusData {
    IasAcePanelStatus id;
    const char* name;
};

static const IasAcePanelStatusData iasAcePanelStatusData[] = {
    { IasAcePanelStatus::PanelDisarmed, Resources::Fields::Command::Formats::IasAce::PanelStatus::PanelDisarmed },
    { IasAcePanelStatus::ArmedStay, Resources::Fields::Command::Formats::IasAce::PanelStatus::ArmedStay },
    { IasAcePanelStatus::ArmedNight, Resources::Fields::Command::Formats::IasAce::PanelStatus::ArmedNight },
    { IasAcePanelStatus::ArmedAway, Resources::Fields::Command::Formats::IasAce::PanelStatus::ArmedAway },
    { IasAcePanelStatus::ExitDelay, Resources::Fields::Command::Formats::IasAce::PanelStatus::ExitDelay },
    { IasAcePanelStatus::EntryDelay, Resources::Fields::Command::Formats::IasAce::PanelStatus::EntryDelay },
    { IasAcePanelStatus::NotReadyToArm, Resources::Fields::Command::Formats::IasAce::PanelStatus::NotReadyToArm },
    { IasAcePanelStatus::InAlarm, Resources::Fields::Command::Formats::IasAce::PanelStatus::InAlarm },
    { IasAcePanelStatus::ArmingStay, Resources::Fields::Command::Formats::IasAce::PanelStatus::ArmingStay },
    { IasAcePanelStatus::ArmingNight, Resources::Fields::Command::Formats::IasAce::PanelStatus::ArmingNight },
    { IasAcePanelStatus::ArmingAway, Resources::Fields::Command::Formats::IasAce::PanelStatus::ArmingAway }
};

const char* toString( IasAcePanelStatus id ) {
    for( const auto& data : iasAcePanelStatusData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Ias Ace Panel Status" );

    return Resources::Fields::Command::Formats::Unknown;
}

IasAcePanelStatus iasAcePanelStatusFromString( const std::string& name ) {
    for( const auto& data : iasAcePanelStatusData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Ias Ace Panel Status name" );

    return IasAcePanelStatus::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
