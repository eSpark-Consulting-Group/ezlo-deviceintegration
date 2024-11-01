#include "hardware_manager/remote_interface/hm_remote_interface.h"
#include "hardware_manager/hub_state/model/hub_state_utils.h"
#include "service_interfaces/service.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"
#ifdef __HUB_WITH_SPEAKER__
#include "hardware_manager/speaker/remote/speaker_remote_interface.h"
#endif

namespace HubPlatform {
namespace HardwareManager {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {
#ifdef __HUB_WITH_SPEAKER__
    m_speaker = std::make_unique< Speaker::SpeakerRemoteImpl >( m_hostService );
#endif
}

void RemoteInterface::hubPowerOff() {
    m_hostService.sendMethodCall( GroupsNames::HARDWARE_MANAGER, HUB_POWEROFF, JSON_EMPTY_OBJ );
}

std::unique_ptr< HardwareManager::BatteryInfo > RemoteInterface::getBatteryInfo() const {
    auto infoJson = m_hostService.getDBClient().findOne(
        TempBatteryInfoDBName, JSON( db::ID_FIELD_NAME << "notification" ), nullptr );
    if( infoJson.empty() ) {
        infoJson = m_hostService.getDBClient().findOne(
            BatteryInfoDBName, JSON( db::ID_FIELD_NAME << "notification" ), nullptr );
    }
    if( infoJson.empty() ) {
        return {};
    }

    return std::make_unique< HardwareManager::BatteryInfo >( BatteryInfo::fromJson( infoJson ) );
}

Speaker::SpeakerRemote* RemoteInterface::getSpeakerRemoteInterface() {
    return m_speaker.get();
}

void RemoteInterface::setCloudConnection( HubState::CloudConnection cloudConnection ) {
    m_hostService.sendMethodCall( GroupsNames::HARDWARE_MANAGER,
                                  HUB_STATE_SET,
                                  JSON( "type" << HubState::toString( HubState::Type::CloudConnection ) << "value"
                                               << HubState::toString( cloudConnection ) ) );
}

HubState::CloudConnection RemoteInterface::getCloudConnection() const {
    return HubStateUtils::getCloudConnection( m_hostService.getDBClient() );
}

void RemoteInterface::setZwaveState( HubState::Zwave zwaveState ) {
    m_hostService.sendMethodCall( GroupsNames::HARDWARE_MANAGER,
                                  HUB_STATE_SET,
                                  JSON( "type" << HubState::toString( HubState::Type::ZwaveState ) << "value"
                                               << HubState::toString( zwaveState ) ) );
}

HubState::Zwave RemoteInterface::getZwaveState() const {
    return HubStateUtils::getZwaveState( m_hostService.getDBClient() );
}

void RemoteInterface::onProcessRunning( HubState::Process process ) {
    m_hostService.sendMethodCall(
        GroupsNames::HARDWARE_MANAGER, HUB_PROCESS_RUNNING, JSON( "type" << HubState::toString( process ) ) );
}

void RemoteInterface::onProcessStopped( HubState::Process process, HubState::ProcessResult result ) {
    m_hostService.sendMethodCall(
        GroupsNames::HARDWARE_MANAGER,
        HUB_PROCESS_STOPPED,
        JSON( "type" << HubState::toString( process ) << "result" << HubState::toString( result ) ) );
}

}  // namespace HardwareManager
}  // namespace HubPlatform
