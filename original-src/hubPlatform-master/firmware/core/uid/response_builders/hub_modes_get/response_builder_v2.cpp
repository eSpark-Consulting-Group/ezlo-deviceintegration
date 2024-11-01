
#include "uid/response_builders/hub_modes_get/response_builder_v2.h"
#include "cored/house_modes/house_mode_utils.h"
#include "json_utils/json_utils.h"
#include <algorithm>

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {
namespace V2 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message, const std::string& api )
: V1::ResponseBuilder( message, api ) {}

JsonArray ResponseBuilder::createAlarmedObject( IDbClient& dbClient ) const {
    const auto allArmedEvents = CoreDHouseModes::getHouseModesAlarmDevicesEvents( dbClient );
    JsonObject result;
    int timeIsLeft = 0;
    int entryDelay = 0;
    bool silent = false;
    JsonArrayBuilder sources;

    for( const auto event : allArmedEvents ) {
        int alarmDelay = json_get_int( event, "alarmDelay" );
        entryDelay = alarmDelay;
        silent = json_get_bool( event, "silent" );
        auto timestamp = json_get_number< std::int64_t >( event, "timestamp" );
        auto now =
            std::chrono::duration_cast< std::chrono::seconds >( std::chrono::system_clock::now().time_since_epoch() )
                .count();

        int left = alarmDelay - ( now - timestamp / 1000 );

        if( timeIsLeft > 0 ) {
            JsonObject res;
            timeIsLeft = std::min( left, timeIsLeft );
        }
        else {
            timeIsLeft = left;
        }

        if( timeIsLeft > 0 ) {
            JsonObject itemObject = event;
            itemObject.erase( "_id" );
            itemObject.erase( "alarmDelay" );
            itemObject[ "delay" ] = alarmDelay;
            JsonObject res;
            itemObject[ "deviceId" ] = json_get_str( event, "_id" );
            sources << itemObject;
        }
    }

    if( timeIsLeft > 0 ) {
        result[ "timeIsLeft" ] = timeIsLeft;
        result[ "type" ] = "global";
        result[ "entryDelay" ] = entryDelay;
        result[ "sources" ] = sources;
        result[ "silent" ] = silent;
    }

    return result;
}

JsonObject ResponseBuilder::buildResponse( IDbClient& dbClient ) const {
    auto joResponse = V1::ResponseBuilder::buildResponse( dbClient );

    joResponse[ "result" ][ "timeIsLeftToSwitch" ] = m_timeToLeftForSwitching;
    joResponse[ "result" ][ "alarmDelay" ] = m_alarmOnDelay;
    joResponse[ "result" ][ "switchToDelay" ] = m_modesSwitchDelay;

    for( auto& item : joResponse[ "result" ][ "modes" ] ) {
        item.erase( "switchToDelay" );
        item.erase( "alarmDelay" );
    }

    auto alarmedObject = createAlarmedObject( dbClient );
    if( !alarmedObject.empty() ) {
        if( alarmedObject.find( "silent" ) != alarmedObject.end() ) {
            alarmedObject.erase( "silent" );
        }
        joResponse[ "result" ][ "alarmed" ] = alarmedObject;
    }

    return joResponse;
}

}  // namespace V2
}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform