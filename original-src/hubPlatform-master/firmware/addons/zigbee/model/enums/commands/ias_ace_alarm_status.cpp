#include "zigbee/model/enums/commands/ias_ace_alarm_status.h"
#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

struct IasAceAlarmStatusData {
    IasAceAlarmStatus id;
    const char* name;
};

static const IasAceAlarmStatusData iasAceAlarmStatusData[] = {
    { IasAceAlarmStatus::NoAlarm, Resources::Fields::Command::Formats::IasAce::AlarmStatus::NoAlarm },
    { IasAceAlarmStatus::Burglar, Resources::Fields::Command::Formats::IasAce::AlarmStatus::Burglar },
    { IasAceAlarmStatus::Fire, Resources::Fields::Command::Formats::IasAce::AlarmStatus::Fire },
    { IasAceAlarmStatus::Emergency, Resources::Fields::Command::Formats::IasAce::AlarmStatus::Emergency },
    { IasAceAlarmStatus::PolicePanic, Resources::Fields::Command::Formats::IasAce::AlarmStatus::PolicePanic },
    { IasAceAlarmStatus::FirePanic, Resources::Fields::Command::Formats::IasAce::AlarmStatus::FirePanic },
    { IasAceAlarmStatus::EmergencyPanic, Resources::Fields::Command::Formats::IasAce::AlarmStatus::EmergencyPanic }
};

const char* toString( IasAceAlarmStatus id ) {
    for( const auto& data : iasAceAlarmStatusData ) {
        if( data.id == id ) {
            return data.name;
        }
    }

    assert( !"Unknown Ias Ace Alarm Status" );

    return Resources::Fields::Command::Formats::Unknown;
}

IasAceAlarmStatus iasAceAlarmStatusFromString( const std::string& name ) {
    for( const auto& data : iasAceAlarmStatusData ) {
        if( name == data.name ) {
            return data.id;
        }
    }

    assert( !"Unknown Ias Ace Alarm Status name" );

    return IasAceAlarmStatus::Unknown;
}

}  // namespace Zigbee
}  // namespace HubPlatform
