#include "group_db_reader.h"
#include "database_client/idbclient.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace CoreD {
namespace GroupOperations {

using namespace CoreD::Model::Resources::DBNames;

JsonArray DbReader::readDeviceGroupsFromDatabase( IDbClient& dbClient ) {
    return dbClient.findAll( DeviceGroupsDB, {}, nullptr );
}

JsonObject DbReader::fetchSingleDeviceGroup( IDbClient& dbClient, const JsonObject& condition ) {
    return dbClient.findOne( DeviceGroupsDB, condition, nullptr );
}

JsonArray DbReader::readItemGroupsFromDatabase( IDbClient& dbClient ) {
    return dbClient.findAll( ItemGroupsDB, {}, nullptr );
}

JsonObject DbReader::fetchSingleItemGroup( IDbClient& dbClient, const JsonObject& condition ) {
    return dbClient.findOne( ItemGroupsDB, condition, nullptr );
}

}  // namespace GroupOperations
}  // namespace CoreD
}  // namespace HubPlatform
