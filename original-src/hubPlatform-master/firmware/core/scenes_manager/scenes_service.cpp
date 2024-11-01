#include "scenes_manager/scenes_service.h"
#include "scenes_manager/scenes_processor/scene_request_handler_factory.h"
#include "scenes_manager/scenes_processor/actions/action_factory.h"

#include "scenes_manager/scenes_engine/helpers/clock.h"
#include "scenes_manager/scenes_engine/helpers/test_clock.h"
#include "scenes_manager/scenes_engine/actions/dispatcher.h"
#include "scenes_manager/scenes_engine/actions/action.h"
#include "scenes_manager/scenes_engine/actions/action_exception.h"
#include "scenes_manager/scenes_engine/expressions/expression_manager.h"
#include "scenes_manager/scenes_engine/item_cache/item_cache.h"
#include "scenes_manager/scenes_engine/scene_manager.h"
#include "scenes_manager/scenes_engine/states_registry.h"
#include "scenes_manager/scenes_engine/registry_flush_scheduler.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "scenes_manager/scenes_engine/advanced_scenes_version.h"
#include "cored/group_operations/device_item_group_manager.h"
#include "cored/group_operations/group_db_reader.h"
#include "cored/house_modes/resources/house_mode_resources.h"

#include "service_interfaces/groups_names.h"
#include "service_interfaces/service.h"
#include "service_interfaces/bus_wrapper.h"

#include "cored/model/resources/cored_resources.h"

#include "scenes_manager/scenes_engine/events/event_base.h"
#include "scenes_manager/scenes_engine/environment.h"

#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbid.h"
#include "bus/messagesclasses.h"
#include "jsonrpc/jsonrpc.h"
#include "bus/base_bus.h"

#include "networkd/remote_interface/network_remote_interface.h"
#include "uid/remote_interface/ui_remote_interface.h"
#include "cored/remote_interface/cored_interface_factory.h"

namespace HubPlatform {
namespace ScenesManager {

namespace {
const std::vector< std::string > BackupTables = {
    HaScenesBlocksDBName,    HaScenesDBName,      HaScenesExpressionsDBName,
    HaScenesItemCacheDBName, HaSceneStatesDBName, HaSceneBlockStatesDBName,
};

constexpr std::chrono::minutes StatesRegistryFlushInterval{ 30 };
}  // namespace

Service::Service()
: Services::Service( GroupsNames::SCENE_MANAGER ) {}

Service::Service( const std::string& name,
                  std::unique_ptr< Services::BusWrapper > impl,
                  std::unique_ptr< IDbClient > dbClient )
: Services::Service( name, std::move( impl ), std::move( dbClient ) ) {}

Service::~Service() = default;

void Service::setUp() {
    LOGGER_TRACE( cc::trace( "start" ) );
    Services::Service::setUp();
    LOGGER_INFO_RAW( cc::info( "Feature: advanced_scenes, version " ) + cc::note( FEATURE_ADVANCED_SCENES_VERSION ) );

    m_ownControllerId = HubInfoHelper::getPkDevice();

    ScenesRequestHandler::BroadcastCallback sendBroadcast = [this]( MessageSubClasses messageSubClass,
                                                                    const JsonObject& answer ) {
        MessageClasses messageClass = EVENT;
        LOGGER_TRACE_RAW( cc::trace( "sendBroadcast: send broadcast with subclass " +
                                     msgSubclassToStr( messageClass, messageSubClass ) + ": " + answer.dump() ) );
        Services::Message message = { GroupsNames::UI_GROUP, messageClass, messageSubClass, answer };
        send( message );
        LOGGER_TRACE_RAW( cc::trace( "sendBroadcast: broadcast is sent" ) );
    };
    auto sendSceneProgressBroadcast = [sendBroadcast]( const JsonObject& message ) {
        auto messageCopy = message;
        bool isGlobal = json_get_bool( messageCopy, "isGlobal" );
        messageCopy.erase( "isGlobal" );
        if( isGlobal ) {
            sendBroadcast( GLOBAL_SCENE_PROGRESS, messageCopy );
        }
        else {
            sendBroadcast( SCENE_PROGRESS, messageCopy );
        }
    };
    ScenesRequestHandlerInterface::AddTask addTaskMethod = [this]( std::function< void() > func ) {
        addTask( std::move( func ) );
    };

    m_core = CoreDRemoteInterface::makeInstance( *this );
    m_extensionsManager = std::make_unique< ExtensionsManager::RemoteInterface >( *this );
    m_extControllerInterface = std::make_unique< ExternalControllers::RemoteInterface >( *this );
    m_controllerFactory = std::make_unique< ControllerFactory >( *this, *m_extControllerInterface );

    m_itemCache = std::make_unique< SceneEngine::ItemCache >( *m_controllerFactory, getDBClient() );
    m_expressions = std::make_unique< SceneEngine::Expressions::ExpressionManager >(
        *m_core, getDBClient(), *m_itemCache, m_ownControllerId );
    m_deviceItemGroupManager = std::make_unique< CoreD::DeviceItemGroupManager >();

    m_itemCache->setUp();
    m_expressions->setUp();
    using namespace CoreD::GroupOperations;

    m_deviceItemGroupManager->setDeviceGroups( DbReader::readDeviceGroupsFromDatabase( getDBClient() ) );
    m_deviceItemGroupManager->setItemGroups( DbReader::readItemGroupsFromDatabase( getDBClient() ) );

    m_clock.reset( new SceneEngine::Clock );
    m_statesRegistry = std::make_unique< SceneEngine::StatesRegistry >(
        getDBClient(), [this]() { return m_clock->greenwichMeanTimePoint(); } );
    m_registryFlushScheduler = std::make_unique< SceneEngine::RegistryFlushScheduler >( *this, *m_statesRegistry );
    m_registryFlushScheduler->schedule( StatesRegistryFlushInterval );

    m_scenesManager = std::make_unique< SceneEngine::SceneManager >( *this,
                                                                     *m_core,
                                                                     *m_controllerFactory,
                                                                     sendSceneProgressBroadcast,
                                                                     *m_expressions,
                                                                     *m_itemCache,
                                                                     *m_deviceItemGroupManager,
                                                                     *m_statesRegistry,
                                                                     *m_clock );

    m_env = std::make_unique< SceneEngine::Environment >();
    m_scenesHandler = makeScenesRequestHandler( Services::Service::getDBClient(),
                                                m_ownControllerId,
                                                *m_scenesManager,
                                                *m_clock,
                                                *m_core,
                                                *m_extensionsManager,
                                                *m_expressions,
                                                *m_deviceItemGroupManager,
                                                std::move( sendBroadcast ),
                                                std::move( addTaskMethod ) );
    LOGGER_TRACE( cc::trace( "init variables" ) );

    join( GroupsNames::SCENE_MANAGER );
    join( GroupsNames::DEVICE_GROUP );  // needs for Items updating. See core_service ITEM_BROADCASTER
    join( GroupsNames::INTERNAL_BROADCAST_GROUP );
    join( GroupsNames::EXTERNAL_DEVICE_GROUP );
    // Needs for rebuilding scenes with the new current value
    // Generally I think it would be pretty to send the previous time value
    // The difference can be measured in the seconds. But changing locale are based in minutes at least
    join( GroupsNames::SYSTEM_GROUP );
    join( GroupsNames::CLOUD_GROUP );
    join( GroupsNames::UPDATE_MANAGER_BROADCASTS );
    join( GroupsNames::DEVICE_GROUP );
    join( GroupsNames::BACKUPD_BROADCASTS );
    LOGGER_TRACE( cc::trace( "finished joining" ) );

    {
        using namespace std::placeholders;
        auto actionRunHandler = std::bind( &Service::processActionRunRequest, this, _1, _2, _3 );
        SceneEngine::Dispatcher::instance().addHandler( actionRunHandler );
    }

    Network::RemoteInterface networkRemoteInterface( *this );
    auto interface = networkRemoteInterface.getActiveNetworkInterface();
    getSceneManager().processMainNetworkInterfaceChanged( json_get_str( interface, "_id" ), JSON_EMPTY_OBJ );

    Uid::RemoteInterface uiRemoteInterface( *this );
    getSceneManager().processCloudConnectionState( uiRemoteInterface.isRegisteredOnCloud(), JSON_EMPTY_OBJ );

    LOGGER_TRACE( cc::trace( "set up scenesHandler" ) );
    m_scenesHandler->setUp();
    LOGGER_TRACE( cc::trace( "end" ) );
}

void Service::tearDown() {
    LOGGER_TRACE( cc::trace( "started" ) );
    m_registryFlushScheduler->flushNow();
    m_scenesHandler->tearDown();
    SceneEngine::Dispatcher::instance().clear();

    m_expressions->tearDown();
    m_itemCache->tearDown();

    m_scenesManager->tearDown();

    Services::Service::tearDown();
    LOGGER_TRACE( cc::trace( "finished" ) );
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT: {
        handleEventMessage( message.msgClass, message.msgSubClass, message.data, sender );
    } break;
    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

JsonObject Service::createBroadcastMessage( const int msgClass, const int subClass, const JsonObject& data ) {
    JsonObject event;
    if( BaseBus::isBroadcastMessage( msgClass ) ) {
        json_add_str( event, "id", "ui_broadcast" );
    }

    event[ "msg_id" ] = db::DbId::genOID();
    event[ "msg_subclass" ] = msgSubclassToStr( UI_EVENT, subClass );
    event[ "result" ] = data;

    return event;
}

void Service::handleEventMessage( const int msgClass,
                                  const int subClass,
                                  const JsonObject& data,
                                  const std::string& sender ) {
    auto broadcast = createBroadcastMessage( msgClass, subClass, data );

    try {
        switch( subClass ) {
        case HUB_ITEM_UPDATED:
            handleItemUpdate( broadcast );
            break;
        case HOUSE_MODE_SWITCHED:
            handleHouseModeUpdate( broadcast );
            handleHouseModeSwitchToUpdate( broadcast );
            break;
        case HOUSE_MODE_ALARMED:
            handleHouseModePhaseUpdate( broadcast );
            break;
        case HUB_BATTERY_INFO_UPDATED:
            handleBatteryInfoUpdate( broadcast );
            break;
        case HUB_SYS_TIME_UPDATED:
            handleSystemTimeUpdate( data );
            break;
        case HUB_ROOM_DELETED:
            handleRoomRemoved( data );
            break;
        case HUB_DEVICE_UPDATED:
            handleDeviceUpdated( broadcast );
            break;
        case HUB_DEVICE_ADDED:
            m_controllerFactory->deviceAdded( data );
            break;
        case HUB_ITEM_ADDED:
            m_scenesHandler->itemAddedHandler( data );
            m_controllerFactory->itemAdded( data );
            break;
        case HUB_ITEM_REMOVED:
            m_scenesHandler->itemRemovedHandler( data );
            break;
        case HUB_ITEM_DICTIONARY_UPDATED:
            handleItemDictionaryUpdate( broadcast );
            break;
        case NETWORK_BROADCAST_IP_ADDRESS_CHANGED:
            handleMainNetworkInterface( broadcast );
            break;
        case HUB_NETWORK_CHANGED:
            handleNetworkChanged( broadcast );
            break;
        case CLOUD_REGISTERED:
            handleCloudConnectionState( true, broadcast );
            break;
        case CLOUD_UNREGISTERED:
            handleCloudConnectionState( false, broadcast );
            break;
        case PLANNING_SOFTWARE_UPDATE:
        case UPDATING_SOFTWARE:
            handleFirmwareUpdate( broadcast );
            break;
        case EXTENDED_HUB_ITEM_UPDATED:
            handleExtendedItemUpdate( broadcast );
            break;
        case HUB_SYSTEM_DATETIME_CHANGED:
            handleSystemDateTimeChanged( data );
            break;
        case HUB_DEVICE_REMOVED:
            handleDeviceRemoved( data );
            break;
        case DEVICE_GROUP_CREATED:
        case DEVICE_GROUP_UPDATED:
        case DEVICE_GROUP_DELETED:
            handleDeviceGroupChanged( data );
            break;
        case ITEM_GROUP_CREATED:
        case ITEM_GROUP_UPDATED:
        case ITEM_GROUP_DELETED:
            handleItemGroupChanged( data );
            break;
        case BACKUP_PROGRESS:
            m_scenesManager->handleHubBackupCreateProgress( data );
            break;
        case RESTORE_PROGRESS:
            m_scenesManager->handleHubBackupRestoreProgress( data );
            break;
        case HUB_FEATURE_UPDATED:
            handleHubFeatureUpdated( broadcast );

        default:
            break;
        }
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( "Event handler error: " ) + cc::note( CommonUtils::whatError( ex ) ) );
    }
}

void Service::handleItemUpdate( const JsonObject& message ) {
    using namespace CoreD::Model::Resources;
    using namespace SceneEngine;

    auto messageData = message[ "result" ];

    LOGGER_DEBUG( "message: " + cc::j( messageData ) );
    // Structure depends on the cored/handleItemUpdate()

    const auto itemId = json_get_str( messageData, ItemFields::IdFieldName );
    if( itemId.empty() ) {
        LOGGER_ERROR( cc::error( "empty item id" ) );
        return;
    }

    // check if item.name or item.value are changed; if so, notify expressions; call even with no value
    m_scenesHandler->itemUpdatedHandler( messageData );

    JsonObject value;
    if( !json_get( messageData, ItemFields::ValueFieldName, value ) ) {
        LOGGER_ERROR( cc::error( "empty item value" ) );
        return;
    }
    auto controllerId = getControllerId( messageData );
    Json extractedValue;
    if( !m_expressions->extractValueFromBroadcast( messageData, extractedValue ) )
        extractedValue = messageData;

    ItemValueWrapper itemValue( extractedValue );
    SceneEngine::ValueSourceKey searchKey = ValueSourceKey( controllerId, itemId );

    using namespace CoreD::GroupOperations;

    getSceneManager().processItemEvent( searchKey, itemValue, message );
    getSceneManager().processDeviceItemGroupEvent( messageData, itemValue );
    getSceneManager().processIsDeviceItemGroupEvent( messageData );
    const auto armed = json_get_int( messageData, "deviceArmed" );
    getSceneManager().processIsItemStateGroupEvent(
        messageData, itemValue, static_cast< ItemEvent::ArmedState >( armed ) );
}

void Service::handleExtendedItemUpdate( const JsonObject& message ) {
    LOGGER_DEBUG( cc::j( message ) );
    auto messageData = message[ "result" ];
    if( json_get_str( messageData, CoreD::Model::Resources::ItemFields::NameFieldName ) == "hotzones" ) {
        processHotzoneUpdate( message );
    }
    else {
        processGenericExtendedItemUpdate( messageData );
    }
}

void Service::processHotzoneUpdate( const JsonObject& message ) {
    using namespace CoreD::Model::Resources;
    auto messageData = message[ "result" ];
    const auto controllerId = getControllerId( messageData );
    const auto itemId = json_get_str( messageData, ItemFields::IdFieldName );
    Json itemValue = JSON_EMPTY_OBJ;
    json_get( messageData, ItemFields::ValueFieldName, itemValue );

    getSceneManager().processIsDetectedInHotzoneEvent( { controllerId, itemId }, itemValue, message );
}

void Service::processGenericExtendedItemUpdate( const JsonObject& message ) {
    using namespace CoreD::Model::Resources;

    auto controllerId = getControllerId( message );
    auto itemId = json_get_str( message, ItemFields::IdFieldName );

    Json newValue = JSON_EMPTY_OBJ;
    Json oldValue = JSON_EMPTY_OBJ;
    json_get( message, ItemFields::ValueFieldName, newValue );
    json_get( message, ItemFields::OldValueFieldName, oldValue );

    getSceneManager().processExtendedItemEvent(
        { std::move( controllerId ), std::move( itemId ) }, oldValue, newValue );
}

void Service::handleItemDictionaryUpdate( const JsonObject& message ) {
    auto messageData = message[ "result" ];
    LOGGER_DEBUG( cc::j( messageData ) );
    using namespace CoreD::Model::Resources;
    const auto itemId = json_get_str( messageData, ItemFields::IdFieldName );
    const auto operation = json_get_str( messageData, DictionaryEditRequestFields::OperationFieldName );

    auto element = getDictionaryNewElement( messageData );
    auto controllerId = getControllerId( messageData );

    getSceneManager().processItemDictionaryEvent( { controllerId, itemId }, operation, element.value, message );
    getSceneManager().processIsDictionaryChangedEvent( { controllerId, itemId }, operation, element.key, message );
    getSceneManager().processIsDeviceItemGroupEvent( messageData );
}

void Service::handleBatteryInfoUpdate( const JsonObject& message ) {
    std::string status;
    int level;
    auto messageData = message[ "result" ];
    if( json_get( messageData, "status", status ) ) {
        getSceneManager().processBatteryStateEvent( status, message );
    }
    else {
        LOGGER_ERROR( cc::error( "no 'status' field in broadcast" ) );
    }
    if( json_get( messageData, "stateOfCharge", level ) ) {
        getSceneManager().processBatteryLevelEvent( level, message );
    }
    else {
        LOGGER_ERROR( cc::error( "no 'stateOfCharge' field in broadcast" ) );
    }
}

void Service::handleFirmwareUpdate( const JsonObject& message ) {
    std::string status;
    auto messageData = message[ "result" ];
    if( json_get( messageData, "status", status ) ) {
        getSceneManager().processFirmwareUpdateStateEvent( status, message );
    }
    else {
        LOGGER_ERROR( cc::error( "no 'status' field in broadcast" ) );
    }
}

void Service::handleSystemDateTimeChanged( const JsonObject& message ) {
    LOGGER_DEBUG_RAW( cc::debug( "System date/time has been reset: " ) + cc::j( message ) );
    getSceneManager().onSystemTimeChanged();
}

Service::DictionaryElement Service::getDictionaryNewElement( const Json& message ) const {
    using namespace CoreD::Model::Resources;
    return getDictionaryElement( DictionaryEditRequestFields::ElementFieldName, message );
}

Service::DictionaryElement Service::getDictionaryOldElement( const Json& message ) const {
    using namespace CoreD::Model::Resources;
    return getDictionaryElement( DictionaryEditRequestFields::OldElementName, message );
}

Service::DictionaryElement Service::getDictionaryElement( const std::string& field, const Json& message ) const {
    using namespace CoreD::Model::Resources;

    JsonObject element;

    if( !json_get( message, field, element ) ) {
        auto error = "No " + field + " field in broadcast";
        LOGGER_ERROR( cc::error( error ) );
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, error );
    }

    if( element.empty() || !element.is_object() ) {
        auto error = "No " + field + " field in broadcast";
        LOGGER_ERROR( cc::error( error ) );
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, error );
    }

    if( !element.is_object() ) {
        auto error = "Element is not object in field: " + field;
        LOGGER_ERROR( cc::error( error ) );
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, error );
    }

    std::string key = element.begin().key();
    Json value = element.begin().value();

    if( value.empty() && !value.is_object() ) {
        auto error = "Value is not object in field: " + field;
        LOGGER_ERROR( cc::error( error ) );
        throw SceneEngine::SceneException( Errors::WRONG_FIELD, error );
    }

    value = value.begin().value();

    return { key, value };
}

void Service::handleHouseModeUpdate( const JsonObject& message ) {
    auto messageData = message[ "result" ];
    auto status = json_get_str( messageData, "status" );
    if( status != "done" ) {
        return;
    }
    auto controllerId = getControllerId( messageData );
    std::string from;
    if( json_get( messageData, "from", from ) ) {
        getSceneManager().processHouseModeChangedFromEvent( controllerId, from, message );
    }
    else {
        LOGGER_ERROR( cc::error( "No 'from' field: " ) + cc::j( messageData ) );
    }

    std::string to;
    if( json_get( messageData, "to", to ) ) {
        getSceneManager().processHouseModeChangedToEvent( controllerId, to, message );
    }
    else {
        LOGGER_ERROR( cc::error( "No 'to' field: " ) + cc::j( messageData ) );
    }
}

void Service::handleHouseModeSwitchToUpdate( const JsonObject& message ) {
    using namespace CoreDHouseModes::Resources::BroadcastFields;
    auto messageData = message[ "result" ];
    auto status = json_get_str( messageData, DeviceStatusFieldName );
    auto controllerId = getControllerId( messageData );
    getSceneManager().processHouseSwitchToRangeEvent( controllerId, "switchTo", status, message );
};

void Service::handleHouseModePhaseUpdate( const JsonObject& message ) {
    using namespace CoreDHouseModes::Resources::BroadcastFields;
    auto messageData = message[ "result" ];
    auto phase = json_get_str( messageData, DevicePhaseFieldName );
    auto status = json_get_str( messageData, DeviceStatusFieldName );

    auto controllerId = getControllerId( messageData );
    getSceneManager().processHouseAlarmRangeEvent( controllerId, phase, status, message );
}

void Service::handleHubFeatureUpdated( const JsonObject& message ) {
    auto messageData = message[ "result" ];
    bool statusBool = json_get_bool( messageData, "status" );
    auto name = json_get_str( messageData, "_id" );

    auto controllerId = getControllerId( messageData );
    getSceneManager().processFeatureStateEvent( controllerId, name, statusBool, message );
}

void Service::handleSystemTimeUpdate( const JsonObject& ) {
    // TODO: add implementation
}

void Service::handleDeviceUpdated( const JsonObject& message ) {
    auto messageData = message[ "result" ];
    LOGGER_TRACE( cc::j( messageData ) );
    const auto deviceId = json_get_str( messageData, db::ID_FIELD_NAME );
    if( deviceId.empty() ) {
        LOGGER_DEBUG( "empty device id" );
        return;
    }
    using namespace CoreD::GroupOperations;
    // check if item.name or item.value are changed; if so, notify expressions; call even with no value
    m_scenesHandler->deviceUpdatedHandler( messageData );
    auto controllerId = getControllerId( messageData );
    getSceneManager().processDeviceStateEvent(
        SceneEngine::ValueSourceKey( controllerId, deviceId ), messageData, message );

    using namespace CoreD::Model::Resources;
    auto device = m_core->getSingleDevice( deviceId );
    std::string category = json_get_str( device, DeviceFields::CategoryFieldName );
    std::string subCategory = json_get_str( device, DeviceFields::SubcategoryFieldName );

    getSceneManager().processDeviceGroupStateEvent(
        SceneEngine::ValueSourceKey( controllerId, deviceId ), messageData, deviceId, category, subCategory, message );

    bool armed;
    if( !json_get( messageData, "armed", armed ) ) {
        LOGGER_DEBUG( "empty device armed" );
        return;
    }

    getSceneManager().processDeviceArmed( controllerId, deviceId, armed, message );
}

void Service::handleDeviceRemoved( const JsonObject& message ) {
    LOGGER_TRACE( cc::j( message ) );
    m_scenesHandler->deviceRemovedHandler( message );
}

void Service::handleDeviceGroupChanged( const JsonObject& message ) {
    LOGGER_TRACE( cc::j( message ) );
    using namespace CoreD::GroupOperations;
    m_deviceItemGroupManager->setDeviceGroups( DbReader::readDeviceGroupsFromDatabase( getDBClient() ) );
}

void Service::handleItemGroupChanged( const JsonObject& message ) {
    LOGGER_TRACE( cc::j( message ) );
    using namespace CoreD::GroupOperations;
    m_deviceItemGroupManager->setItemGroups( DbReader::readItemGroupsFromDatabase( getDBClient() ) );
}

void Service::handleMainNetworkInterface( const JsonObject& message ) {
    auto messageData = message[ "result" ];
    getSceneManager().processMainNetworkInterfaceChanged( json_get_str( messageData, "interfaceId" ), message );
}

void Service::handleNetworkChanged( const JsonObject& message ) {
    auto messageData = message[ "result" ];
    auto it = messageData.find( "interfaces" );
    if( it == messageData.end() ) {
        LOGGER_ERROR( cc::error( "interfaces field is not found" ) );
        return;
    }
    for( const auto& interface : *it ) {
        auto id = json_get_str( interface, "_id" );
        auto internetAvailable = json_get_bool( interface, "internetAvailable" );
        getSceneManager().processNetworkInterfaceStateChanged( id, internetAvailable, message );
    }
}

void Service::handleCloudConnectionState( bool isConnected, const JsonObject& message ) {
    getSceneManager().processCloudConnectionState( isConnected, message );
}

Service::ControllerId Service::getControllerId( const Json& obj ) {
    auto controllerId = json_get_str( obj, "controllerId" );
    if( controllerId.empty() ) {
        controllerId = m_ownControllerId;
    }
    return controllerId;
}

SceneEngine::SceneManager& Service::getSceneManager() {
    return *m_scenesManager;
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    LOGGER_DEBUG_RAW( std::string( __FUNCTION__ ) + ", request :" + request.dump() );

    try {
        switch( requestType ) {
        case SCENE_CREATE:
            return handleCreateScene( request );
        case SCENE_EDIT:
            return handleEditScene( request );
        case SCENE_DELETE:
            return handleDeleteScene( request );
        case SCENE_DELETE_ALL:
            return handleDeleteAllScenes( request );
        case SCENE_ENABLE:
            return handleSwitchScene( request );
        case SCENE_CLONE:
            return handleCloneScene( request );
        case SCENE_RUN:
            return handleRunScene( request );
        case SCENE_STOP:
            return handleStopScene( request );
        case SCENE_SET_TEST_CLOCK:
            return handleSetTestClock( request );
        case SCENE_ROOM_SET:
            return handleRoomSet( request );
        case SCENE_ICON_SET:
            return m_scenesHandler->processSetIcon( request );
        case SCENE_NOTIFICATION_ADD:
            return handleNotificationAdd( request );
        case SCENE_NOTIFICATION_REMOVE:
            return handleNotificationRemove( request );
        case SCENE_TIME_LIST:
            return handleTimeList( request );
        case SCENE_SET_BLOCKS:
            return m_scenesHandler->processSetBlocks( request );
        case SCENE_REMOVE_BLOCKS:
            return m_scenesHandler->processRemoveBlocks( request );
        case SCENE_BLOCKS_LIST:
            return m_scenesHandler->processGetBlocks( request );
        case SCENE_BLOCKS_DATA_LIST:
            return m_scenesHandler->processGetBlocksDataList( request );
        case SCENE_MODIFY_SCENES:
            return m_scenesHandler->modifyScenes( request );
        case SCENE_EXPRESSION_VALUE_GET:
            return m_scenesHandler->getExpressionValue( request );
        case SCENE_NAME_SET:
            return m_scenesHandler->setSceneName( request );
        case SCENE_META_SET:
            return m_scenesHandler->setSceneMeta( request );
        case SCENE_BLOCK_META_SET:
            return m_scenesHandler->setSceneBlockMeta( request );
        case SCENE_HOUSE_MODES_SET:
            return m_scenesHandler->setHouseModes( request );
        case SCENE_BLOCK_ENABLED_SET:
            return m_scenesHandler->setBlockEnabled( request );
        case SCENE_BLOCK_STATUS_RESET:
            return m_scenesHandler->processBlockStatusReset( request );
        case SCENE_STATUS_GET:
            return m_scenesHandler->processScenesStatusGet( request );
        case SCENE_EXPRESSIONS_SET:
            return m_scenesHandler->expressionSet( request );
        case SCENE_EXPRESSIONS_DELETE:
            return m_scenesHandler->expressionDelete( request );
        case SCENE_EXPRESSIONS_LIST:
            return m_scenesHandler->expressionList( request );
        case GLOBAL_LOCK:
            return handleLockRequest( request );
        case GLOBAL_UNLOCK:
            return handleUnlockRequest( request );
        case BACKUP_CREATE:
            return handleBackupCreateRequest( request );
        case CLOUD_MESSAGE:
            return unhandledRequestAnswer();
        default:
            return Services::Service::handleRequestFromBus( request, requestType );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to handleRequestFromBus: " + CommonUtils::whatError( e ) ) );
    }
    LOGGER_TRACE( "end" );
    return JSON_EMPTY_OBJ;
}

bool Service::handleAsyncRequestFromBus( const JsonObject& request,
                                         MessageSubClasses requestType,
                                         const std::string& uuid,
                                         const std::string& sender ) {
    try {
        switch( requestType ) {
        case SCENE_ACTION_BLOCK_TEST: {
            m_scenesHandler->actionBlockTest(
                request, *this, *m_controllerFactory, [this, sender, uuid]( const Json& response ) {
                    sendAsyncResponse( sender, response, uuid );
                } );
            break;
        }
        case BACKUP_RESTORE_VERA: {
            m_veraBackupRequestInfo = std::make_unique< AsyncRequestInfo >( request, uuid, sender );

            // TODO implement vera backup flow
            auto dummyTask = createPostponedTask( [this]( Services::PostponedTask& ) {
                if( !m_veraBackupRequestInfo ) {
                    LOGGER_DEBUG_RAW( "No vera backup request info" );
                    return;
                }
                auto response = JSON( "status"
                                      << "finished" );
                sendAsyncResponse(
                    m_veraBackupRequestInfo->m_sender, std::move( response ), m_veraBackupRequestInfo->m_uuid );
                m_veraBackupRequestInfo.reset();
            } );
            dummyTask->start( std::chrono::seconds( 3 ), false );
            break;
        }
        default:
            return false;
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Unable to handleAsyncRequestFromBus: " + CommonUtils::whatError( e ) ) );
    }

    return true;
}

JsonObject Service::handleCreateScene( const JsonObject& params ) {
    JsonObject internalAnswer;
    m_scenesHandler->processCreateScene( params, internalAnswer );
    Log::debug( "handleCreateScene, answer :" + internalAnswer.dump() );
    return internalAnswer;
}

JsonObject Service::handleEditScene( const JsonObject& params ) {
    JsonObject internalAnswer = JSON_EMPTY_OBJ;
    m_scenesHandler->processEditScene( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleDeleteScene( const JsonObject& params ) {
    JsonObject internalAnswer = JSON_EMPTY_OBJ;
    m_scenesHandler->processDeleteScene( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleDeleteAllScenes( const JsonObject& params ) {
    JsonObject internalAnswer = JSON_EMPTY_OBJ;
    m_scenesHandler->processDeleteAllScenes( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleSwitchScene( const JsonObject& params ) {
    JsonObject internalAnswer;
    m_scenesHandler->processEnableScene( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleCloneScene( const JsonObject& params ) {
    JsonObject internalAnswer;
    m_scenesHandler->processCloneScene( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleRunScene( const JsonObject& params ) {
    JsonObject internalAnswer = JSON_EMPTY_OBJ;
    m_scenesHandler->processRunScene( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleStopScene( const JsonObject& params ) {
    JsonObject internalAnswer = JSON_EMPTY_OBJ;
    m_scenesHandler->processStopScene( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleSetTestClock( const JsonObject& params ) {
    JsonObject internalAnswer;
    int64_t startTime;
    if( !getField( params, "startUnixTime", internalAnswer, startTime ) ) {
        return internalAnswer;
    }

    std::unique_ptr< SceneEngine::TestClock > testClock;

    std::string units;
    if( !getField( params, "units", internalAnswer, units ) ) {
        return internalAnswer;
    }

    int64_t timeZoneOffset;
    if( !getField( params, "timeZoneOffset", internalAnswer, timeZoneOffset ) ) {
        return internalAnswer;
    }

    if( units == "hour" ) {
        LOGGER_INFO( cc::info( "test clock units are set in hours" ) );

        testClock = std::make_unique< SceneEngine::TestClock >( std::chrono::seconds( startTime ),
                                                                std::chrono::hours( timeZoneOffset ) );
    }
    else if( units == "minute" ) {
        LOGGER_INFO( cc::info( "test clock units are set in minutes" ) );

        testClock = std::make_unique< SceneEngine::TestClock >( std::chrono::seconds( startTime ),
                                                                std::chrono::minutes( timeZoneOffset ) );
    }
    m_scenesManager->setClock( *testClock );
    m_scenesHandler->setClock( *testClock );
    m_clock = std::move( testClock );

    return internalAnswer;
}

JsonObject Service::handleRoomSet( const JsonObject& params ) {
    JsonObject internalAnswer;
    m_scenesHandler->processSetRoom( params, internalAnswer );
    return internalAnswer;
}

JsonObject Service::handleRoomRemoved( const JsonObject& params ) {
    return m_scenesHandler->processRoomWasRemoved( params );
}

JsonObject Service::handleNotificationAdd( const JsonObject& params ) {
    return m_scenesHandler->processAddNotification( params );
}

JsonObject Service::handleNotificationRemove( const JsonObject& params ) {
    return m_scenesHandler->processRemoveNotification( params );
}

JsonObject Service::handleTimeList( const JsonObject& params ) {
    return m_scenesHandler->processTimeList( params );
}

// json = {
//        "reason": "backup"
// }
JsonObject Service::handleLockRequest( const JsonObject& request ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.tryLock( request );
    return lockHelper.getResponse();
}

// json = {
//        "reason": "backup"
// }
JsonObject Service::handleUnlockRequest( const JsonObject& request ) {
    auto& lockHelper = CommonUtils::LockHelper::global();
    lockHelper.unlock( request );
    return lockHelper.getResponse();
}

JsonObject Service::handleBackupCreateRequest( const JsonObject& request ) {
    m_registryFlushScheduler->flushNow();

    CommonUtils::BackupHelper helper( getDBClient(), request );
    // NOTE: Not backing up following databases:
    //  db.firmware.scenes_manager.controller.subscription
    //  temp.db.firmware.scenes_manager.items
    //  temp.db.firmware.scenes_manager.option_items
    for( auto& tableName : BackupTables ) {
        if( !helper.backupDatabase( tableName ) ) {
            return helper.getResponse();
        }
    }
    return helper.getResponse();
}

int Service::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& tableName : BackupTables ) {
        if( !helper.restoreDatabase( tableName ) ) {
            return helper.getReturnCode();
        }
    }
    return helper.getReturnCode();
}

void Service::processActionRunRequest( const std::string& name,
                                       const SceneEngine::Action& action,
                                       SceneEngine::ActionExecutorPtr actionExecutorPtr ) {
    using namespace SceneEngine;

    ActionException actionException( action.getExceptionData(), *m_statesRegistry, *m_expressions );
    ActionException::State exceptionState = ActionException::State::Satisfied;

    bool exceptionEvaluated = false;
    try {
        exceptionState = actionException.evaluate();
        exceptionEvaluated = true;
    }
    catch( const ActionExceptionError& ex ) {
        LOGGER_ERROR( cc::error( ex.what() ) );
    }

    if( !exceptionEvaluated || exceptionState != ActionException::State::Satisfied ) {
        if( exceptionState != ActionException::State::Satisfied ) {
            LOGGER_INFO( cc::info( "Skipping action execution due to exception, scene id: " ) +
                         cc::note( actionExecutorPtr->getSceneId() ) + cc::info( " action id: " ) +
                         cc::num10( action.getId() ) );
        }

        ActionInterface::SendFinishCallback callback( action, actionExecutorPtr );
        callback.setResult( exceptionEvaluated == true );
        return;
    }

    try {
        SceneActions::ActionInterfacePtr methodHandler =
            SceneActions::getActionForMethod( name, *this, *m_controllerFactory, *m_expressions, *m_scenesHandler );
        methodHandler->run( action, actionExecutorPtr );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Scenes manager, run scene method has failed : " + CommonUtils::whatError( ex ) ) );
    }
}

Service::AsyncRequestInfo::AsyncRequestInfo( JsonObject request, std::string uuid, std::string sender )
: m_request( std::move( request ) )
, m_uuid( std::move( uuid ) )
, m_sender( std::move( sender ) ) {}

}  // namespace ScenesManager
}  // namespace HubPlatform
