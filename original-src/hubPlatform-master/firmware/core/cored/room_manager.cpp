#include "cored/room_manager.h"
#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/room_db_modifier.h"

#include "json_utils/json_wrapper.h"
#include "device_model/h/permission.h"

#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "base_utils/dblists.h"

#include "tiny/tiny_utils.h"  // tiny::tools::trim( std::string, ... )

namespace HubPlatform {
namespace CoreD {

RoomManager::RoomManager( CoreD::CoreService& hostService )
: m_coreService( hostService ) {}

JsonObject RoomManager::createRoom( const JsonObject& request ) {
    JsonObject response = JSON_EMPTY_OBJ;

    std::string name;
    if( !getNonEmptyField( request, "name", response, name, true ) ) {
        return response;
    }
    // Shrink name length to max limit.
    if( name.length() > NameMaxLength ) {
        name.resize( NameMaxLength );
    }

    const auto id = Db::genOid();
    auto joQuery = JSON( db::ID_FIELD_NAME << id << "name" << name );
    m_coreService.getRoomDbModifier().insert( joQuery );

    response = std::move( joQuery );
    return response;
}

JsonObject RoomManager::setRoomName( const JsonObject& request ) {
    auto response = JSON_EMPTY_OBJ;
    std::string id, name;
    if( !getField( request, db::ID_FIELD_NAME, response, id ) ||
        !getNonEmptyField( request, "name", response, name, true ) ) {
        return response;
    }

    try {
        m_coreService.getRoomDbModifier().update( id, JSON( "name" << name ) );
    }
    catch( const std::runtime_error& ) {
        setError( response, Errors::ROOM_NOT_FOUND );
    }

    return response;
}

JsonObject RoomManager::setRoomOrder( const JsonObject& request ) {
    JsonObject response = JSON_EMPTY_OBJ;

    JsonArray idList = JSON_EMPTY_ARR;
    if( !json_get_arr( request, "roomsId", idList ) ) {
        setError( response, Errors::BAD_PARAMS );
        return response;
    }

    JsonArrayBuilder jsonIdsArray;
    for( const JsonObject& idValue : idList )
        if( idValue.is_string() && !idValue.empty() )
            jsonIdsArray << json_get_str( idValue );

    JsonArray array = JSON( "roomsId" << jsonIdsArray.arr() );
    m_coreService.getDBClient().update(
        HaRoomsOrderDBName,
        JSON_EMPTY_OBJ,
        JSON( db::ID_FIELD_NAME << Model::Resources::RoomRecords::OrderRecordId << array ),
        true,
        false );
    m_coreService.sendBroadcast( HUB_ROOMS_REORDERED, array );

    return response;
}

void RoomManager::deleteRoom( const std::string& id ) {
    const JsonObject fieldsToReturn = JSON( "roomsId" << 1 );
    auto result = m_coreService.getDBClient().findOne( HaRoomsOrderDBName, JSON_EMPTY_OBJ, &fieldsToReturn );
    if( !result.empty() ) {
        std::set< std::string > set = result[ "roomsId" ];
        set.erase( id );
        m_coreService.getDBClient().update(
            HaRoomsOrderDBName,
            JSON_EMPTY_OBJ,
            JSON( db::ID_FIELD_NAME << Model::Resources::RoomRecords::OrderRecordId << JSON( "roomsId" << set ) ),
            false,
            false );
    }

    try {
        m_coreService.getRoomDbModifier().remove( id );
    }
    catch( const std::runtime_error& ) {
        // Ignore exception when record with id doesn't exist.
    }
}

std::pair< std::string, std::string > RoomManager::migrateVeraRooms(
    const Json& veraDataModel,
    std::unordered_map< int, std::string >& veraToEzloRoomMappings ) {
    m_coreService.getDBClient().clear( HaRoomsOrderDBName );

    auto roomsIt = veraDataModel.find( "rooms" );
    if( roomsIt != veraDataModel.end() ) {
        for( const auto& veraRoomInfo : *roomsIt ) {
            int veraRoomId;
            std::string veraRoomName;

            if( !json_get( veraRoomInfo, "id", veraRoomId ) || !json_get( veraRoomInfo, "name", veraRoomName ) ) {
                continue;
            }

            auto ezloRoomInfo = createRoom( JSON( "name" << veraRoomName ) );
            auto ezloRoomIdIt = ezloRoomInfo.find( db::ID_FIELD_NAME );
            if( ezloRoomIdIt != ezloRoomInfo.end() ) {
                veraToEzloRoomMappings.emplace( veraRoomId, ezloRoomIdIt->get< std::string >() );
            }
        }
    }

    return { "finished", "" };
}

}  // namespace CoreD
}  // namespace HubPlatform
