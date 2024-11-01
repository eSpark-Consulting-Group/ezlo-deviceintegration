#include "external_controllers/remote_interface/ec_remote_interface.h"
#include "external_controllers/resources/ec_resources.h"
#include "external_controllers/common/ec_common_utils.h"

#include "uid/icc_interface/icc_remote_interface.h"
#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace ExternalControllers {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

void RemoteInterface::subscribeOnItemChanges( const RemoteInterface::ControllerID& controllerId,
                                              const RemoteInterface::ItemsIDs& itemsIds ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "controllerId" ] = controllerId;
    params[ "itemsIds" ] = itemsIds;
    m_hostService.sendMethodCall( GroupsNames::EXTERNAL_CONTROLLERS, EXTERNAL_CONTROLLERS_SUBSCRIBE, params );
}

void RemoteInterface::unsubscribeFromItemChanges( const RemoteInterface::ControllerID& controllerId,
                                                  const RemoteInterface::ItemsIDs& itemsIds ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "controllerId" ] = controllerId;
    params[ "itemsIds" ] = itemsIds;
    m_hostService.sendMethodCall( GroupsNames::EXTERNAL_CONTROLLERS, EXTERNAL_CONTROLLERS_UNSUBSCRIBE, params );
}

void RemoteInterface::subscribeOnDevicesChanges( const RemoteInterface::ControllerID& controllerId,
                                                 const RemoteInterface::DevicesIDs& devicesIds ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "controllerId" ] = controllerId;
    params[ "devicesIds" ] = devicesIds;
    m_hostService.sendMethodCall( GroupsNames::EXTERNAL_CONTROLLERS, EXTERNAL_CONTROLLERS_SUBSCRIBE, params );
}

void RemoteInterface::unsubscribeFromDevicesChanges( const RemoteInterface::ControllerID& controllerId,
                                                     const RemoteInterface::DevicesIDs& devicesIds ) {
    auto params = JSON_EMPTY_OBJ;
    params[ "controllerId" ] = controllerId;
    params[ "devicesIds" ] = devicesIds;
    m_hostService.sendMethodCall( GroupsNames::EXTERNAL_CONTROLLERS, EXTERNAL_CONTROLLERS_UNSUBSCRIBE, params );
}

JsonObject RemoteInterface::getItem( const RemoteInterface::ControllerID& controllerId,
                                     const RemoteInterface::ItemID& itemId ) const {
    using namespace Resources;
    auto item = m_hostService.getDBClient().findOne(
        DBTableNames::Items, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, itemId ) ), nullptr );

    if( !item.empty() ) {
        item[ db::ID_FIELD_NAME ] = itemId;
        item[ "controllerId" ] = controllerId;
    }

    return item;
}

RemoteInterface::RawItems RemoteInterface::getItems( const RemoteInterface::ControllerID& controllerId,
                                                     const RemoteInterface::ItemsIDs& itemsIds ) const {
    using namespace Resources;
    std::vector< std::string > ids;
    std::transform( itemsIds.begin(), itemsIds.end(), std::back_inserter( ids ), [&]( const std::string& itemId ) {
        return Utils::generateDbId( controllerId, itemId );
    } );
    auto items = m_hostService.getDBClient().findAll(
        DBTableNames::Items, JSON( db::ID_FIELD_NAME << JSON( "$in" << ids ) ), nullptr );

    for( auto& item : items ) {
        item[ db::ID_FIELD_NAME ] = Utils::extractId( item[ db::ID_FIELD_NAME ] );
        item[ "controllerId" ] = controllerId;
    }

    return items;
}

void RemoteInterface::setItemValue( const RemoteInterface::ControllerID& controllerId,
                                    const JsonObject& data,
                                    RemoteInterface::SendResponseCallback sendResponseCallback ) {
    Json request;
    request[ "method" ] = "hub.item.value.set";
    request[ "params" ] = data;

    InterControllerCommunication::RemoteInterface icc( m_hostService );
    icc.sendAsyncRequest(
        controllerId, request, [=]( const JsonObject& response ) { sendResponseCallback( response ); } );
}

RemoteInterface::RawItems RemoteInterface::getItemsBy( const ControllerID& controllerId,
                                                       const JsonObject& condition ) const {
    using namespace Resources;
    auto items = m_hostService.getDBClient().findAll( DBTableNames::Items, condition, nullptr );
    return items;
}

RemoteInterface::RawDevices RemoteInterface::getDevicesBy( const ControllerID& controllerId,
                                                           const JsonObject& condition ) const {
    using namespace Resources;
    auto devices = m_hostService.getDBClient().findAll( DBTableNames::Devices, condition, nullptr );
    return devices;
}

JsonObject RemoteInterface::getSingleDevice( const RemoteInterface::ControllerID& controllerId,
                                             const RemoteInterface::DeviceID& deviceId ) const {
    using namespace Resources;
    auto device = m_hostService.getDBClient().findOne(
        DBTableNames::Devices, JSON( db::ID_FIELD_NAME << Utils::generateDbId( controllerId, deviceId ) ), nullptr );

    if( !device.empty() ) {
        device[ db::ID_FIELD_NAME ] = deviceId;
        device[ "controllerId" ] = controllerId;
    }

    return device;
}

JsonObject RemoteInterface::setDeviceArmed( const RemoteInterface::ControllerID& controllerId,
                                            const RemoteInterface::DeviceID& deviceId,
                                            bool armed ) {
    Json request;
    request[ "method" ] = "hub.device.armed.set";
    Json& params = request[ "params" ];
    params[ "_id" ] = deviceId;
    params[ "armed" ] = armed;

    InterControllerCommunication::RemoteInterface icc( m_hostService );
    auto response = icc.sendRequest( controllerId, request );
    return response[ "result" ];
}

}  // namespace ExternalControllers
}  // namespace HubPlatform
