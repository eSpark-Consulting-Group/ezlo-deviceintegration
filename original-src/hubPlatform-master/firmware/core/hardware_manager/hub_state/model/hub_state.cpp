#include "hardware_manager/hub_state/model/hub_state.h"
#include "base_utils/enums/enum_from_string.h"

namespace HubPlatform {
namespace HardwareManager {
namespace HubState {

std::string toString( Type type ) {
    switch( type ) {
    case Type::CloudConnection:
        return "cloud_connection";
    case Type::InternetAvailable:
        return "internet_available";
    case Type::ZwaveState:
        return "zwave_state";
    case Type::BatteryState:
        return "battery_state";
    case Type::Unknown:
    default:
        return "";
    }
}

std::string toString( CloudConnection cloudConnection ) {
    switch( cloudConnection ) {
    case CloudConnection::NotConnected:
        return "not_connected";
    case CloudConnection::UpdatingControllerInformation:
        return "updating_controller_information";
    case CloudConnection::Connecting:
        return "connecting";
    case CloudConnection::Connected:
        return "connected";
    default:
        return "";
    }
}

std::string toString( Zwave zwave ) {
    switch( zwave ) {
    case Zwave::Off:
        return "off";
    case Zwave::On:
        return "on";
    case Zwave::Include:
        return "include";
    case Zwave::Exclude:
        return "exclude";
    default:
        return "";
    }
}

std::string toString( Battery battery ) {
    switch( battery ) {
    case Battery::Unspecified:
        return "unspecified";
    case Battery::Ok:
        return "ok";
    case Battery::Discharging:
        return "discharging";
    case Battery::DischargingLow:
        return "discharging_low";
    case Battery::DischargingCritical:
        return "discharging_critical";
    default:
        return "";
    }
}

std::string toString( Process process ) {
    switch( process ) {
    case Process::FirmwareUpdate:
        return "firmware_update";
    case Process::BootloaderUpdate:
        return "bootloader_update";
    case Process::RecoveryPartitionUpdate:
        return "recovery_partition_update";
    case Process::SwitchFirmwareToLuup:
        return "switch_to_luup";
    case Process::Reboot:
        return "reboot";
    case Process::FactoryReset:
        return "factory_reset";
    case Process::SoftReset:
        return "soft_reset";
    case Process::DeviceAdding:
        return "device_adding";
    case Process::DeviceRemoving:
        return "device_removing";
    case Process::Custom:
        return "custom";
    default:
        return "";
    }
}

std::string toString( ProcessResult processResult ) {
    switch( processResult ) {
    case ProcessResult::Success:
        return "success";
    case ProcessResult::Fail:
        return "fail";
    case ProcessResult::Unspecified:
    default:
        return "";
    }
}

Type typeFromString( const std::string& value ) {
    return enumFromString< Type >( value );
}

CloudConnection cloudConnectionFromString( const std::string& value ) {
    return enumFromString< CloudConnection >( value );
}

Zwave zwaveStateFromString( const std::string& value ) {
    return enumFromString< Zwave >( value );
}

Battery batteryStateFromString( const std::string& value ) {
    return enumFromString< Battery >( value );
}

Process processFromString( const std::string& value ) {
    return enumFromString< Process >( value );
}

ProcessResult processResultFromString( const std::string& value ) {
    return enumFromString< ProcessResult >( value );
}

}  // namespace HubState
}  // namespace HardwareManager
}  // namespace HubPlatform
