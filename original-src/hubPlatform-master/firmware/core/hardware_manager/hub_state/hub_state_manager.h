#pragma once

#include "hardware_manager/hub_state/model/hub_state.h"
#include "hardware_manager/hub_state/hub_state_indication_manager.h"

#include "networkd/remote_interface/network_remote_interface.h"
#include "cored/remote_interface/cored_interface.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace HardwareManager {

/*
    Types of indication:
        - Hub state. Background indication of current state of different firmware subsystems
            - Can be managed in this manager (e.g. Cloud Connection) or
            - Can be managed in corresponding services (e.g. Internet Availability).
              This approach is preferred.
        - Processes. Similar to hub state, but can have result: success, or failed.
          Have higher priority then hub state indication
        - Events. One-shot indication of some event (e.g. Zwave Identify).
          Have highest priority
        - As a special case there is also indication of pressed button pictures.
*/

class HubStateManager {
public:
    explicit HubStateManager( const Network::RemoteInterface& networkManager,
                              const CoreDRemoteInterface::CoreDInterface& cored,
                              IDbClient& dbClient,
                              HubStateIndicationManager& hubStateEffectManager );

    ~HubStateManager();

    // NOTE: As for now we do not have default effects for g150 leds,
    // need way to know when background effects for those leds should be started
    void onHubStateEffectStarted();

public:
    // States managed by this manager, TODO: use external states flow instead
    void setCloudConnection( HubState::CloudConnection cloudConnection );
    HubState::CloudConnection getCloudConnection() const;

    void setZwaveState( HubState::Zwave zwaveState );
    HubState::Zwave getZwaveState() const;

    void setBatteryState( HubState::Battery batteryState );
    HubState::Battery getBatteryState() const;

    // Handling external states (managed by appropriate services)
    void onInternetAvailabilityChanged( bool internetAvailable );

    // Handling events
    void onZwaveIdentify();

    // Handling user interaction
    void onProcessReadyToStart( HubState::Process process = HubState::Process::Unknown );
    void onProcessStart();

    // Handling processes
    void onProcessRunning( HubState::Process process );
    void onProcessStopped( HubState::Process process,
                           HubState::ProcessResult result = HubState::ProcessResult::Unspecified );

    void handleCustom( const Color color, BlinkingType type );

    bool isAnyProcessRunning() const;

private:
    void manageHubStateIndication();

private:
    const Network::RemoteInterface& m_networkManager;
    const CoreDRemoteInterface::CoreDInterface& m_cored;
    IDbClient& m_dbClient;
    HubStateIndicationManager& m_hubStateIndicationManager;

    std::string m_currentHubStateEffect;

    HubState::Process m_processReadyToRun;
    std::set< HubState::Process > m_runningProcesses;

    Color m_customEffectColor = Color::None;
    BlinkingType m_customEffectType = BlinkingType::None;

    // External states
    bool m_internetAvailable;
};

}  // namespace HardwareManager
}  // namespace HubPlatform
