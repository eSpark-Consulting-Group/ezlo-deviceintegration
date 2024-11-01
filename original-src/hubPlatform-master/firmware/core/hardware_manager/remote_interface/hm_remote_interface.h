#pragma once

#include "hardware_manager/utils/rgb.h"
#include "hardware_manager/speaker/remote/interface.h"
#include "hardware_manager/battery/battery_info.h"
#include "hardware_manager/hub_state/model/hub_state.h"

#include "json_utils/json_fwd.h"
#include <string>

namespace HubPlatform {
namespace Services {
class Service;
}
namespace HardwareManager {

enum class Priority : int {
    REALTIME = 0,
    HIGH,
    NORMAL,
    LOW,
    IDLE,
};

namespace EffectType {
constexpr char NotConnected[] = "notConnected";
constexpr char DefaultEffect[] = "defaultEffect";
constexpr char DefaultRecovery[] = "defaultRecoveryEffect";
constexpr char UpdateEzloFirmware[] = "updateEzloFirmware";
constexpr char LockEzlo[] = "lockEzlo";
constexpr char CriticalError[] = "criticalError";
constexpr char Ok[] = "ok";
constexpr char Fail[] = "fail";
constexpr char SelectEzlo[] = "selectEzlo";
constexpr char EzloFound[] = "ezloFound";
constexpr char WifiWps[] = "wifiWps";
constexpr char WifiConnecting[] = "wifiConnecting";
constexpr char ConfirmRecovery[] = "confirmRecovery";
constexpr char CustomBlink[] = "customBlink";
constexpr char ZwaveIdentify[] = "zwaveIdentify";
constexpr char FilesUploaded[] = "filesUploaded";
constexpr char HubReboot[] = "hubReboot";
constexpr char HubSoftReset[] = "hubSoftReset";
constexpr char HubFactoryReset[] = "hubFactoryReset";
constexpr char DeviceAdding[] = "deviceAdding";
constexpr char DeviceRemoving[] = "deviceRemoving";
constexpr char FirmwareLuupSwitch[] = "firmwareLuupSwitch";
constexpr char BatteryDischarging[] = "batteryDischarging";
constexpr char BatteryDischargingLow[] = "batteryDischargingLow";
constexpr char BatteryDischargingCritical[] = "batteryDischargingCritical";
constexpr char HubPowerOff[] = "hubPowerOff";  // Fast red blinking (four times per second)
constexpr char CustomEffect[] = "Custom";
}  // namespace EffectType

namespace Indicator {
constexpr char Ready[] = "ready";
constexpr char Internet[] = "internet";
constexpr char Error[] = "error";
constexpr char Message[] = "message";
constexpr char Zwave[] = "zwave";
}  // namespace Indicator

class RemoteInterface final {
public:
    RemoteInterface( Services::Service& hostService );

    void hubPowerOff();

    std::unique_ptr< HardwareManager::BatteryInfo > getBatteryInfo() const;

    Speaker::SpeakerRemote* getSpeakerRemoteInterface();

    void setCloudConnection( HubState::CloudConnection cloudConnection );
    HubState::CloudConnection getCloudConnection() const;

    void setZwaveState( HubState::Zwave zwaveState );
    HubState::Zwave getZwaveState() const;

    void onProcessRunning( HubState::Process process );
    void onProcessStopped( HubState::Process process,
                           HubState::ProcessResult result = HubState::ProcessResult::Unspecified );

private:
    Services::Service& m_hostService;
    Speaker::SpeakerRemoteUPtr m_speaker;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
