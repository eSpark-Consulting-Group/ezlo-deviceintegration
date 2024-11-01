#pragma once

#include "cored/group_operations/group.h"
#include "json_utils/json_utils.h"
#include "cored/remote_interface/cored_interface.h"
#include <string>

class IDbClient;

namespace HubPlatform {
namespace CoreD {

class GroupOperationsHandler {
public:
    using BroadcastCallback = std::function< void( MessageSubClasses messageSubClass, const JsonObject message ) >;

    explicit GroupOperationsHandler( IDbClient& dbClient,
                                     BroadcastCallback& sendBroadcast,
                                     CoreDRemoteInterface::CoreDInterface& core );
    JsonObject processCreateDeviceGroupRequest( const JsonObject& params );
    JsonObject processDeleteDeviceGroupRequest( const JsonObject& message );
    JsonObject processUpdateDeviceGroupRequest( const JsonObject& params );
    JsonObject processAddOrRemoveDevicesDeviceGroupRequest( const JsonObject& params, bool addNotRemove );
    JsonObject processDeviceGroupsListRequest( const JsonObject& message );
    JsonObject processDeviceGroupGetRequest( const JsonObject& params );
    JsonObject validateCreateGroupRequest( const JsonObject& message, bool checkRequiredParams );
    static JsonObject getDeviceGroupUpdateBroadcastData( const JsonObject& updatedData, const JsonObject& savedData );
    static JsonObject getItemGroupUpdateBroadcastData( const JsonObject& updatedData, const JsonObject& savedData );
    bool isExistDeviceGroupWithName( const std::string& name );
    static JsonObject validateItemGroupRequest( const JsonObject& message, bool checkRequiredParams );
    JsonObject processCreateItemGroupRequest( const JsonObject& params );
    JsonObject processDeleteItemGroupRequest( const JsonObject& params );
    JsonObject processUpdateItemGroupRequest( const JsonObject& params );

    JsonObject processItemGroupsListRequest( const JsonObject& params );
    JsonObject processItemGroupGetRequest( const JsonObject& params );
    JsonObject processDeviceItemGroupExpandRequest( const JsonObject& params );
    std::vector< JsonObject > expandDeviceGroup( const JsonObject& deviceGroup );
    std::vector< JsonObject > expandItemGroup( const JsonObject& itemGroup,
                                               const std::vector< JsonObject >& devicesArray );
    JsonObject processDeviceGroupFindRequest( const JsonObject& params );
    JsonObject processGroupOperationsFindRequest( const JsonObject& params, const char* dbName );

private:
    IDbClient& getDBClient();
    JsonObject processGroupOperationsListRequest( const JsonObject& params,
                                                  const char* dbName,
                                                  const char* groupName,
                                                  GroupInfo& groupInfo );
    JsonObject processGroupOperationsGetRequest( const JsonObject& params, const char* dbName, GroupInfo& groupInfo );

private:
    IDbClient& m_dbClient;
    BroadcastCallback m_sendBroadcast;
    CoreDRemoteInterface::CoreDInterface& m_core;
};

}  // namespace CoreD
}  // namespace HubPlatform
