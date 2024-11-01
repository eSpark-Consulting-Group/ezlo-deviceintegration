#pragma once

#include <string>

namespace HubPlatform {
namespace HardwareManager {
namespace HubState {

enum class Type {
    Unknown,
    CloudConnection,
    InternetAvailable,
    ZwaveState,
    BatteryState,
    End,
    Begin = Unknown,
    Default = Unknown
};

enum class CloudConnection {
    NotConnected,
    UpdatingControllerInformation,
    Connecting,
    Connected,
    End,
    Begin = NotConnected,
    Default = NotConnected
};

enum class Zwave { Off, On, Include, Exclude, End, Begin = Off, Default = Off };

enum class Battery {
    Unspecified,
    Ok,
    Discharging,
    DischargingLow,
    DischargingCritical,
    End,
    Begin = Unspecified,
    Default = Unspecified
};

// NOTE: if few processes are running simultaneously,
// the one that is specified in enum earlier will be indicated
enum class Process {
    Unknown,
    FirmwareUpdate,
    BootloaderUpdate,
    RecoveryPartitionUpdate,
    SwitchFirmwareToLuup,
    Reboot,
    FactoryReset,
    SoftReset,
    DeviceAdding,
    DeviceRemoving,
    Custom,
    End,
    Begin = Unknown,
    Default = Unknown
};
enum class ProcessResult { Unspecified, Success, Fail, End, Begin = Unspecified, Default = Unspecified };

std::string toString( Type type );
std::string toString( CloudConnection cloudConnection );
std::string toString( Zwave zwave );
std::string toString( Battery battery );
std::string toString( Process process );
std::string toString( ProcessResult processResult );

Type typeFromString( const std::string& value );
CloudConnection cloudConnectionFromString( const std::string& value );
Zwave zwaveStateFromString( const std::string& value );
Battery batteryStateFromString( const std::string& value );
Process processFromString( const std::string& value );
ProcessResult processResultFromString( const std::string& value );

}  // namespace HubState
}  // namespace HardwareManager
}  // namespace HubPlatform
