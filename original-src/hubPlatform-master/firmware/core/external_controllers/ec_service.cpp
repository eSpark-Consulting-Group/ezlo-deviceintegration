#include "external_controllers/ec_service.h"
#include "external_controllers/resources/ec_resources.h"

#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/groups_names.h"
#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "bus/messagesclasses.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace ExternalControllers {

namespace {
const auto DefaultReaskOnErrorDelay = std::chrono::minutes( 1 );

void copyIfExist( const Json& from, Json& to, const std::string& field ) {
    if( from.count( field ) != 0 )
        to[ field ] = from[ field ];
}
}  // namespace

Service::Service()
: Services::Service( GroupsNames::EXTERNAL_CONTROLLERS )
, m_icc( *this )
, m_reaskOnErrorDelay( DefaultReaskOnErrorDelay ) {}

Service::Service( const std::string& name,
                  std::unique_ptr< Services::BusWrapper > impl,
                  std::unique_ptr< IDbClient > dbClient )
: Services::Service( name, std::move( impl ), std::move( dbClient ) )
, m_icc( *this )
, m_reaskOnErrorDelay( DefaultReaskOnErrorDelay ) {}

void Service::setUp() {
    Services::Service::setUp();
    m_core = std::make_unique< Core >( *this, getDBClient() );
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    using MethodHandler = JsonObject ( Service::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { GLOBAL_LOCK, &Service::handleLockRequest },
        { GLOBAL_UNLOCK, &Service::handleUnlockRequest },
        { BACKUP_CREATE, &Service::handleBackupCreateRequest },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        return ( this->*handler )( request );
    }

    return Services::Service::handleRequestFromBus( request, requestType );
}

void Service::handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& /*sender*/ ) {
    using MethodHandler = void ( Service::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { EXTERNAL_CONTROLLERS_SUBSCRIBE, &Service::handleSubscribe },
        { EXTERNAL_CONTROLLERS_UNSUBSCRIBE, &Service::handleUnsubscribe },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( json );
    }
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleEventFromBus( message );
        break;
    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Service::handleEventFromBus( const Services::Message& message ) {
    switch( message.msgSubClass ) {
    case EXTERNAL_CONTROLLERS_BROADCAST:
        handleBroadcast( message.data );
        break;
    case EXTERNAL_CONTROLLERS_CONNECTED:
        handleControllerConnected( message.data );
        break;
    default:
        break;
    }
}

void Service::handleBroadcast( const JsonObject& data ) {
    auto uuid = json_get_str( data[ "sender" ], "uuid" );
    auto controllerId = m_icc.getControllerIdByUUID( uuid );
    if( controllerId.empty() ) {
        Log::error( cc::error( "Can not find controller " ) + cc::note( uuid ) + cc::error( " serial" ) );
        return;
    }

    auto subclass = json_get_str( data, "msg_subclass" );
    auto result = data[ "result" ];

    if( subclass == "hub.item.updated" ) {
        handleItemUpdatedBroadcast( controllerId, result );
    }
    else if( subclass == "hub.item.dictionary.updated" ) {
        handleItemDictionaryUpdatedBroadcast( controllerId, result );
    }
    else if( subclass == "hub.item.removed" ) {
        handleItemRemovedBroadcast( controllerId, result );
    }
    else if( subclass == "hub.device.updated" ) {
        handleDeviceUpdatedBroadcast( controllerId, result );
    }
    else if( subclass == "hub.device.removed" ) {
        handleDeviceRemovedBroadcast( controllerId, result );
    }
}

// json = {
//        "reason": "backup"
// }
JsonObject Service::handleLockRequest( const JsonObject& json ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.tryLock( json );
    return lockHelper.getResponse();
}

// json = {
//        "reason": "backup"
// }
JsonObject Service::handleUnlockRequest( const JsonObject& json ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.unlock( json );
    return lockHelper.getResponse();
}

JsonObject Service::handleBackupCreateRequest( const JsonObject& json ) {
    CommonUtils::BackupHelper helper( getDBClient(), json );
    helper.backupDatabase( Resources::DBTableNames::Subscriptions );
    // NOTE: Not backuping following databases (they contain cache of other controller's items and devices):
    //  db.firmware.external_controllers.items
    //  db.firmware.external_controllers.devices
    return helper.getResponse();
}

int Service::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    helper.restoreDatabase( Resources::DBTableNames::Subscriptions );
    return helper.getReturnCode();
}

void Service::handleItemUpdatedBroadcast( const std::string& controllerId, const JsonObject& result ) {
    auto itemId = result[ "_id" ];

    if( !m_core->itemSubscribed( controllerId, itemId ) ) {
        // Ignore broadcasts for unsubscribed items
        return;
    }

    if( !m_core->itemExists( controllerId, itemId ) ) {
        Log::warn( cc::warn( "Trying to update missing controller " ) + cc::warn( controllerId ) +
                   cc::warn( " item with id " ) + cc::note( itemId ) );
        requestItemsList( controllerId );
        return;
    }

    Json diff;
    copyIfExist( result, diff, "show" );
    copyIfExist( result, diff, "value" );
    copyIfExist( result, diff, "valueFormatted" );
    copyIfExist( result, diff, "maxValue" );
    copyIfExist( result, diff, "minValue" );
    copyIfExist( result, diff, "valueType" );

    m_core->updateItem( controllerId, itemId, diff );
}

void Service::handleItemDictionaryUpdatedBroadcast( const std::string& controllerId, const JsonObject& result ) {
    auto itemId = result[ "_id" ];

    auto item = m_core->getItem( controllerId, itemId );

    if( item.empty() ) {
        Log::warn( cc::warn( "Trying to update missing controller " ) + cc::warn( controllerId ) +
                   cc::warn( " item with id " ) + cc::note( itemId ) );
        requestItemsList( controllerId );
        return;
    }

    auto value = item[ "value" ];
    auto operation = result[ "operation" ];
    auto element = result[ "element" ];

    if( ( operation == "added" ) || ( operation == "updated" ) ) {
        for( auto it = element.begin(); it != element.end(); ++it ) {
            value[ it.key() ] = it.value()[ "value" ];
        }
    }
    else if( operation == "removed" ) {
        for( auto it = element.begin(); it != element.end(); ++it ) {
            json_remove( value, it.key() );
        }
    }

    Json diff;
    diff[ "value" ] = std::move( value );
    m_core->updateItem( controllerId, itemId, diff );
}

void Service::handleItemRemovedBroadcast( const std::string& controllerId, const JsonObject& result ) {
    auto itemId = json_get_str( result, "_id" );

    if( !m_core->itemSubscribed( controllerId, itemId ) ) {
        // Ignore broadcasts for unsubscribed items
        return;
    }

    if( !m_core->itemExists( controllerId, itemId ) ) {
        Log::warn( cc::warn( "Trying to remove missing controller " ) + cc::note( controllerId ) +
                   cc::debug( " item with id " ) + cc::note( itemId ) );
        requestItemsList( controllerId );
        return;
    }

    m_core->removeItem( controllerId, itemId );
}

void Service::handleDeviceUpdatedBroadcast( const std::string& controllerId, const JsonObject& result ) {
    auto deviceId = json_get_str( result, "_id" );

    if( !m_core->deviceSubscribed( controllerId, deviceId ) ) {
        // Ignore broadcasts for unsubscribed devices
        return;
    }

    if( !m_core->deviceExists( controllerId, deviceId ) ) {
        Log::warn( cc::warn( "Trying to update missing controller " ) + cc::warn( controllerId ) +
                   cc::warn( " device with id " ) + cc::note( deviceId ) );
        requestDevicesList( controllerId );
        return;
    }

    Json diff;
    copyIfExist( result, diff, "armed" );
    copyIfExist( result, diff, "name" );
    copyIfExist( result, diff, "reachable" );
    copyIfExist( result, diff, "roomId" );
    copyIfExist( result, diff, "ready" );
    copyIfExist( result, diff, "parentRoom" );

    m_core->updateDevice( controllerId, deviceId, diff );
}

void Service::handleDeviceRemovedBroadcast( const std::string& controllerId, const JsonObject& result ) {
    auto deviceId = json_get_str( result, "_id" );

    if( !m_core->deviceSubscribed( controllerId, deviceId ) ) {
        // Ignore broadcasts for unsubscribed devices
        return;
    }

    if( !m_core->deviceExists( controllerId, deviceId ) ) {
        Log::warn( cc::warn( "Trying to remove missing controller " ) + cc::note( controllerId ) +
                   cc::debug( " device with id " ) + cc::note( deviceId ) );
        requestDevicesList( controllerId );
        return;
    }

    m_core->removeDevice( controllerId, deviceId );
}

// data = {
//        "controllerId": "70010999"
//    }
void Service::handleControllerConnected( const JsonObject& data ) {
    auto controllerId = json_get_str( data, "controllerId" );
    Log::debug( cc::debug( "Detected connection to controller " ) + cc::note( controllerId ) );
    m_core->syncControllerData( controllerId );
}

// json = {
//     "controllerId": "70010999"
//     "itemsIds": [ "id1", "id2" ]
//     "devicesIds": [ "id1", "id2" ]
// }
void Service::handleSubscribe( const JsonObject& json ) {
    // TODO: maintain separate subscriptions for each service

    auto controllerId = json_get_str( json, "controllerId" );
    auto itemsIds = json_get_vec< std::string >( json, "itemsIds" );
    auto devicesIds = json_get_vec< std::string >( json, "devicesIds" );
    m_core->subscribe( controllerId, std::move( itemsIds ), std::move( devicesIds ) );
}

// json = {
//     "controllerId": "70010999"
//     "itemIds": [ "id1", "id2" ]
//     "deviceIds": [ "id1", "id2" ]
// }
void Service::handleUnsubscribe( const JsonObject& json ) {
    auto controllerId = json_get_str( json, "controllerId" );
    auto itemsIds = json_get_vec< std::string >( json, "itemsIds" );
    auto devicesIds = json_get_vec< std::string >( json, "devicesIds" );
    m_core->unsubscribe( controllerId, std::move( itemsIds ), std::move( devicesIds ) );
}

void Service::requestItemsList( const std::string& controllerId ) {
    auto& requestInfo = m_controllersRequests[ controllerId ];
    if( requestInfo.reaskItemsTask ) {
        requestInfo.reaskItemsTask->stop();
        requestInfo.reaskItemsTask.reset();
    }

    if( requestInfo.pendingItmesList ) {
        Log::debug( cc::debug( "Scheduling re-checking controller " ) + cc::note( controllerId ) +
                    cc::debug( " items list" ) );
        requestInfo.updateItemsList = true;
        return;
    }
    Log::info( cc::info( "Synchronizing controller " ) + cc::note( controllerId ) + cc::info( " items list" ) );
    requestInfo.updateItemsList = false;
    requestInfo.pendingItmesList = true;

    Json request;
    request[ "method" ] = "hub.items.list";
    request[ "params" ] = JSON_EMPTY_OBJ;

    m_icc.sendAsyncRequest( controllerId,
                            request,
                            std::bind( &Service::handleItemsListResponse, this, controllerId, std::placeholders::_1 ) );
}

// response = {
//     "result": {
//         "items": [
//             {
//                 "_id": "5cef9bf17f00007937ab8015",
//                 ...
//             }
//         ]
//     }
// }
void Service::handleItemsListResponse( const std::string& controllerId, const JsonObject& response ) {
    auto& requestInfo = m_controllersRequests[ controllerId ];
    requestInfo.pendingItmesList = false;

    auto reaskItemsListAfterError = [&]() {
        if( requestInfo.updateItemsList ) {
            requestItemsList( controllerId );
        }
        else if( !requestInfo.reaskItemsTask ) {
            requestInfo.reaskItemsTask = createPostponedTask(
                [this, controllerId]( Services::PostponedTask& ) { requestItemsList( controllerId ); } );
            requestInfo.reaskItemsTask->start( m_reaskOnErrorDelay, false );
        }
    };

    if( hasError( response ) ) {
        Log::error( cc::error( "Error getting controller " ) + cc::note( controllerId ) + cc::error( " items list: " ) +
                    cc::j( response[ "error" ] ) );
        reaskItemsListAfterError();
        return;
    }

    if( response.count( "result" ) == 0 ) {
        Log::error( cc::error( "Malformed controller " ) + cc::note( controllerId ) +
                    cc::error( " items list response" ) );
        reaskItemsListAfterError();
        return;
    }

    auto result = response[ "result" ];
    if( result.count( "items" ) == 0 ) {
        Log::error( cc::error( "Malformed controller " ) + cc::note( controllerId ) +
                    cc::error( " items list response result" ) );
        reaskItemsListAfterError();
        return;
    }

    JsonArray items;
    json_get_arr( result, "items", items );
    m_core->updateItemsList( controllerId, items );

    if( requestInfo.updateItemsList ) {
        requestItemsList( controllerId );
    }
}

void Service::requestDevicesList( const std::string& controllerId ) {
    auto& requestInfo = m_controllersRequests[ controllerId ];
    if( requestInfo.reaskDevicesTask ) {
        requestInfo.reaskDevicesTask->stop();
        requestInfo.reaskDevicesTask.reset();
    }

    if( requestInfo.pendingDevicesList ) {
        Log::debug( cc::debug( "Scheduling re-checking controller " ) + cc::note( controllerId ) +
                    cc::debug( " devices list" ) );
        requestInfo.updateDevicesList = true;
        return;
    }
    Log::info( cc::info( "Synchronizing controller " ) + cc::note( controllerId ) + cc::info( " devices list" ) );
    requestInfo.updateDevicesList = false;
    requestInfo.pendingDevicesList = true;

    Json request;
    request[ "method" ] = "hub.devices.list";
    request[ "params" ] = JSON_EMPTY_OBJ;

    m_icc.sendAsyncRequest(
        controllerId,
        request,
        std::bind( &Service::handleDevicesListResponse, this, controllerId, std::placeholders::_1 ) );
}

// response = {
//     "result": {
//         "devices": [
//             {
//                 "_id": "5cf0dc8c7f000068d223e889",
//                 ...
//             }
//         ]
//     }
// }
void Service::handleDevicesListResponse( const std::string& controllerId, const JsonObject& response ) {
    auto& requestInfo = m_controllersRequests[ controllerId ];
    requestInfo.pendingDevicesList = false;

    auto reaskDevicesListAfterError = [&]() {
        if( requestInfo.updateDevicesList ) {
            requestDevicesList( controllerId );
        }
        else if( !requestInfo.reaskDevicesTask ) {
            requestInfo.reaskDevicesTask = createPostponedTask(
                [this, controllerId]( Services::PostponedTask& ) { requestDevicesList( controllerId ); } );
            requestInfo.reaskDevicesTask->start( m_reaskOnErrorDelay, false );
        }
    };

    if( hasError( response ) ) {
        Log::error( cc::error( "Error getting controller " ) + cc::note( controllerId ) +
                    cc::error( " devices list: " ) + cc::j( response[ "error" ] ) );
        reaskDevicesListAfterError();
        return;
    }

    if( response.count( "result" ) == 0 ) {
        Log::error( cc::error( "Malformed controller " ) + cc::note( controllerId ) +
                    cc::error( " devices list response" ) );
        reaskDevicesListAfterError();
        return;
    }

    auto result = response[ "result" ];
    if( result.count( "devices" ) == 0 ) {
        Log::error( cc::error( "Malformed controller " ) + cc::note( controllerId ) +
                    cc::error( " devices list response result" ) );
        reaskDevicesListAfterError();
        return;
    }

    JsonArray devices;
    json_get_arr( result, "devices", devices );
    m_core->updateDevicesList( controllerId, devices );

    if( requestInfo.updateDevicesList ) {
        requestDevicesList( controllerId );
    }
}

void Service::sendInternalBroadcast( MessageSubClasses eventType, const JsonObject& broadcast ) {
    sendBroadcast( eventType, broadcast, GroupsNames::EXTERNAL_DEVICE_GROUP );
}

void Service::registerControllerConnection( const std::string& controllerId ) {
    m_icc.registerControllerConnection( controllerId );
}

void Service::unregisterControllerConnection( const std::string& controllerId ) {
    m_icc.unregisterControllerConnection( controllerId );
    m_controllersRequests.erase( controllerId );
}

}  // namespace ExternalControllers
}  // namespace HubPlatform
