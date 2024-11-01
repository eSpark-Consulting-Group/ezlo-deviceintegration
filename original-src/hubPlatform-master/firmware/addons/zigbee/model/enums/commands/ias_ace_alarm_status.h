#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {

enum class IasAceAlarmStatus {
    NoAlarm = 0x00,
    Burglar = 0x01,
    Fire = 0x02,
    Emergency = 0x03,
    PolicePanic = 0x04,
    FirePanic = 0x05,
    EmergencyPanic = 0x06,

    Unknown = 0xff
};

const char* toString( IasAceAlarmStatus id );
IasAceAlarmStatus iasAceAlarmStatusFromString( const std::string& id );

}  // namespace Zigbee
}  // namespace HubPlatform
