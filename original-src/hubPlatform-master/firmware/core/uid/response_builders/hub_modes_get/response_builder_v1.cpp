
#include "uid/response_builders/hub_modes_get/response_builder_v1.h"

#include "jsonrpc/jsonrpc.h"
#include "database_client/idbclient.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {
namespace V1 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message, const std::string& api )
: ResponseBuilderBase( message, api ) {}

void ResponseBuilder::setSettings( const std::string& currentHouseModeId,
                                   const std::string& nextHouseMode,
                                   int timeToLeftForSwitching,
                                   int modesSwitchDelay,
                                   int alarmOnDelay,
                                   Json entryDelay,
                                   Json abortWindow,
                                   AllSecurityDevices securityDevices,
                                   AllAlarmDevices alarmDevices,
                                   AllCameraDevices cameraDevices,
                                   AllProtectButtons protectButtons,
                                   AllProtectDevices protectDevices ) {
    m_currentHouseModeId = currentHouseModeId;
    m_nextHouseMode = nextHouseMode;

    m_timeToLeftForSwitching = timeToLeftForSwitching;
    m_modesSwitchDelay = modesSwitchDelay;
    m_alarmOnDelay = alarmOnDelay;
    m_entryDelay = entryDelay;
    m_abortWindow = abortWindow;
    m_securityDevices = std::move( securityDevices );
    m_alarmDevices = std::move( alarmDevices );
    m_cameraDevices = std::move( cameraDevices );
    m_protectButtons = std::move( protectButtons );
    m_protectDevices = std::move( protectDevices );
}

void ResponseBuilder::addHouseMode( const std::string& id,
                                    const std::string& name,
                                    const std::string& description,
                                    bool ignoreSwitchDelay,
                                    bool disarmedDefault,
                                    bool protect,
                                    bool armed,
                                    const Notifications& notifications,
                                    const DisarmedDevices& disarmedDevices,
                                    const AlarmsOffDevices& alarmsOffDevices,
                                    const CamerasOffDevices& camerasOffDevices,
                                    const BypassDevices& bypassDevices ) {
    JsonObject houseMode;

    houseMode[ "_id" ] = id;
    houseMode[ "name" ] = name;
    houseMode[ "description" ] = description;
    houseMode[ "ignoreSwitchDelay" ] = ignoreSwitchDelay;
    houseMode[ "disarmedDefault" ] = disarmedDefault;
    houseMode[ "protect" ] = protect;
    houseMode[ "armed" ] = armed;
    CoreDHouseModes::HouseModeEntry::notificationsToJson( houseMode[ "notifications" ], notifications );
    houseMode[ "disarmedDevices" ] = disarmedDevices;
    houseMode[ "alarmsOffDevices" ] = alarmsOffDevices;
    houseMode[ "camerasOffDevices" ] = camerasOffDevices;
    houseMode[ "bypassDevices" ] = bypassDevices;

    m_houseModes.emplace_back( std::move( houseMode ) );
}

JsonObject ResponseBuilder::buildResponse( IDbClient& dbClient ) const {
    JsonObject joResponse = ResponseBuilderBase::buildResponse();

    JsonObject result = JSON_EMPTY_OBJ;
    result[ "current" ] = m_currentHouseModeId;
    result[ "switchTo" ] = m_nextHouseMode;
    result[ "switchToDelay" ] = m_timeToLeftForSwitching;
    result[ "devices" ] = m_securityDevices;
    result[ "alarms" ] = m_alarmDevices;
    result[ "cameras" ] = m_cameraDevices;

    JsonArray buttonsResult = JSON_EMPTY_ARR;
    for( const auto& button : m_protectButtons ) {
        buttonsResult.push_back(
            JSON( "id" << button.deviceId() << "service" << CoreDHouseModes::toString( button.service() ) ) );
    }
    if( !buttonsResult.empty() ) {
        result[ "protectButtons" ] = buttonsResult;
    }
    result[ "protectDevices" ] = m_protectDevices;

    JsonArray modes;
    for( const auto& houseMode : m_houseModes ) {
        auto houseModeCopy = houseMode;
        if( json_get_bool( houseModeCopy, "ignoreSwitchDelay", false ) ) {
            houseModeCopy[ "switchToDelay" ] = 0;
        }
        else {
            houseModeCopy[ "switchToDelay" ] = m_modesSwitchDelay;
        }

        houseModeCopy.erase( "ignoreSwitchDelay" );
        houseModeCopy[ "alarmDelay" ] = m_alarmOnDelay;

        modes.push_back( std::move( houseModeCopy ) );
    }

    result[ "modes" ] = std::move( modes );

    joResponse[ "result" ] = std::move( result );

    return joResponse;
}

}  // namespace V1
}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform
