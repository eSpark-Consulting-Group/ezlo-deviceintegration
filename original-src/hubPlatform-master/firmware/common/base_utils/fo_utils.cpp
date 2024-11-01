
#include "base_utils/utils.h"

#include "base_utils/ezlodb.h"
#include "database_client/db_tables_names.h"
#include "device_model/h/permission.h"

#include "fo_utils.h"

bool FoUtils::checkIdIsItem( const std::string& objectId ) {
    size_t objectCount = Db::count( HaItemsDBName, JSON( "_id" << objectId ) );
    return ( objectCount > 0 );
}

bool FoUtils::getItemTrigger( const std::string& itemId,
                              std::string& dataitemId,
                              std::string& method,
                              int& instance,
                              const std::string& blockType ) {
    const JsonObject fieldsToReturn = JSON( "name" << 1 << "private.instanceId" << 1 );
    JsonObject item = Db::findOne( HaItemsDBName, Db::getIdObj( itemId ), &fieldsToReturn );
    if( item.empty() )
        return false;
    dataitemId = itemId;
    std::string prefix = ( blockType == "when" ) ? "get_" : "set_";
    method = prefix + json_get_str( item, "name" );
    JsonObject& privateSection = item[ "private" ];
    instance = json_get_int( privateSection, "instanceId" );
    return true;
}

bool FoUtils::getSubdeviceId( const std::string& itemId,
                              std::string& dataitemId,
                              std::string& method,
                              int& instance,
                              const std::string& blockType ) {
    const JsonObject fieldsToReturn = JSON( "deviceId" << 1 << "name" << 1 << "private.instanceId" << 1 );
    JsonObject item = Db::findOne( HaItemsDBName, Db::getIdObj( itemId ), &fieldsToReturn );
    if( item.empty() )
        return false;
    dataitemId = getDataItemId( json_get_str( item, "deviceId" ) );
    if( dataitemId.empty() )
        dataitemId = itemId;
    std::string prefix = ( blockType == "when" ) ? "get_" : "set_";
    method = prefix + json_get_str( item, "name" );
    JsonObject& privateSection = item[ "private" ];
    instance = json_get_int( privateSection, "instanceId" );
    return true;
}

std::string FoUtils::getDataItemId( const std::string& deviceId ) {
    const JsonObject fieldsToReturn = JSON( "private.dataitemId" );
    JsonObject device = Db::findOne( HaDevicesDBName, Db::getIdObj( deviceId ), &fieldsToReturn );
    JsonObject& privateSection = device[ "private" ];
    return json_get_str( privateSection, "dataitemId" );
}

int FoUtils::getNodeIdByDeviceId( const std::string& deviceId ) {
    const JsonObject fieldsToReturn = JSON( "private.nodeId" );
    JsonObject device = Db::findOne( HaDevicesDBName, Db::getIdObj( deviceId ), &fieldsToReturn );
    JsonObject& privateSection = device[ "private" ];
    return json_get_int( privateSection, "nodeId" );
}

std::string FoUtils::getDeviceIdByDataItemId( const std::string& dataitemId ) {
    const JsonObject fieldsToReturn = JSON_EMPTY_OBJ;
    return Db::getId( Db::findOne( HaDevicesDBName, JSON( "private.dataitemId" << dataitemId ), &fieldsToReturn ) );
}

std::string FoUtils::getDeviceId( const std::string& itemId ) {
    const JsonObject fieldsToReturn = JSON( "deviceId" << 1 );
    JsonObject item = Db::findOne( HaItemsDBName, Db::getIdObj( itemId ), &fieldsToReturn );
    return json_get_str( item, "deviceId" );
}

std::string FoUtils::getGatewayId( const std::string& deviceId ) {
    const JsonObject fieldsToReturn = JSON( "gatewayId" << 1 );
    JsonObject device = Db::findOne( HaDevicesDBName, Db::getIdObj( deviceId ), &fieldsToReturn );
    return json_get_str( device, "gatewayId" );
}

std::string FoUtils::getGatewayAddress( const std::string& gatewayId ) {
    const JsonObject fieldsToReturn = JSON( "private.adapterName" << 1 << "private.adapterId" << 1 );
    JsonObject gateway = Db::findOne( HaGatewaysDBName, Db::getIdObj( gatewayId ), &fieldsToReturn );
    JsonObject& privateSection = gateway[ "private" ];
    return ( json_get_str( privateSection, "adapterName" ) + ":" +
             std::to_string( json_get_int( privateSection, "adapterId" ) ) );
}

std::string FoUtils::getItemId( const std::string& dataitemId, const std::string& method, const int instance ) {
    const std::string& deviceId = getDeviceIdByDataItemId( dataitemId );
    const JsonObject fieldsToReturn = JSON_EMPTY_OBJ;
    return Db::getId(
        Db::findOne( HaItemsDBName,
                     JSON( "deviceId" << deviceId << "name" << method.substr( 4 ) << "private.instanceId" << instance ),
                     &fieldsToReturn ) );
}

bool FoUtils::getSubdevice( const std::string& itemId, int& nodeId, int& instanceId, std::string& method ) {
    const JsonObject fieldsToReturn = JSON( "name" << 1 << "private.nodeId" << 1 << "private.instanceId" << 1 );
    JsonObject item = Db::findOne( HaItemsDBName, Db::getIdObj( itemId ), &fieldsToReturn );
    if( item.empty() )
        return false;
    JsonObject& privateSection = item[ "private" ];
    nodeId = json_get_int( privateSection, "nodeId" );
    instanceId = json_get_int( privateSection, "instanceId" );
    method = json_get_str( item, "name" );
    return true;
}

JsonObject FoUtils::getItemById( std::string itemId ) {
    return Db::findOne( HaItemsDBName, JSON( "_id" << itemId ) );
}

JsonObject FoUtils::getDeviceById( std::string deviceId ) {
    return Db::findOne( HaDevicesDBName, JSON( "_id" << deviceId ) );
}

void FoUtils::assignUserWithPermission( const std::string& db_name,
                                        const std::string& object_id,
                                        const std::string& user,
                                        const std::string& permission ) {
    if( user.empty() || permission.empty() || db_name.empty() || object_id.empty() )
        return;
    std::string lowerUser = toLower( user );
    Json assignObj = JSON( "user" << lowerUser << "permission" << permission );
    if( Db::has( db_name, JSON( Db::getIdObj( object_id ) << "assigns.user" << lowerUser ) ) )
        Db::update( db_name,
                    JSON( Db::getIdObj( object_id ) << "assigns.user" << lowerUser ),
                    JSON( "$set" << JSON( "assigns.$" << assignObj ) ) );
    else
        Db::update( db_name,
                    Db::getIdObj( object_id ),
                    JSON( "$push" << JSON( "assigns" << assignObj ) ),
                    true );  // l_sergiy: TO-FIX: growing unlimited array?
}

void FoUtils::assignToDevices( const std::string& device_id,
                               const std::string& user,
                               const std::string& permission,
                               JsonObject& answer ) {
    if( Permission::getCode( permission ) == Permission::UNKNOWN ) {
        setError( answer, Errors::UNKNOWN_PERMISSION );
        return;
    }
    assignUserWithPermission( HaDevicesDBName, device_id, user, permission );
}
