#include "uid/response_builders/hub_modes_get/response_builder_v3.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/resources/house_mode_resources.h"

#include "json_utils/json_utils.h"
#include "base_utils/timeutils.h"

namespace HubPlatform {
namespace Uid {
namespace ResponseBuilder {
namespace HubModesGet {
namespace V3 {

ResponseBuilder::ResponseBuilder( const rpc::Message& message, const std::string& api )
: V2::ResponseBuilder( message, api ) {}

JsonObject ResponseBuilder::createSwingerShutdownObject( IDbClient& dbClient ) {
    JsonObject result;

    JsonObject global;
    JsonArray devices;
    auto record = dbClient.findOne( HouseModesSwingerShutdown, JSON_EMPTY_OBJ, nullptr );
    json_get_obj( record, "defaults", global );
    result[ "global" ] = global;

    json_get_arr( record, "devices", devices );
    result[ "devices" ] = devices;

    return result;
}

JsonObject ResponseBuilder::getSystemTestInfo( IDbClient& dbClient ) {
    Json result = JSON_EMPTY_OBJ;
    auto record = dbClient.findOne( TempSystemWalkTestProgress, JSON_EMPTY_OBJ, nullptr );
    if( !record.empty() ) {
        std::string type = json_get_str( record, "type" );
        std::string operationId = json_get_str( record, "operationId" );
        int64_t startedAt = json_get_int( record, "startedAt" );
        int duration = json_get_int( record, "duration" );
        int timeIsLeft = duration - ( TimeUtils::currentSystemTime().count() - startedAt );
        result = JSON( "type" << type << "operationId" << operationId << "timesLeft" << timeIsLeft << "duration"
                              << duration << "startedAt" << startedAt );
    }

    return result;
}

JsonObject ResponseBuilder::buildResponse( IDbClient& dbClient ) const {
    auto joResponse = V2::ResponseBuilder::buildResponse( dbClient );
    auto alarmedObject = V2::ResponseBuilder::createAlarmedObject( dbClient );
    if( !alarmedObject.empty() ) {
        auto silent = json_get_bool( alarmedObject, "silent" );
        joResponse[ "result" ][ "alarmed" ][ "silent" ] = silent;
    }
    joResponse[ "result" ][ "entryDelay" ] = m_entryDelay;
    joResponse[ "result" ].erase( "alarmDelay" );
    joResponse[ "result" ][ "abortWindow" ] = m_abortWindow;

    auto swingerShutdownLimits = createSwingerShutdownObject( dbClient );
    if( !swingerShutdownLimits.empty() ) {
        joResponse[ "result" ][ "swingerShutdown" ] = swingerShutdownLimits;
    }
    auto runsOutTime = CoreDHouseModes::Resources::ExitTimeRunsOut;
    joResponse[ "result" ][ "switchToDelayRunsOut" ] = runsOutTime.count();

    Json systemTestInfo = getSystemTestInfo( dbClient );

    if( !systemTestInfo.empty() ) {
        joResponse[ "result" ][ "systemTest" ] = systemTestInfo;
    }

    return joResponse;
}

}  // namespace V3
}  // namespace HubModesGet
}  // namespace ResponseBuilder
}  // namespace Uid
}  // namespace HubPlatform