#include "cored/core_service.h"
#include "cored/house_mode_alarms.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/values.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/cored_gateway_processes.h"
#include "cored/model/cored_gateway_processes_utils.h"
#include "cored/exceptions.h"
#include "cored/features_manager.h"
#include "hub_settings/hub_settings_manager.h"
#include "cored/remote_interface/system_attribute_types.h"

#include "cored/db_modifier/gateway_setting_db_modifier.h"
#include "cored/db_modifier/device_setting_db_modifier.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "cored/db_modifier/policy_db_modifier.h"
#include "cored/db_modifier/room_db_modifier.h"
#include "cored/db_modifier/security_item_db_modifier.h"

#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/script_events/house_mode_alarmed.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/permanent_db_write_scheduler.h"
#include "cored/script_events/device_added.h"
#include "cored/script_events/device_removed.h"
#include "cored/script_events/device_updated.h"
#include "cored/script_events/item_added.h"
#include "cored/script_events/item_dictionary_updated.h"
#include "cored/script_events/item_removed.h"
#include "cored/script_events/item_updated.h"
#include "cored/script_events/gateway_setting_added.h"
#include "cored/script_events/gateway_setting_updated.h"
#include "cored/script_events/gateway_setting_removed.h"
#include "cored/script_events/device_setting_added.h"
#include "cored/script_events/device_setting_dictionary_updated.h"
#include "cored/script_events/device_setting_removed.h"
#include "cored/script_events/device_setting_updated.h"

#include "cored/swinger_shutdown/swinger_shutdown.h"
#include "cored/walk_test/walk_test.h"

#include "uid/field_names.h"

#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/groups_names.h"
#include "service_interfaces/postponed_task.h"
#include "bus/messagesclasses.h"
#include "json_utils/json_utils.h"

#include "service_interfaces/groups_names.h"
#include "system_status/ram_usage.h"
#include "cored/webtrc_group_info.h"

#include "backupd/remote_interface/backup_remote_interface.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include "cored/what_happened.h"
#include "database_client/dbqueryparser.h"
#include "database_client/dbid.h"
#include "cored/resources/core_resources.h"

#include "config_reader/config_keys.h"
#include "config_reader/config_reader.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "system_utils/timezone/debian_timezone.h"
#include "system_utils/timezone/openwrt_timezone.h"
#include "system_utils/environment.h"
#include "opkg_utils/opkg_manager.h"
#include "base_utils/utils.h"
#include "base_utils/lzo_utils.h"
#include "jsonrpc/jsonrpc.h"
#include "common_utils/cpp14.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/hub_info_helper.h"
#include "common_utils/exception_utils.h"
#include "provision_data_utils/provision_data_manager.h"
#include "hub_settings/hub_settings_manager.h"
#include "cored/remote_interface/cored_interface.h"
#include "cored/remote_interface/cored_interface_factory.h"
#include "cored/group_operations/device_group_updater.h"
#include "cored/group_operations/group_operations_utils.h"
#include "cored/group_operations/resources.h"
#include "cored/group_operations/device_group.h"
#include "cored/policies/policies_updater.h"
#include "cored/walk_test/walk_test.h"
#include "cored/protect/protect_utils.h"
#include "cored/protect/security_config.h"
#include "cored/protect/resources.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <functional>
#include <thread>
#include <utility>
#include <chrono>

namespace HubPlatform {
namespace CoreD {

namespace {
constexpr auto WriteToPermanentDBInterval = std::chrono::minutes( 30 );
}  // namespace

using namespace HubPlatform::CoreD::Model::Resources;

#define GATEWAY_BROADCASTER \
    ( [this]( MessageSubClasses subClass, const Json& json ) { sendBroadcast( subClass, json ); } )
#define DEVICE_BROADCASTER \
    ( [this]( MessageSubClasses subClass, const Json& json ) { sendDeviceBroadcast( subClass, json ); } )
#define ITEM_BROADCASTER \
    ( [this]( MessageSubClasses subClass, const Json& json ) { sendItemBroadcast( subClass, json ); } )
#define POLICY_BROADCASTER \
    ( [this]( MessageSubClasses subClass, const Json& json ) { sendPolicyBroadcast( subClass, json ); } )
#define GATEWAY_SETTING_BROADCASTER \
    ( [this]( MessageSubClasses subClass, const Json& json ) { sendGatewaySettingBroadcast( subClass, json ); } )
#define DEVICE_SETTING_BROADCASTER \
    ( [this]( MessageSubClasses subClass, const Json& json ) { sendDeviceSettingBroadcast( subClass, json ); } )
#define ROOM_BROADCASTER                                            \
    ( [this]( MessageSubClasses subClass, const Json& json ) {      \
        sendBroadcast( subClass, json, GroupsNames::DEVICE_GROUP ); \
    } )

CoreService::CoreService()
: Service( GroupsNames::CORED ) {}

CoreService::CoreService( const std::string& name,
                          std::unique_ptr< Services::BusWrapper > impl,
                          std::unique_ptr< IDbClient > dbClient,
                          ConfigReader&& configReader )
: Service( name, std::move( impl ), std::move( dbClient ) )
, m_configReader( std::make_unique< ConfigReader >( std::move( configReader ) ) ) {}

CoreService::~CoreService() = default;

void CoreService::setUp() {
    Services::Service::setUp();

    if( !m_configReader ) {
        m_configReader = std::make_unique< ConfigReader >();
    }

    m_featuresManager = std::make_unique< ExternalFeaturesManager >( *this );
    m_settingsManager = std::make_unique< Hub::SettingsManager >( getDBClient(), *m_configReader );

    m_roomManager = std::make_unique< RoomManager >( *this );
    m_deviceManager = std::make_unique< DeviceManager >( *this );
    m_whatHappened = std::make_unique< WhatHappened >( getDBClient() );
    m_gatewaysDb = std::make_unique< GatewayDbModifier >( getDBClient(), GATEWAY_BROADCASTER );
    m_devicesDb = std::make_unique< DeviceDbModifier >( getDBClient(), DEVICE_BROADCASTER );
    m_swingerShutdownDevicesManager =
        std::make_unique< HubPlatform::CoreD::SwingerShutdown::SwingerShutdown >( *this, *m_configReader );
    m_securityItemsDbModifier =
        std::make_unique< SecurityItemDbModifier >( *this, *m_featuresManager, *m_swingerShutdownDevicesManager );
    m_itemsDb = std::make_unique< ItemDbModifier >( *this,
                                                    ProvisionDataUtils::getManager().getProvisionData(),
                                                    ITEM_BROADCASTER,
                                                    *m_featuresManager,
                                                    *m_securityItemsDbModifier );
    m_policiesDb = std::make_unique< PolicyDbModifier >( getDBClient(), POLICY_BROADCASTER );
    m_gatewaySettingsDb = std::make_unique< GatewaySettingDbModifier >( *this, GATEWAY_SETTING_BROADCASTER );
    m_deviceSettingsDb = std::make_unique< DeviceSettingDbModifier >( *this, DEVICE_SETTING_BROADCASTER );
    m_roomsDb = std::make_unique< RoomDbModifier >( getDBClient(), ROOM_BROADCASTER );
    m_houseModeProcessor = std::make_unique< HouseModeProcessor >( *this, getDBClient() );

    m_permanentDBWriteScheduler =
        std::make_unique< PermanentDBWriteScheduler >( *this, getItemsDbModifier(), getDevicesDbModifier() );
    m_permanentDBWriteScheduler->schedule( WriteToPermanentDBInterval );

    m_busDBManager = std::make_unique< BusDbManager >( getDBClient() );
    m_houseModeAlarms =
        std::make_unique< HouseModeAlarms >( *this, [&]( const JsonObject& json ) { handleSetItemValue( json ); } );

    HubPlatform::CoreD::GroupOperationsHandler::BroadcastCallback sendBroadcast =
        [this]( MessageSubClasses messageSubClass, const JsonObject& answer ) {
            MessageClasses messageClass = EVENT;
            LOGGER_TRACE_RAW( cc::trace( "sendBroadcast: send broadcast with subclass " +
                                         msgSubclassToStr( messageClass, messageSubClass ) + ": " + answer.dump() ) );
            Services::Message message = { GroupsNames::DEVICE_GROUP, messageClass, messageSubClass, answer };
            sendDeviceItemGroupBroadcast( messageSubClass, answer );
            LOGGER_TRACE_RAW( cc::trace( "sendBroadcast: broadcast is sent" ) );
        };

    m_core = CoreDRemoteInterface::makeInstance( *this );
    m_deviceGroupHandler =
        std::make_unique< HubPlatform::CoreD::GroupOperationsHandler >( getDBClient(), sendBroadcast, *m_core );
    m_policyHandler =
        std::make_unique< HubPlatform::CoreD::PolicyOperationsHandler >( getDBClient(), sendBroadcast, *m_core );
    CoreDHouseModes::clearDeviceFromHouseModesAlarmedList( getDBClient() );

    m_featuresManager->setUpExternalFeatures();
    ConfigReader configReader;
    initLogOptionsFromFirmwareConfig( HubPlatform::Environment::getConfigsFolder() + "/firmware.cfg",
                                      configReader,
                                      UpdateManager::RemoteInterface( *this ) );
    setUpPredefinedDeviceGroups();
    setUpPredefinedItemGroups();
    setUpPredefinedContactIds();
    setUpPredefinedSecurityConfigs();
    setUpPredefinedPolicies();
    setUpHouseModes();
    setUpGeneratedSensorGroups();
}

void CoreService::tearDown() {
    m_permanentDBWriteScheduler->cancelSchedule();
    m_permanentDBWriteScheduler->doNow();
    stopWalkTestIfNeeded();
    Services::Service::tearDown();
}

#undef ITEM_BROADCASTER
#undef DEVICE_BROADCASTER
#undef GATEWAY_BROADCASTER
#undef ROOM_BROADCASTER

void CoreService::setUpHouseModes() {
    auto& dbClient = getDBClient();

    try {
        CoreDHouseModes::ensureHouseModesInDb( dbClient, *m_configReader );

        CoreDHouseModes::setAlarmPhase( dbClient, HouseModeAlarmPhase::Idle, "" );

        if( !dbClient.has( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ) ) ) {
            const auto defaultModeIdPath = ConfigReader::Path( *m_configReader, ConfigKeys::HouseModesDefault );
            const auto switchToDelayPath = ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeSwitchToDelay );
            const auto alarmDelayPath = ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeAlarmDelay );
            const auto entryDelayNormalPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayNormal );
            const auto entryDelayLongExtendedPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayLongExtended );
            const auto entryDelayExtendedPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayExtended );
            const auto entryDelayInstantPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayInstant );
            const auto abortWindowDefaultPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeAbortWindowDefault );
            const auto abortWindowMinimumPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeAbortWindowMinimum );
            const auto abortWindowMaximumPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeAbortWindowMaximum );

            const auto defaultModeId = std::to_string( m_configReader->getInt( defaultModeIdPath ) );
            const auto switchToDelay = m_configReader->getInt( switchToDelayPath );
            auto alarmToDelay = m_configReader->getInt( alarmDelayPath );
            auto entryDelayNormal = m_configReader->getInt( entryDelayNormalPath );
            const auto entryDelayLongExtended = m_configReader->getInt( entryDelayLongExtendedPath );
            const auto entryDelayExtended = m_configReader->getInt( entryDelayExtendedPath );
            const auto entryDelayInstant = m_configReader->getInt( entryDelayInstantPath );
            if( alarmToDelay == 0 ) {
                alarmToDelay = entryDelayNormal;
            }
            else {
                entryDelayNormal = alarmToDelay;
            }
            const auto abortWindowDefault = m_configReader->getInt( abortWindowDefaultPath );
            const auto abortWindowMinimum = m_configReader->getInt( abortWindowMinimumPath );
            const auto abortWindowMaximum = m_configReader->getInt( abortWindowMaximumPath );

            auto entryDelayJson =
                JSON( CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_NORMAL
                      << entryDelayNormal << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_LONG_EXTENDED
                      << entryDelayLongExtended << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_EXTENDED
                      << entryDelayExtended << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_INSTANT << entryDelayInstant );
            auto abortWindowJson =
                JSON( CoreDHouseModes::HOUSE_MODE_ABORT_WINDOW_DEFAULT
                      << abortWindowDefault << CoreDHouseModes::HOUSE_MODE_ABORT_WINDOW_MINIMUM << abortWindowMinimum
                      << CoreDHouseModes::HOUSE_MODE_ABORT_WINDOW_MAXIMUM << abortWindowMaximum );

            dbClient.insert(
                HUBSettings,
                JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY << CoreDHouseModes::HOUSE_MODE_ENTRY
                                        << defaultModeId << CoreDHouseModes::HOUSE_MODE_SWITCH_DELAY << switchToDelay
                                        << CoreDHouseModes::HOUSE_MODE_ALARM_DELAY << entryDelayNormal
                                        << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY << entryDelayJson
                                        << CoreDHouseModes::HOUSE_MODE_ABORT_WINDOW << abortWindowJson ) );
        }
        else {
            auto houseModeSetting = dbClient.findOne(
                HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );
            const auto entryDelayNormalPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayNormal );
            const auto entryDelayLongExtendedPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayLongExtended );
            const auto entryDelayExtendedPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayExtended );
            const auto entryDelayInstantPath =
                ConfigReader::Path( *m_configReader, ConfigKeys::HouseModeEntryDelayInstant );

            auto entryDelayNormal = m_configReader->getInt( entryDelayNormalPath );
            const auto entryDelayLongExtended = m_configReader->getInt( entryDelayLongExtendedPath );
            const auto entryDelayExtended = m_configReader->getInt( entryDelayExtendedPath );
            const auto entryDelayInstant = m_configReader->getInt( entryDelayInstantPath );

            houseModeSetting[ CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY ] =
                JSON( CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_NORMAL
                      << entryDelayNormal << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_LONG_EXTENDED
                      << entryDelayLongExtended << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_EXTENDED
                      << entryDelayExtended << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_INSTANT << entryDelayInstant );
            const std::string id = json_get_str( houseModeSetting, db::ID_FIELD_NAME );
            dbClient.update( HUBSettings, JSON( db::ID_FIELD_NAME << id ), houseModeSetting );
        }
    }
    catch( const std::exception& error ) {
        LOGGER_WARN( cc::warn( "Cannot read house modes configuration: " ) +
                     cc::error( CommonUtils::whatError( error ) ) );
    }

    dbClient.update(
        HUBTempSettings,
        JSON_EMPTY_OBJ,
        JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY << CoreDHouseModes::HOUSE_MODE_SWITCH_ENTRY << ""
                                << CoreDHouseModes::HOUSE_MODE_TIME_TO_SWITCH << 0 ),
        true );

    if( !dbClient.exist( HaHouseModesProtectButtonsDBName ) ) {
        dbClient.create( HaHouseModesProtectButtonsDBName );
    }
    if( !dbClient.exist( HaHouseModesSecurityThreatsDBName ) ) {
        dbClient.create( HaHouseModesSecurityThreatsDBName );
    }
    if( !dbClient.exist( HaHouseModesCommandTimestampsDBName ) ) {
        dbClient.create( HaHouseModesCommandTimestampsDBName );
    }
}

std::string CoreService::generateOperationId( const std::string& uuid, const std::string& sender ) {
    return sender + "|" + uuid;
}

bool CoreService::handleAsyncRequestFromBus( const JsonObject& request,
                                             MessageSubClasses requestType,
                                             const std::string& uuid,
                                             const std::string& sender ) {
    try {
        switch( requestType ) {
        case SET_ITEM_VALUE:
            if( !handleSetItemValue( request, generateOperationId( uuid, sender ) ) ) {
                sendAsyncResponse( sender, JSON_EMPTY_OBJ, uuid );
            }
            return true;
        case ADD_ITEM_DICTIONARY_VALUE:
            if( !handleAddItemDictionaryValue( request, generateOperationId( uuid, sender ) ) ) {
                sendAsyncResponse( sender, JSON_EMPTY_OBJ, uuid );
            }
            return true;
        case SET_ITEM_DICTIONARY_VALUE:
            if( !handleSetItemDictionaryValue( request, generateOperationId( uuid, sender ) ) ) {
                sendAsyncResponse( sender, JSON_EMPTY_OBJ, uuid );
            }
            return true;
        case REMOVE_ITEM_DICTIONARY_VALUE:
            if( !handleRemoveItemDictionaryValue( request, generateOperationId( uuid, sender ) ) ) {
                sendAsyncResponse( sender, JSON_EMPTY_OBJ, uuid );
            }
            return true;
        case CLEAR_ITEM_DICTIONARY:
            if( !handleClearItemDictionary( request, generateOperationId( uuid, sender ) ) ) {
                sendAsyncResponse( sender, JSON_EMPTY_OBJ, uuid );
            }
            return true;
        default:
            return false;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_WARN( cc::warn( "Exception while handling async request from bus: " ) +
                     cc::note( CommonUtils::whatError( ex ) ) );
        sendAsyncResponse( GroupsNames::UI_GROUP, JSON_EMPTY_OBJ, uuid );
        return true;
    }
}

void CoreService::handleLegacyMethodCall( const JsonObject& json, const std::string& sender ) {
    Service::handleLegacyMethodCall( json, sender );
    using MethodHandler = void ( CoreService::* )( const JsonObject& json );
    using MethodHandlersMap = std::map< std::string, MethodHandler >;

    static const MethodHandlersMap methodHandlers = {
        { RPC::RemovePluginGatewaysRPC, &CoreService::handleRemovePluginGateways },
        { RPC::RemoveGatewayDevicesRPC, &CoreService::handleRemoveGatewayDevices },
        { RPC::RemoveDeviceRPC, &CoreService::handleRemoveDevice },
        { RPC::RenameDeviceRPC, &CoreService::handleRenameDevice },
        { RPC::UpdateItemFieldRPC, &CoreService::handleUpdateItemField },
        { RPC::UpdateItemValueRPC, &CoreService::handleUpdateItemValue },
        { RPC::UpdateItemDictionaryValueRPC, &CoreService::handleUpdateItemDictionaryValue },
        { RPC::UpdateSettingValueRPC, &CoreService::handleUpdateSettingValue },
        { RPC::UpdateSettingDictionaryValueRPC, &CoreService::handleUpdateSettingDictionaryValue },
        { RPC::UpdateSettingStatusRPC, &CoreService::handleUpdateSettingStatus },
        { RPC::UpdateReachableStateRPC, &CoreService::handleUpdateReachableState },
        { RPC::UpdateReadyStateRPC, &CoreService::handleUpdateReadyState },
        { RPC::UpdateDeviceNameRPC, &CoreService::handleUpdateDeviceName },
        { RPC::ForceRemoveDeviceRPC, &CoreService::handleRemoveDeviceForce },  // throw exception
    };

    const auto it = methodHandlers.find( json_get_str( json, "method" ) );
    if( it == methodHandlers.end() ) {
        return;
    }
    try {
        const auto handler = it->second;
        ( this->*handler )( json[ "params" ] );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception while handling method: " ) + cc::note( it->first ) +
                      cc::error( ", exception: " ) + cc::note( CommonUtils::whatError( ex ) ) );
    }
}

JsonObject CoreService::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    try {
        switch( requestType ) {
        case CREATE_ROOM:
            return m_roomManager->createRoom( request );

        case SET_ROOM_NAME:
            return m_roomManager->setRoomName( request );

        case SET_ROOM_ORDER:
            return m_roomManager->setRoomOrder( request );

        case DELETE_ROOM: {
            const auto roomId = json_get_str( request, "_id" );
            deleteRoom( roomId );
            return JSON_EMPTY_OBJ;
        }

        case DELETE_ROOM_ALL: {
            const JsonObject fieldsToReturn = JSON( "_id" << 1 );
            const auto roomIds = getDBClient().findAll( DBNames::RoomsDB, JSON_EMPTY_OBJ, &fieldsToReturn );
            for( const auto& roomId : roomIds ) {
                deleteRoom( json_get_str( roomId, "_id" ) );
            }
            return JSON_EMPTY_OBJ;
        }

        case ENSURE_GATEWAY:
            return m_deviceManager->ensureGateway( request );

        case MODIFY_GATEWAY:
            return handleModifyGateway( request );

        case ADD_GATEWAY_SETTING:
            return m_deviceManager->addGatewaySetting( request );

        case ADD_DEVICE:
            return handleAddDevice( request );

        case MODIFY_DEVICE:
            return handleModifyDevice( request );

        case RENAME_DEVICE:
            return m_deviceManager->renameDevice( request );

        case REMOVE_DEVICE_SYNC:
            removeDevice( request );
            return JSON_EMPTY_OBJ;

        case PLACE_DEVICE: {
            return handlePlaceDevice( request );
        }

        case SET_DEVICE_ARMED:
            return m_deviceManager->setDeviceArmed( request );

        case SET_DEVICE_PROTECTCONFIG:
            return m_deviceManager->setDeviceProtectConfig( request );

        case ADD_ITEM:
            return handleAddItem( request );

        case ADD_DEVICE_SETTING:
            return m_deviceManager->addDeviceSetting( request );

        case MODIFY_ITEM:
            return handleModifyItem( request );

        case MODIFY_DEVICE_SETTING:
            return handleModifyDeviceSetting( request );

        case REMOVE_ITEM:
            handleRemoveItem( request );
            return JSON_EMPTY_OBJ;

        case REMOVE_POLICY:
            handleRemovePolicy( request );
            return JSON_EMPTY_OBJ;

        case REMOVE_DEVICE_SETTING:
            removeDeviceSetting( request );
            return JSON_EMPTY_OBJ;

        case GET_HUB_STATUS:
            return getHubStatus( request );
        case GLOBAL_LOCK: {
            auto& lockHelper = CommonUtils::LockHelper::global();
            lockHelper.tryLock( request );
            return lockHelper.getResponse();
        }
        case GLOBAL_UNLOCK: {
            auto& lockHelper = CommonUtils::LockHelper::global();
            lockHelper.unlock( request );
            return lockHelper.getResponse();
        }
        case BACKUP_CREATE: {
            return handleBackupCreate( request );
        }
        case RESTORE_PROGRESS: {
            handleBackupRestore( request );
            return JSON_EMPTY_OBJ;
        }

        // TODO: could be refactored by adding separated MessageClass handle for whatHappened
        case SCENE_DELETED: {
            m_whatHappened->saveRuleDeleted( request );
            return JSON_EMPTY_OBJ;
        }
        case SCENE_ADDED:
        case SCENE_CHANGED: {
            m_whatHappened->saveRuleLog( request, requestType );
            return JSON_EMPTY_OBJ;
        }
        case USER_ASSIGN: {
            m_whatHappened->saveUserAssign( request, true );
            return JSON_EMPTY_OBJ;
        }
        case USER_UNASSIGN: {
            m_whatHappened->saveUserAssign( request, false );
            return JSON_EMPTY_OBJ;
        }
        case USER_BIND: {
            m_whatHappened->saveUserBind( request, true );
            return JSON_EMPTY_OBJ;
        }
        case USER_UNBIND: {
            m_whatHappened->saveUserBind( request, false );
            return JSON_EMPTY_OBJ;
        }
        case SET_HOUSE_MODE:
            return handleSetHouseMode( request );

        case CANCEL_SWITCHING_HOUSE_MODE:
            return handleCancelSwitchHouseMode( request );

        case HOUSE_MODE_CANCEL_ENTRY_DELAY:
            return handleCancelEntryDelay( request );

        case HOUSE_MODE_SKIP_ENTRY_DELAY:
            return handleSkipEntryDelay( request );

        case HOUSE_MODE_SET_ENTRY_DELAY:
            return handleSetEntryDelay( request );

        case HOUSE_MODE_ENTRY_DELAY_RESET:
            return handleResetEntryDelay( request );

        case HOUSE_MODE_ADD_DISARMED_DEVICE:
            return handleAddDisarmedDevice( request );

        case HOUSE_MODE_REMOVE_DISARMED_DEVICE:
            return handleRemoveDisarmedDevice( request );

        case HOUSE_MODE_ADD_ALARMS_OFF_DEVICE:
            return handleAddAlarmsOffDevice( request );

        case HOUSE_MODE_REMOVE_ALARMS_OFF_DEVICE:
            return handleRemoveAlarmsOffDevice( request );

        case HOUSE_MODE_ADD_CAMERAS_OFF_DEVICE:
            return handleAddCamerasOffDevice( request );

        case HOUSE_MODE_REMOVE_CAMERAS_OFF_DEVICE:
            return handleRemoveCamerasOffDevice( request );

        case HOUSE_MODE_ADD_BYPASS_DEVICES:
            return handleAddBypassDevices( request );

        case HOUSE_MODE_REMOVE_BYPASS_DEVICES:
            return handleRemoveBypassDevices( request );

        case HOUSE_MODES_LOCAL_ALARM_OFF:
            return handleLocalAlarmOff( request );

        case HOUSE_MODES_FORCE_DISARM:
            return handleForceDisarm( request );

        case SET_NOTIFICATIONS_HOUSE_MODE:
            return handleSetNotificationsForHouseMode( request );

        case HUB_SET_COORDINATES:
            return handleSetHubCoordinates( request );

        case HUB_SET_TIME_ZONE:
            return handleSetHubTimeZone( request );

        case SET_DISARMED_DEFAULT_HOUSE_MODE:
            return handleSetDisarmedDefault( request );

        case HOUSE_MODE_PROTECT_SET:
            return handleHouseModeProtectSet( request );

        case HOUSE_MODE_PROTECT_BUTTONS_SET:
            return handleHouseModeProtectButtonsSet( request );

        case HOUSE_MODES_PROTECT_DEVICES_ADD:
            return handleAddProtectDevices( request );

        case HOUSE_MODES_PROTECT_DEVICES_REMOVE:
            return handleRemoveProtectDevices( request );

        case SET_SERVICE_NOTIFICATIONS:
            return handleSetServiceNotifications( request );

        case UPDATE_DEVICE_FIRMWARE:
            return handleUpdateDeviceFirmware( request );

        case SET_DEVICE_SETTING_VALUE:
            return handleSetDeviceSettingValue( request );

        case SET_DEVICE_SETTING_DICTIONARY_VALUE:
            return handleSetDeviceSettingDictionaryValue( request );

        case RESET_DEVICE_SETTING:
            return handleResetDeviceSetting( request );

        case CHECK_DEVICE:
            return handleCheckDevice( request );

        case REQUEST_PROVISION_DATA:
            return handleRequestProvisionData( request );

        case REQUEST_DB_DATA:
            return handleRequestDBData( request );

        case ADD_VERA_DEVICE_MAPPING:
            return handleAddVeraDeviceMapping( request );

        case BACKUP_RESTORE_VERA: {
            return handleBackupVeraRequest( request );
        }

        case DEVICE_GROUP_CREATE:
            return m_deviceGroupHandler->processCreateDeviceGroupRequest( const_cast< JsonObject& >( request ) );
        case DEVICE_GROUP_DELETE:
            return m_deviceGroupHandler->processDeleteDeviceGroupRequest( request );
        case DEVICE_GROUP_UPDATE:
            return m_deviceGroupHandler->processUpdateDeviceGroupRequest( request );
        case DEVICE_GROUPS_DEVICE_ADD:
            return m_deviceGroupHandler->processAddOrRemoveDevicesDeviceGroupRequest( request, true );
        case DEVICE_GROUPS_DEVICE_REMOVE:
            return m_deviceGroupHandler->processAddOrRemoveDevicesDeviceGroupRequest( request, false );
        case ITEM_GROUP_CREATE:
            return m_deviceGroupHandler->processCreateItemGroupRequest( request );
        case ITEM_GROUP_DELETE:
            return m_deviceGroupHandler->processDeleteItemGroupRequest( request );
        case ITEM_GROUP_UPDATE:
            return m_deviceGroupHandler->processUpdateItemGroupRequest( request );
        case ITEM_GROUPS_LIST:
            return m_deviceGroupHandler->processItemGroupsListRequest( request );
        case ITEM_GROUP_GET:
            return m_deviceGroupHandler->processItemGroupGetRequest( request );
        case DEVICE_ITEM_GROUP_EXPAND:
            return m_deviceGroupHandler->processDeviceItemGroupExpandRequest( request );
        case DEVICE_GROUP_FIND:
            return m_deviceGroupHandler->processDeviceGroupFindRequest( request );

        case POLICY_CREATE:
            return m_policyHandler->processPolicyCreateRequest( request );
        case POLICY_DELETE:
            return m_policyHandler->processPolicyDeleteRequest( request );
        case POLICY_UPDATE:
            return m_policyHandler->processPolicyUpdateRequest( request );
        case POLICY_GET:
            return m_policyHandler->processPolicyGetRequest( request );
        case POLICY_LIST:
            return m_policyHandler->processPolicyListRequest( request );

        case MODES_SWINGER_SHUTDOWN_LIST:
            return getSwingerShutdownList( request );
        case MODES_SWINGER_SHUTDOWN_RESET:
            return swingerShutdownReset( request );
        case MODES_SWINGER_SHUTDOWN_DISABLE_ADD:
            return swingerShutdownDisableAdd( request );
        case MODES_SWINGER_SHUTDOWN_DISABLE_REMOVE:
            return swingerShutdownDisableRemove( request );
        case MODES_SWINGER_SHUTDOWN_LIMITS_SET:
            return swingerShutdownLimitsSet( request );
        case MODES_SWINGER_SHUTDOWN_LIMITS_RESET:
            return swingerShutdownLimitsReset( request );
        case MODES_PROTECT_SYSTEM_TEST_START:
            return protectSystemTest( request );
        case MODES_PROTECT_SYSTEM_TEST_STOP:
            return protectSystemTestStop( request );
        case MODES_PROTECT_SYSTEM_TEST_EXTEND:
            return protectSystemTestExtend( request );

        default:
            return Service::handleRequestFromBus( request, requestType );
        }
    }
    catch( const DeviceManagerException& ex ) {
        JsonObject error;
        setError( error, Errors::WRONG_PARAMS, ex.what() );
        return error;
    }
    catch( const std::exception& ex ) {
        JsonObject error;
        setError( error, Errors::INTERNAL_OPERATION_ERROR, ex.what() );
        return error;
    }

    assert( false );

    return JSON_EMPTY_OBJ;
}

JsonObject CoreService::handlePlaceDevice( const JsonObject& request ) {
    const auto deviceId = json_get_str( request, "_id" );
    const auto roomId = json_get_str( request, DeviceFields::RoomIdFieldName );
    std::set< std::string > separateChildDevices;
    JsonArray strings;
    if( json_get( request, DeviceFields::SeparateChildDevices, strings ) ) {
        try {
            fromJsonStringArray( strings, [&separateChildDevices]( const std::string& value ) {
                separateChildDevices.emplace( value );
            } );
        }
        catch( ... ) {
            JsonObject result;
            setErrorWithField( result, Errors::WRONG_PARAMS, "Incorrect type", DeviceFields::SeparateChildDevices );
            return result;
        }
    }

    bool parentRoom = json_get_bool( request, DeviceFields::ParentRoomFieldName );
    return m_deviceManager->placeDevice( deviceId, roomId, separateChildDevices );
}

void CoreService::handleMethodCall( const JsonObject& json,
                                    MessageSubClasses requestType,
                                    const std::string& /*sender*/ ) {
    using MethodHandler = void ( CoreService::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;

    static const MethodHandlersMap methodHandlers = {
        { UPDATE_DEVICE_STATUS, &CoreService::handleUpdateDeviceStatus },
        { SET_FEATURE_STATUS, &CoreService::handleSetFeatureStatus },
        { SET_HUB_SETTING_VALUE, &CoreService::handleSetSettingValue },
        { REMOVE_GATEWAY_SETTING, &CoreService::handleRemoveGatewaySetting },
        { UPDATE_GATEWAY_SETTING_VALUE, &CoreService::handleUpdateGatewaySettingValue },
        { UPDATE_GATEWAY_SETTING_STATUS, &CoreService::handleUpdateGatewaySettingStatus },
        { SET_GATEWAY_SETTING_VALUE, &CoreService::handleSetGatewaySettingValue },
        { START_GATEWAY_PROCESS, &CoreService::handleStartGatewayProcess },
        { STOP_GATEWAY_PROCESS, &CoreService::handleStopGatewayProcess },
        { RELOAD_GLOBAL_LOG_OPTIONS, &CoreService::handleReloadGlobalLogOptions },
        { PERFORM_DB_OPERATION, &CoreService::handlePerformDBOperation },
    };

    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( json );
    }
}

JsonObject CoreService::handleModifyGateway( const JsonObject& params ) {
    std::string gatewayId;
    JsonObject gateway;

    if( !json_get( params, RPCParams::GatewayId, gatewayId ) || !json_get( params, RPCParams::Gateway, gateway ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    auto newStatus = json_get_str( gateway, Model::Resources::GatewayFields::StatusFieldName );
    if( newStatus != toString( CoreD::Model::GatewayStatus::Ready ) ) {
        stopGatewayProcesses( gatewayId );
    }

    return m_deviceManager->modifyGateway( gatewayId, gateway );
}

JsonObject CoreService::handleAddDevice( const JsonObject& params ) {
    std::string category, subcategory;
    if( !json_get( params, DeviceFields::CategoryFieldName, category ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    if( !json_get( params, DeviceFields::SubcategoryFieldName, subcategory ) ) {
        subcategory = "";
    }

    // check if the device is a siren, add all siren devices to house modes here
    LOGGER_TRACE( cc::j( params ) );
    auto deviceManagerResponse = m_deviceManager->addDevice( params );

    auto modesAffectedByAlarmDevice =
        CoreDHouseModes::checkAlarmCategoryAffectedByHouseMode( getDBClient(), category, subcategory );
    if( !modesAffectedByAlarmDevice.empty() ) {
        // add the alarm device to house modes here, there will be no special device for it to add in handleAddItem()
        const auto deviceId = json_get_str( deviceManagerResponse, "id" );
        m_houseModeProcessor->handleAddDevice( deviceId, params );
    }
    return deviceManagerResponse;
}

/** extracted from handleAddDevice */
JsonObject CoreService::addDeviceToHouseModes( const std::string& deviceId, const Json& device ) {
    std::string category, subcategory;
    JsonObject addParams = device;  // NOLINT
    if( !json_get( device, DeviceFields::CategoryFieldName, category ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    if( !json_get( device, DeviceFields::SubcategoryFieldName, subcategory ) ) {
        subcategory = "";
    }

    bool armedField = false;

    auto allModes = CoreDHouseModes::getHouseModesList( getDBClient() );
    auto initDisarmedModes = [&allModes]( bool initWith = true ) -> std::map< std::string, bool > {
        std::map< std::string, bool > disarmedModes;
        for( const auto& mode : allModes ) {
            disarmedModes[ mode.m_id ] = initWith;
        }
        return disarmedModes;
    };

    std::map< std::string, bool > disarmedModes = initDisarmedModes( true );
    Json houseModesOptions = JSON_EMPTY_OBJ;
    bool hasHouseModesOptions = json_get( device, DeviceFields::HouseModesOptionsFieldName, houseModesOptions ) &&
                                houseModesOptions.is_object() &&
                                houseModesOptions.count( DeviceFields::HouseModesOptions::DisarmedFieldName );
    if( hasHouseModesOptions ) {
        auto disarmedIt = houseModesOptions.find( DeviceFields::HouseModesOptions::DisarmedFieldName );
        if( disarmedIt != houseModesOptions.end() && disarmedIt->is_array() ) {
            disarmedModes = initDisarmedModes( false );
            for( const auto& modeId : *disarmedIt ) {
                disarmedModes[ modeId ] = true;
            }
        }
    }

    const auto currentModeId = CoreDHouseModes::getCurrentHouseModeId( getDBClient() );
    std::vector< CoreDHouseModes::HouseModeEntry > houseModes = CoreDHouseModes::getHouseModesList( getDBClient() );

    if( !hasHouseModesOptions ) {
        auto affectedByCurrentHouseMode =
            CoreDHouseModes::checkCategoryAffectedByHouseMode( getDBClient(), category, subcategory );

        if( !affectedByCurrentHouseMode.empty() ) {
            for( const auto& affected : affectedByCurrentHouseMode ) {
                auto modeId = affected.first;
                bool armed = affected.second;
                disarmedModes[ modeId ] = !armed;
            }
        }
    }
    bool armed = false;
    auto securityConfigId = json_get_str( device, DeviceFields::ProtectConfig );
    auto houseModeArmedWhen = m_houseModeProcessor->houseModeIdToArmedWhenField( currentModeId );
    if( !securityConfigId.empty() || securityConfigId != "default" ) {
        auto securityConfigJson = Protect::ProtectUtils::getSecurityConfigObject( getDBClient(), securityConfigId );
        Protect::SecurityConfigInfo securityConfig;
        securityConfig.fromJson( securityConfigJson );
        auto armedWhenArray = securityConfig.getArmedWhen();
        for( auto& armedWhen : armedWhenArray ) {
            if( armedWhen == houseModeArmedWhen ) {
                armed = true;
                break;
            }
        }
    }

    if( !disarmedModes.empty() ) {
        auto it = disarmedModes.find( currentModeId );
        if( it != disarmedModes.end() ) {
            armedField = !it->second;
            if( armed ) {
                addParams[ DeviceFields::ArmedFieldName ] = armed;
            }
            else {
                addParams[ DeviceFields::ArmedFieldName ] = armedField;
            }
        }
    }

    if( armed ) {
        m_devicesDb->update( deviceId, JSON( DeviceFields::ArmedFieldName << armed ) );
    }
    else {
        m_devicesDb->update( deviceId, JSON( DeviceFields::ArmedFieldName << armedField ) );
    }

    if( !disarmedModes.empty() ) {
        for( const auto& houseMode : houseModes ) {
            if( !disarmedModes[ houseMode.m_id ] )
                continue;
            if( armed ) {
                CoreDHouseModes::addDisarmedDevice(
                    *this, getDevicesDbModifier(), houseMode.m_id, deviceId, true, false );
            }
            else {
                CoreDHouseModes::addDisarmedDevice(
                    *this, getDevicesDbModifier(), houseMode.m_id, deviceId, true, true );
            }
        }
    }

    auto answer = m_houseModeProcessor->handleAddDevice( deviceId, addParams );
    return answer;
}

JsonObject CoreService::handleModifyDevice( const JsonObject& params ) {
    std::string deviceId;
    JsonObject device;

    if( !json_get( params, RPCParams::DeviceId, deviceId ) || !json_get( params, RPCParams::Device, device ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    auto result = m_deviceManager->modifyDevice( deviceId, device );

    try {
        JsonObject modifyDeviceResult = m_houseModeProcessor->handleModifyDevice( deviceId, device );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( modifyDeviceResult ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( CommonUtils::whatError( ex ) ) );
        JsonObject error;
        setError( error, Errors::INTERNAL_OPERATION_ERROR );
        return error;
    }

    return result;
}

JsonObject CoreService::handleAddItem( const JsonObject& request ) {
    LOGGER_TRACE( cc::j( request ) );
    DeviceManager::ItemID response = m_deviceManager->addItem( request );
    if( !hasError( response ) ) {
        const auto& itemId = json_get_str( response, "id" );
        const auto& deviceId = json_get_str( request, ItemFields::DeviceIdFieldName );
        auto addDeviceToHouseModesCallback = [this]( const std::string& deviceId, const JsonObject& device ) -> Json {
            return this->addDeviceToHouseModes( deviceId, device );
        };

        auto result = m_houseModeProcessor->handleAddItem(
            deviceId, itemId, request, std::move( addDeviceToHouseModesCallback ) );
        setErrorIfHas( response, result );
    }

    return response;
}

JsonObject CoreService::handleModifyItem( const JsonObject& params ) {
    std::string itemId;
    JsonObject item;

    if( !json_get( params, RPCParams::ItemId, itemId ) || !json_get( params, RPCParams::Item, item ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    JsonObject existingItem = m_deviceManager->fetchItem( itemId );
    if( !existingItem.empty() ) {
        JsonObject modificationResult = m_deviceManager->modifyItem( itemId, item );
        if( modificationResult.count( "error" ) == 0 ) {
            // fetch previous value of item and pass it to houseModesProc::handleModifyItem, because it will be
            // updated in the db already
            Json modeModificationResult = m_houseModeProcessor->handleModifyItem(
                json_get_str( item, ItemFields::DeviceIdFieldName ), itemId, item, existingItem );

            if( json_get_bool( modeModificationResult, "SetDeviceArmedFalse", false ) ) {
                LOGGER_DEBUG( cc::debug( "Set device.armed to false" ) );
                namespace Fields = Model::Resources::ItemFields;
                auto armedParams = JSON( db::ID_FIELD_NAME << json_get_str( existingItem, Fields::DeviceIdFieldName )
                                                           << Model::Resources::DeviceFields::ArmedFieldName << false );
                m_deviceManager->setDeviceArmed( armedParams );
            }
        }
        return modificationResult;
    }

    JsonObject result;
    setError( result, Errors::WRONG_PARAMS );
    return result;
}

JsonObject CoreService::handleModifyDeviceSetting( const JsonObject& params ) {
    std::string settingId;
    JsonObject setting;

    if( !json_get( params, RPCParams::SettingId, settingId ) || !json_get( params, RPCParams::Setting, setting ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    return m_deviceManager->modifySetting( settingId, setting );
}

JsonObject CoreService::handleRemoveItem( const ItemID& itemId ) {
    auto item = getItem( itemId );
    auto modesResult = m_houseModeProcessor->handleRemoveItem( item );
    removeItem( itemId );

    if( json_get_bool( modesResult, "SecurityThreatRemoved", false ) ) {
        LOGGER_DEBUG( cc::debug( "The security_threat item removed, set device.armed to false" ) );
        Json deviceFields = JSON( db::ID_FIELD_NAME << json_get_str( item, ItemFields::DeviceIdFieldName )
                                                    << DeviceFields::ArmedFieldName << false );
        m_deviceManager->setDeviceArmed( deviceFields );
    }
    return JSON_EMPTY_OBJ;
}

JsonObject CoreService::handleRemovePolicy( const PolicyID& policyId ) {
    /* we dont do anything yet */
    return JSON_EMPTY_OBJ;
}

JsonObject CoreService::handleSetHouseMode( const JsonObject& params ) {
    try {
        std::string modeId;
        if( !json_get( params, CoreDHouseModes::HOUSE_MODE_ENTRY, modeId ) ) {
            JsonObject result;
            setError( result, Errors::WRONG_PARAMS );
            return result;
        }

        JsonObject result = m_houseModeProcessor->handleSetHouseMode( modeId );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( result ) );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( CommonUtils::whatError( ex ) ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

bool CoreService::setHubTimeZone( const std::string& zoneName ) {
    try {
#if defined( __HUB_TYPE_VHUB__ )
        system_utils::DebianTimeZone().setTimeZone( zoneName );
#else
        system_utils::OpenWrtTimeZone().setTimeZone( zoneName );
#endif
        return true;
    }
    catch( std::exception& e ) {
        LOGGER_ERROR( cc::error( "Failed to set hub timezone: " ) + cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Failed to set hub timezone: unknown exception" ) );
    }
    return false;
}

JsonObject CoreService::handleSetHubTimeZone( const JsonObject& params ) {
    JsonObject result;
    std::string locationParam;

    if( json_get( params, HubPlatform::Uid::FieldNames::TimeLocationFieldName, locationParam ) ) {
        bool restartFirmware;
        if( !json_get( params, "restart_firmware", restartFirmware ) ) {
            restartFirmware = true;
        }

        getDBClient().update(
            HUBSettings,
            JSON_EMPTY_OBJ,
            JSON( db::ID_FIELD_NAME << CoreDRemoteInterface::SettingsFields::ZONE_NAME << "value" << locationParam ),
            true,
            false );

        setHubTimeZone( locationParam );
        sendBroadcast( HUB_SYS_TIME_UPDATED, JSON_EMPTY_OBJ, GroupsNames::SYSTEM_GROUP );
        m_swingerShutdownDevicesManager->clean();
        if( restartFirmware ) {
            Log::info( "Restarting..." );
            std::thread thread( []() {
                std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
                system( ( HubPlatform::Environment::getBinariesFolder() + "/updatecmd --daemon --restart_firmware" )
                            .c_str() );
            } );
            thread.detach();
        }
        return result;
    }
    setError( result, Errors::WRONG_PARAMS );
    return result;
}

JsonObject CoreService::handleCancelSwitchHouseMode( const JsonObject& /*params*/ ) {
    try {
        m_houseModeProcessor->cancelSwitchingHouseMode();
        LOGGER_DEBUG( "" );
        return JSON( "modeId" << CoreDHouseModes::getCurrentHouseModeId( getDBClient() ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( CommonUtils::whatError( ex ) ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleCancelEntryDelay( const JsonObject& /*params*/ ) {
    try {
        m_houseModeAlarms->cancelEntryDelay();
        return JSON_EMPTY_OBJ;
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( CommonUtils::whatError( ex ) ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleSkipEntryDelay( const JsonObject& /*params*/ ) {
    try {
        LOGGER_INFO_RAW( cc::info( "Entered handle skip entry delay method" ) );
        auto result = m_houseModeAlarms->skipEntryDelay();
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to skip entry delay: " ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleSetEntryDelay( const JsonObject& params ) {
    try {
        JsonObject result;
        LOGGER_INFO_RAW( cc::info( "Entered handle set entry delay method" ) );
        result = checkEntryDelayObject( params );
        if( !result.empty() ) {
            return result;
        }
        result = m_houseModeAlarms->setEntryDelay( params );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to set entry delay: " ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleResetEntryDelay( const JsonObject& /*params*/ ) {
    try {
        JsonObject result;
        LOGGER_INFO_RAW( cc::info( "Entered handle reset entry delay method" ) );
        result = m_houseModeAlarms->resetEntryDelay();
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to set entry delay: " ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::checkEntryDelayObject( const JsonObject& params ) {
    const std::string entryDelayKeys[] = { "normal", "longExtended", "extended", "instant" };
    JsonObject result;
    auto size = params.size();
    if( size != sizeof( entryDelayKeys ) / sizeof( entryDelayKeys[ 0 ] ) ) {
        LOGGER_ERROR( cc::error( "Request not contain four object keys" ) );
        setError( result, Errors::WRONG_PARAMS, "four params keys not found" );
        return result;
    }

    for( const auto& entryDelayKey : entryDelayKeys ) {
        auto keyPresent = json_contains( params, entryDelayKey );
        if( !keyPresent ) {
            LOGGER_ERROR( cc::error( "Object key: " + entryDelayKey + " not found in request" ) );
            auto description = "key: " + entryDelayKey + " is not found";
            setError( result, Errors::KEY_NOT_FOUND, description );
            return result;
        }
    }

    for( const auto& entryDelayKey : entryDelayKeys ) {
        auto value = json_get_int( params, entryDelayKey );
        if( entryDelayKey != "instant" ) {
            if( value < 30 || value > 240 ) {
                LOGGER_ERROR(
                    cc::error( entryDelayKey + " :value is out of range, value: " + std::to_string( value ) ) );
                auto description =
                    "Value: " + std::to_string( value ) + " for key: " + entryDelayKey + " is out of range";
                setError( result, Errors::INVALID_VALUE_RANGE, description );
                return result;
            }
        }
        else {
            if( value > 0 ) {
                LOGGER_ERROR_RAW( cc::error( "Entry Delay value is constant and can't be changed, : " ) +
                                  cc::note( entryDelayKey ) + ": " + cc::num( value ) );
                auto description = "Value: " + std::to_string( value ) + " for key: " + entryDelayKey + " is not valid";
                setError( result, Errors::CONSTANT_PARAMS, description );
                return result;
            }
        }
    }
    return JSON_EMPTY_OBJ;
}

JsonObject CoreService::handleAddDisarmedDevice( const JsonObject& params ) {
    try {
        std::string modeId, deviceId;
        if( !json_get( params, "modeId", modeId ) || !json_get( params, "deviceId", deviceId ) ) {
            JsonObject result;
            setError( result, Errors::WRONG_PARAMS );
            return result;
        }

        JsonObject result = m_houseModeProcessor->handleAddDisarmedDevice( modeId, deviceId );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( result ) );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "unexpected error :" ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleRemoveDisarmedDevice( const JsonObject& params ) {
    try {
        std::string modeId, deviceId;
        if( !json_get( params, "modeId", modeId ) || !json_get( params, "deviceId", deviceId ) ) {
            JsonObject result;
            setError( result, Errors::WRONG_PARAMS );
            return result;
        }

        JsonObject result = m_houseModeProcessor->handleRemoveDisarmedDevice( modeId, deviceId, true );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( result ) );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "unexpected error :" ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

void CoreService::removeDisarmedDevice( const DeviceID& deviceId ) {
    auto houseModes = CoreDHouseModes::getHouseModesList( getDBClient() );
    for( auto& houseMode : houseModes ) {
        const auto houseModeId = houseMode.m_id;
        CoreDHouseModes::removeDisarmedDevice( *this, getDevicesDbModifier(), houseModeId, deviceId );
    }
}

JsonObject CoreService::handleAddAlarmsOffDevice( const JsonObject& params ) {
    JsonObject answer;
    try {
        std::string modeId, deviceId;
        if( !getNonEmptyField( params, "modeId", answer, modeId ) ) {
            return answer;
        }
        if( !getNonEmptyField( params, "deviceId", answer, deviceId ) ) {
            return answer;
        }

        answer = m_houseModeProcessor->handleAddAlarmsOffDevice( modeId, deviceId );
        LOGGER_DEBUG_RAW( cc::debug( "handleAddAlarmsOffDevice, answer :" ) + cc::j( answer ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR );
    }
    return answer;
}

JsonObject CoreService::handleRemoveAlarmsOffDevice( const JsonObject& params ) {
    JsonObject answer;
    try {
        std::string modeId, deviceId;
        if( !getNonEmptyField( params, "modeId", answer, modeId ) ) {
            return answer;
        }
        if( !getNonEmptyField( params, "deviceId", answer, deviceId ) ) {
            return answer;
        }

        answer = m_houseModeProcessor->handleRemoveAlarmsOffDevice( modeId, deviceId );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( answer ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR );
    }
    return answer;
}

void CoreService::removeAlarmsOffDevice( const DeviceID& deviceId ) {
    auto houseModes = CoreDHouseModes::getHouseModesList( getDBClient() );
    for( auto& houseMode : houseModes ) {
        const auto houseModeId = houseMode.m_id;
        CoreDHouseModes::removeAlarmsOffDevice( getDBClient(), houseModeId, deviceId );
    }
}

JsonObject CoreService::handleAddCamerasOffDevice( const JsonObject& params ) {
    JsonObject answer;
    try {
        std::string modeId, deviceId;
        if( !getNonEmptyField( params, "modeId", answer, modeId ) ) {
            LOGGER_ERROR( cc::error( "Wrong modeId field, return answer: " ) + cc::j( answer ) );
            return answer;
        }
        if( !getNonEmptyField( params, "deviceId", answer, deviceId ) ) {
            LOGGER_ERROR( cc::error( "CoreService: Wrong deviceId field, return answer: " ) + cc::j( answer ) );
            return answer;
        }

        answer = m_houseModeProcessor->handleAddCamerasOffDevice( modeId, deviceId );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( answer ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR );
        LOGGER_ERROR( cc::error( "CoreService: Exception, return answer: " ) + cc::j( answer ) );
    }
    return answer;
}

JsonObject CoreService::handleRemoveCamerasOffDevice( const JsonObject& params ) {
    JsonObject answer;
    try {
        std::string modeId, deviceId;
        if( !getNonEmptyField( params, "modeId", answer, modeId ) ) {
            return answer;
        }
        if( !getNonEmptyField( params, "deviceId", answer, deviceId ) ) {
            return answer;
        }

        answer = m_houseModeProcessor->handleRemoveCamerasOffDevice( modeId, deviceId );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( answer ) );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( ex.what() ) );
        setError( answer, Errors::INTERNAL_OPERATION_ERROR );
    }
    return answer;
}

void CoreService::removeCamerasOffDevice( const DeviceID& deviceId ) {
    auto houseModes = CoreDHouseModes::getHouseModesList( getDBClient() );
    for( auto& houseMode : houseModes ) {
        const auto houseModeId = houseMode.m_id;
        CoreDHouseModes::removeCamerasOffDevice( getDBClient(), houseModeId, deviceId );
    }
}

JsonObject CoreService::handleAddBypassDevices( const JsonObject& params ) {
    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject answer;
    std::tie( isGood, deviceIds, answer ) = CoreDHouseModes::readDeviceIdsParams( params );

    std::string modeId;
    if( isGood && getNonEmptyField( params, "modeId", answer, modeId ) ) {
        try {
            answer = m_houseModeProcessor->handleAddBypassDevices( modeId, deviceIds );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "unexpected error :" ) + cc::note( ex.what() ) );
            setError( answer, Errors::INTERNAL_OPERATION_ERROR );
        }
    }

    LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( answer ) );
    return answer;
}

JsonObject CoreService::handleRemoveBypassDevices( const JsonObject& params ) {
    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject answer;
    std::tie( isGood, deviceIds, answer ) = CoreDHouseModes::readDeviceIdsParams( params );

    std::string modeId;
    if( isGood && getNonEmptyField( params, "modeId", answer, modeId ) ) {
        try {
            answer = m_houseModeProcessor->handleRemoveBypassDevices( modeId, deviceIds );
        }
        catch( const std::exception& ex ) {
            LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( ex.what() ) );
            setError( answer, Errors::INTERNAL_OPERATION_ERROR );
        }
    }

    LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( answer ) );
    return answer;
}

void CoreService::removeBypassDevice( const DeviceID& deviceId ) {
    auto houseModes = CoreDHouseModes::getHouseModesList( getDBClient() );
    for( auto& houseMode : houseModes ) {
        const auto houseModeId = houseMode.m_id;
        CoreDHouseModes::removeBypassDevices( *this, houseModeId, { deviceId } );
    }
}

void CoreService::removeProtectButton( const CoreService::DeviceID& deviceId ) {
    using namespace CoreDHouseModes;

    auto found = findProtectButtonById( getDBClient(), deviceId );
    if( found.second == true ) {
        CoreDHouseModes::removeProtectButton( getDBClient(), deviceId );
    }
}

JsonObject CoreService::handleSetDisarmedDefault( const JsonObject& params ) {
    try {
        std::string modeId;
        bool disarmedDefault;
        if( !json_get( params, "modeId", modeId ) || !json_get( params, "disarmedDefault", disarmedDefault ) ) {
            JsonObject result;
            setError( result, Errors::WRONG_PARAMS );
            return result;
        }

        JsonObject result = m_houseModeProcessor->handleSetDisarmedDefault( modeId, disarmedDefault );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( result ) );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG( cc::debug( "unexpected error :" ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleHouseModeProtectSet( const JsonObject& params ) {
    const auto modeId = json_get_str( params, "modeId" );
    const auto protect = json_get_bool( params, "protect" );

    try {
        if( json_contains( params, "timestamp" ) ) {
            const auto timestamp = std::chrono::milliseconds( json_get_int64( params, "timestamp" ) );
            return m_houseModeProcessor->handleSetProtectWithTimestamp( modeId, protect, timestamp );
        }
        else {
            return m_houseModeProcessor->handleSetProtect( modeId, protect );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to turn " ) + cc::note( protect ? "on" : "off" ) +
                          cc::error( " an Ezlo Protect mode for a house mode " ) + cc::note( modeId ) +
                          cc::error( ", " ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleHouseModeProtectButtonsSet( const JsonObject& params ) {
    JsonObject result;
    auto deviceIdIt = params.find( "deviceId" );
    auto serviceIt = params.find( "service" );
    if( deviceIdIt == params.end() || !deviceIdIt->is_string() || deviceIdIt->empty() ) {
        setError( result, Errors::BAD_PARAMS, "Wrong params, deviceId" );
        return result;
    }
    const auto& deviceId = *deviceIdIt;
    if( serviceIt != params.end() && !serviceIt->is_string() ) {
        setError( result, Errors::BAD_PARAMS, "Wrong params, service" );
        return result;
    }
    auto protectServiceEnum = CoreDHouseModes::ProtectServiceEnum::NONE;
    std::string protectService;
    if( serviceIt != params.end() ) {
        protectService = json_get_str( *serviceIt );
        protectServiceEnum = CoreDHouseModes::toProtectService( protectService );

        if( CoreDHouseModes::ProtectServiceEnum::NONE == protectServiceEnum && !protectService.empty() ) {
            JsonObject error = JSON_EMPTY_OBJ;
            Errors::getWrongParams( "service", error );
            setErrorObject( result, error, "Format of a required field is wrong" );
            return result;
        }
    }

    try {
        return m_houseModeProcessor->handleProtectButtonsSet( deviceId, protectServiceEnum );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to set protectButton for device id: " ) + cc::note( deviceId ) +
                          cc::error( ", protect service:  " ) + cc::note( protectService ) + cc::error( ", " ) +
                          cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

JsonObject CoreService::handleAddProtectDevices( const JsonObject& params ) {
    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject result;
    std::tie( isGood, deviceIds, result ) = CoreDHouseModes::readDeviceIdsParams( params );
    if( isGood ) {
        try {
            return m_houseModeProcessor->handleAddProtectDevices( deviceIds );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to add protectDevices: " ) + cc::note( std::string( ex.what() ) ) );
            setError( result, Errors::INTERNAL_OPERATION_ERROR );
        }
    }
    return result;
}

JsonObject CoreService::handleRemoveProtectDevices( const JsonObject& params ) {
    bool isGood;
    std::vector< std::string > deviceIds;
    JsonObject result;

    std::tie( isGood, deviceIds, result ) = CoreDHouseModes::readDeviceIdsParams( params );
    if( isGood ) {
        try {
            return m_houseModeProcessor->handleRemoveProtectDevices( deviceIds );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to remove protectDevices: " ) + cc::note( std::string( ex.what() ) ) );
            setError( result, Errors::INTERNAL_OPERATION_ERROR );
        }
    }
    return result;
}

JsonObject CoreService::handleSetServiceNotifications( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    JsonArray ids = JSON_EMPTY_ARR;
    if( !getFieldAsArray( params, "ids", answer, ids ) ) {
        LOGGER_DEBUG( cc::debug( "no ids" ) );
        return answer;
    }
    for( auto& id : ids ) {
        if( !id.is_string() ) {
            setError( answer, Errors::WRONG_FIELD );
            return answer;
        }
    }
    std::vector< DeviceManager::DeviceID > devices;
    {
        const auto fieldsToReturn = JSON( DeviceFields::IdFieldName << 1 );
        const auto result = getDBClient().findAll( DBNames::DevicesDB, JSON_EMPTY_OBJ, &fieldsToReturn );
        devices.resize( result.size() );
        std::transform( result.begin(), result.end(), devices.begin(), []( const JsonObject& v ) {
            return v[ DeviceFields::IdFieldName ];
        } );
    }
    std::sort( devices.begin(), devices.end() );
    std::sort( ids.begin(), ids.end() );

    // set serviceNotification field to TRUE for devices are specified in entry array
    std::size_t size = ( ids.size() > devices.size() ) ? ids.size() : devices.size();
    std::vector< DeviceManager::DeviceID > toUpdate( size );
    auto last = std::set_intersection( devices.begin(), devices.end(), ids.begin(), ids.end(), toUpdate.begin() );
    toUpdate.resize( last - toUpdate.begin() );
    if( toUpdate.size() != ids.size() ) {
        setError( answer, Errors::NO_DEVICE_FOUND );
        return answer;
    }
    JsonObject serviceNotificationUpdate = JSON( DeviceFields::ServiceNotificationFieldName << true );
    for( const auto& id : toUpdate ) {
        m_devicesDb->update( id, serviceNotificationUpdate );
    }

    // set serviceNotification field to FALSE for devices are NOT specified in entry array
    size = devices.size() - toUpdate.size();
    toUpdate.resize( size );
    std::set_difference( devices.begin(), devices.end(), ids.begin(), ids.end(), toUpdate.begin() );
    serviceNotificationUpdate[ DeviceFields::ServiceNotificationFieldName ] = false;
    for( const auto& id : toUpdate ) {
        m_devicesDb->update( id, serviceNotificationUpdate );
    }
    return answer;
}

JsonObject CoreService::handleUpdateDeviceFirmware( const JsonObject& params ) {
    JsonObject answer = JSON_EMPTY_OBJ;
    auto updatesIt = params.find( "updates" );
    if( updatesIt == params.end() ) {
        setError( answer, Errors::EMPTY_FIELD, "empty updates field" );
        return answer;
    }
    struct Gateway {
        std::string script;
        std::set< DeviceID > deviceIds;
    };
    struct Update {
        JsonObject data;
        std::map< GatewayID, Gateway > gateway;
    };
    std::vector< Update > updates;
    std::vector< DeviceID > missedDeviceIds;
    for( auto& updateField : *updatesIt ) {
        auto devicesIt = updateField.find( "devices" );
        if( devicesIt == updateField.end() ) {
            setError( answer, Errors::EMPTY_FIELD, "empty devices field" );
            return answer;
        }
        auto firmwareIt = updateField.find( "firmware" );
        if( firmwareIt == updateField.end() ) {
            setError( answer, Errors::EMPTY_FIELD, "empty firmware field" );
            return answer;
        }
        Update update;
        update.data = *firmwareIt;
        for( auto& id : *devicesIt ) {
            auto deviceId = json_get_str( id );
            auto fieldsToReturn = JSON( DeviceFields::GatewayIdFieldName << 1 );
            const auto dbDevice = getDBClient().findOne(
                DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), &fieldsToReturn );
            if( dbDevice.empty() ) {
                missedDeviceIds.push_back( deviceId );
                continue;
            }
            auto gatewayId = json_get_str( dbDevice, DeviceFields::GatewayIdFieldName );
            fieldsToReturn = JSON( GatewayFields::UpdateDeviceFirmwareCommandFieldName << 1 );
            const auto dbGateway = getDBClient().findOne(
                DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayId ), &fieldsToReturn );
            if( dbGateway.empty() ) {
                setError( answer, Errors::NO_GATEWAY_FOUND, "gateway id: " + gatewayId );
                return answer;
            }
            auto& updateGateway = update.gateway[ gatewayId ];
            updateGateway.deviceIds.insert( deviceId );
            if( updateGateway.script.empty() ) {
                updateGateway.script = json_get_str( dbGateway, GatewayFields::UpdateDeviceFirmwareCommandFieldName );
            }
        }
        updates.emplace_back( std::move( update ) );
    }

    if( !missedDeviceIds.empty() ) {
        std::string deviceIds;
        for( const auto& id : missedDeviceIds ) {
            deviceIds += id + ", ";
        }
        deviceIds.pop_back();
        deviceIds.pop_back();
        setError( answer, Errors::DEVICES_NOT_FOUND, "device ids: " + deviceIds );
        return answer;
    }

    for( auto& update : updates ) {
        for( auto& gatewayPair : update.gateway ) {
            Json data;
            auto gateway = gatewayPair.second;
            data[ "devices" ] = gateway.deviceIds;
            data[ "firmware" ] = update.data;
            sendExecutePluginScript( gateway.script, data );
        }
    }
    return JSON_EMPTY_OBJ;
}

void CoreService::eraseDisarmedDevice( const DeviceID& deviceId ) {
    CoreDHouseModes::eraseDisarmedDevice( getDBClient(), deviceId );
}

JsonObject CoreService::handleSetNotificationsForHouseMode( const JsonObject& params ) {
    try {
        std::string modeId;
        bool notifyAll;
        JsonArray userIdsJson;
        if( !json_get( params, "modeId", modeId ) || !json_get( params, "all", notifyAll ) ||
            !json_get_arr( params, "userIds", userIdsJson ) ) {
            JsonObject result;
            setError( result, Errors::WRONG_PARAMS );
            return result;
        }

        std::vector< std::string > userIds = json_get_vec< std::string >( userIdsJson );
        JsonObject result = m_houseModeProcessor->handleSetNotifications( modeId, notifyAll, userIds );
        LOGGER_DEBUG( cc::debug( "answer :" ) + cc::j( result ) );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "unexpected error :" ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

int CoreService::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& tableName : DBNames::BackupTableNames ) {
        if( tableName == Model::Resources::DBNames::ItemsDB ) {
            if( !helper.restoreDatabase( tableName, []( Json& item ) {
                    Model::Item::encryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), item );
                } ) ) {
                return helper.getReturnCode();
            }
        }
        else {
            if( !helper.restoreDatabase( tableName ) ) {
                return helper.getReturnCode();
            }
        }
    }

    return helper.getReturnCode();
}

void CoreService::handleRemovePluginGateways( const JsonObject& json ) {
    auto pluginId = json_get_str( json, RPCParams::PluginId );
    removePluginGateways( pluginId );
}

void CoreService::handleRemoveGatewayDevices( const JsonObject& json ) {
    auto gatewayId = json_get_str( json, RPCParams::GatewayId );
    removeGatewayDevices( gatewayId );
}

void CoreService::handleRemoveDevice( const JsonObject& json ) {
    auto deviceId = json_get_str( json, RPCParams::DeviceId );
    removeDevice( deviceId );
}

void CoreService::handleRenameDevice( const JsonObject& json ) {
    LOGGER_DEBUG_RAW( "CoreDRemoteInterface::rename device  " );

    const auto deviceId = json_get_str( json, RPCParams::DeviceId );
    const auto name = json_get_str( json, DeviceFields::NameFieldName );

    m_devicesDb->update( deviceId, JSON( DeviceFields::NameFieldName << name ) );
}

bool CoreService::handleSetItemValue( const JsonObject& json, const std::string& operationId ) {
    LOGGER_DEBUG( cc::debug( "Setting item value: " ) + cc::j( json ) );
    const JsonArray& ids = json[ "ids" ];
    const JsonArray& items = json[ "items" ];
    Json source = JSON_EMPTY_OBJ;
    json_get( json, RPCParams::SourceField, source );

    if( !ids.empty() ) {
        LOGGER_DEBUG( cc::debug( "Setting item values for multicast" ) );
        return handleSetMultipleItemValues( ids, json[ "value" ], operationId, source );
    }
    else if( !items.empty() ) {
        LOGGER_DEBUG( cc::debug( "Setting item values for different items" ) );
        return handleSetDifferentItemValues( items, operationId, source );
    }
    else {
        LOGGER_DEBUG( cc::debug( "Setting item value" ) );
        return handleSetSingleItemValue( json, operationId, source );
    }
    return false;
}

void CoreService::handleUpdateItemField( const JsonObject& json ) {
    LOGGER_DEBUG( cc::debug( "Updating item string restriction: " ) + cc::j( json ) );

    const ItemID itemId = json_get_str( json, RPCParams::ItemId );
    const auto itemFieldValueIt = json.find( RPCParams::ItemFieldValue );
    auto itemField = json_get_str( json, RPCParams::ItemField );

    if( itemId.empty() ) {
        LOGGER_WARN( cc::debug( "item id is bad" ) );
        return;
    }

    if( itemFieldValueIt == json.end() ) {
        LOGGER_WARN( cc::debug( std::string( RPCParams::ItemFieldValue ) + " missing" ) );
        return;
    }

    if( itemField != ItemFields::StringRestrictionFieldName && itemField != ItemFields::UserCodeRestrictionFieldName ) {
        LOGGER_WARN( cc::debug( "itemField " + itemField + " is bad" ) );
        return;
    }

    JsonObject result;

    result[ std::move( itemField ) ] = *itemFieldValueIt;

    m_itemsDb->update( itemId, result );
}

void CoreService::handleUpdateItemValue( const JsonObject& json ) {
    LOGGER_DEBUG( cc::debug( "Updating item value: " ) + cc::j( json ) );
    const auto valueIt = json.find( RPCParams::ItemValue );
    const ItemID itemId = json_get_str( json, RPCParams::ItemId );
    const auto valueType = json_get_str( json, RPCParams::ItemType );

    if( itemId.empty() ) {
        LOGGER_WARN( cc::warn( "item id is bad" ) );
        return;
    }

    if( valueIt == json.end() ) {
        LOGGER_WARN( cc::warn( std::string( RPCParams::ItemValue ) + " missing" ) );
        return;
    }

    JsonObject result;

    result[ ItemFields::ValueFieldName ] = valueIt.value();

    auto valueMinIt = json.find( RPCParams::ItemValueMin );
    if( valueMinIt != json.end() ) {
        result[ ItemFields::ValueMinFieldName ] = valueMinIt.value();
    }

    auto valueMaxIt = json.find( RPCParams::ItemValueMax );
    if( valueMaxIt != json.end() ) {
        result[ ItemFields::ValueMaxFieldName ] = valueMaxIt.value();
    }

    result[ ItemFields::TypeFieldName ] = valueType;
    Model::Item::encryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), result );
    json_remove( result, ItemFields::TypeFieldName );

    m_itemsDb->update( itemId, result );
    m_houseModeAlarms->checkSecurityThreat( itemId );
}

DbModifier::Dictionary CoreService::getDictionaryOperation( const std::string& key,
                                                            const JsonObject& oldValue,
                                                            const JsonObject& newValue ) const {
    if( newValue.empty() )
        return { key, oldValue, "removed" };
    else if( oldValue.empty() )
        return { key, newValue, "added" };
    else
        return { key, newValue, "updated" };
}

JsonObject CoreService::getItem( const ItemID& itemId ) const {
    JsonObject item;
    try {
        const auto& db = getDBClient();
        const auto filter = JSON( ItemFields::IdFieldName << itemId );
        const auto element = getDBClient().findOne( DBNames::ItemsDB, filter, nullptr );
        const auto elementDynamic = getDBClient().findOne( DBNames::TempItemsDB, filter, nullptr );

        item = JSON( element << elementDynamic );
        Model::Item::checkItemAsJson( item );
        Model::Item::decryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), item );
    }
    catch( const std::exception& exception ) {
        Log::error( cc::error( "CoreService::getItem: " ) + cc::note( exception.what() ) );
        item = JSON_EMPTY_OBJ;
    }
    catch( ... ) {
        Log::error( cc::error( "CoreService::getItem: bad item's entry in the db is corrupted" ) );
        item = JSON_EMPTY_OBJ;
    }
    return item;
}

JsonObject CoreService::getSetting( const DeviceSettingID& settingId ) const {
    try {
        const auto& db = getDBClient();
        const auto filter = JSON( DeviceSettingFields::Id << settingId );
        return getDBClient().findOne( DBNames::DevicesSettingsDB, filter, nullptr );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreService::getSetting: " } + exception.what() );
        return JSON_EMPTY_OBJ;
    }
    catch( ... ) {
        Log::error( "CoreService::getSetting: bad setting entry in the db is corrupted" );
        return JSON_EMPTY_OBJ;
    }
}

Json CoreService::getDeviceGateway( const std::string& itemId ) {
    auto& db = getDBClient();
    const JsonObject item = db.findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item.empty() )
        throw ItemNotFoundError{ itemId };

    const DeviceID deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() )
        throw DeviceNotFoundError{ deviceId };

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );
    if( gateway.empty() )
        throw GatewayNotFoundError{ gatewayID };

    return gateway;
}

bool CoreService::isCustomResponse( Json& request, const std::string& operationId, const Json& gateway ) {
    if( !operationId.empty() && json_get_str( gateway, GatewayFields::SetItemValueResponsePolicyFieldName ) ==
                                    SetItemValueResponsePolicy::Custom ) {
        request[ "operation_id" ] = operationId;
        return true;
    }
    return false;
}

std::string CoreService::getScript( const Json& gateway ) {
    auto script = json_get_str( gateway, GatewayFields::SetItemValueCommandFieldName );
    if( script.empty() ) {
        Log::warn( cc::warn( "Script for changing item values is not present for gateway: " ) +
                   cc::note( json_get_str( gateway, GatewayFields::IdFieldName ) ) );
    }
    return script;
}

void CoreService::sendDeviceBroadcast( MessageSubClasses subClass, const Json& json ) {
    addTask( [this, subClass, json]() {
        switch( subClass ) {
        case HUB_DEVICE_ADDED:
            emit( ScriptEvents::DeviceAdded( json ) );
            break;
        case HUB_DEVICE_UPDATED:
            emit( ScriptEvents::DeviceUpdated( json ) );
            break;
        case HUB_DEVICE_REMOVED: {
            emit( ScriptEvents::DeviceRemoved( json ) );
            swingerShutdownDeviceRemovedHandler( json );
            break;
        }
        default:;
        }
    } );
    sendBroadcast( subClass, json, GroupsNames::DEVICE_GROUP );
}

void CoreService::sendItemBroadcast( MessageSubClasses subClass, const Json& json ) {
    // todo: re-design is required: cloud.videoforward request must be sent to the cloud when doorbell_state is "ring"
    //       it can be implemented either as a binding of the "cameras" module (to be called during update_value event
    //       processing) or at some other layer of abstraction
    sendCloudVideoforwardRequest( subClass, json );
    addTask( [this, subClass, json]() {
        switch( subClass ) {
        case HUB_ITEM_ADDED:
            emit( ScriptEvents::ItemAdded( json ) );
            break;
        case HUB_ITEM_UPDATED:
            emit( ScriptEvents::ItemUpdated( json ) );
            swingerShutdownItemUpdatedHandler( json );
            break;
        case HUB_ITEM_REMOVED:
            emit( ScriptEvents::ItemRemoved( json ) );
            swingerShutdownItemRemovedHandler( json );
            break;
        case HUB_ITEM_DICTIONARY_UPDATED:
            emit( ScriptEvents::ItemDictionaryUpdated( json ) );
            break;
        default:;
        }
    } );
    if( subClass == EXTENDED_HUB_ITEM_UPDATED ) {
        sendBroadcast( subClass, json, GroupsNames::INTERNAL_BROADCAST_GROUP );
    }
    else {
        sendBroadcast( subClass, json, GroupsNames::DEVICE_GROUP );
    }
}

void CoreService::sendPolicyBroadcast( MessageSubClasses subClass, const Json& json ) {
    sendBroadcast( subClass, json, GroupsNames::BROADCAST_GROUP );
}

void CoreService::sendGatewaySettingBroadcast( MessageSubClasses subClass, const Json& json ) {
    addTask( [this, subClass, json]() {
        switch( subClass ) {
        case HUB_GATEWAY_SETTING_ADDED:
            emit( ScriptEvents::GatewaySettingAdded( json ) );
            break;
        case HUB_GATEWAY_SETTING_UPDATED:
            emit( ScriptEvents::GatewaySettingUpdated( json ) );
            break;
        case HUB_GATEWAY_SETTING_REMOVED:
            emit( ScriptEvents::GatewaySettingRemoved( json ) );
            break;
        default:;
        }
    } );
    sendBroadcast( subClass, json, GroupsNames::DEVICE_GROUP );
}

void CoreService::sendDeviceSettingBroadcast( MessageSubClasses subClass, const Json& json ) {
    addTask( [this, subClass, json]() {
        switch( subClass ) {
        case HUB_DEVICE_SETTING_ADDED:
            emit( ScriptEvents::DeviceSettingAdded( json ) );
            break;
        case HUB_DEVICE_SETTING_UPDATED:
            emit( ScriptEvents::DeviceSettingUpdated( json ) );
            break;
        case HUB_DEVICE_SETTING_REMOVED:
            emit( ScriptEvents::DeviceSettingRemoved( json ) );
            break;
        case HUB_DEVICE_SETTING_DICTIONARY_UPDATED:
            emit( ScriptEvents::DeviceSettingDictionaryUpdated( json ) );
            break;
        default:;
        }
    } );
    sendBroadcast( subClass, json, GroupsNames::DEVICE_GROUP );
}

void CoreService::sendDeviceItemGroupBroadcast( MessageSubClasses subClass, const Json& json ) {
    sendBroadcast( subClass, json, GroupsNames::DEVICE_GROUP );
}

bool CoreService::handleSetMultipleItemValues( const JsonArray& ids,
                                               const JsonObject& value,
                                               const std::string& operationId,
                                               const Json& source ) {
    bool customResponse = false;
    if( !ids.is_array() ) {
        Log::error( "\"ids\" parameter should be array" );
        return customResponse;
    }
    std::unordered_map< std::string, JsonObject > scriptRequests;
    std::set< JsonObject > withoutDuplicates;
    for( const auto& itemId : ids ) {
        if( !withoutDuplicates.insert( itemId ).second ) {
            continue;
        }
        auto gateway = getDeviceGateway( itemId );
        auto script = getScript( gateway );
        if( script.empty() ) {
            continue;
        }
        auto scriptIt = scriptRequests.find( script );
        if( scriptIt == scriptRequests.end() ) {
            Json itemIds = JSON_ARRAY_( itemId );
            JsonObject request = JSON( "item_ids" << itemIds << "value" << value );
            if( !source.empty() )
                request[ "source" ] = source;

            customResponse |= isCustomResponse( request, operationId, gateway );
            if( script == GatewayBuiltInScripts::SetItemValueScript ) {
                JsonObject updateParams = makeValueForUpdate( itemId, { { ItemFields::ValueFieldName, value } } );
                scriptRequests.emplace( std::move( script ), std::move( updateParams ) );
            }
            else {
                scriptRequests.emplace( std::move( script ), std::move( request ) );
            }
        }
        else {
            assert( scriptIt->second[ "item_ids" ].is_array() );
            scriptIt->second[ "item_ids" ].push_back( itemId );
        }
    }

    for( const auto& scriptRequest : scriptRequests ) {
        const std::string& script = scriptRequest.first;
        if( script == GatewayBuiltInScripts::SetItemValueScript ) {
            auto& updateParams = scriptRequest.second;
            handleUpdateItemValue( updateParams );
        }
        else {
            sendExecutePluginScript( script, scriptRequest.second );
        }
    }
    return customResponse;
}

bool CoreService::handleSetDifferentItemValues( const JsonObject& items,
                                                const std::string& operationId,
                                                const Json& source ) {
    bool customResponse = false;
    if( !items.is_object() ) {
        Log::error( "\"items\" parameter should be object" );
        return customResponse;
    }
    std::unordered_map< std::string, JsonObject > scriptRequests;
    for( auto it = items.begin(); it != items.end(); ++it ) {
        auto itemId = it.key();
        auto gateway = getDeviceGateway( itemId );
        auto script = getScript( gateway );
        if( script.empty() ) {
            continue;
        }
        auto scriptIt = scriptRequests.find( script );
        if( scriptIt == scriptRequests.end() ) {
            Json request = JSON_EMPTY_OBJ;
            if( !source.empty() )
                request[ "source" ] = source;
            customResponse |= isCustomResponse( request, operationId, gateway );
            auto& items = request[ "items" ];
            items = JSON_EMPTY_OBJ;
            items[ itemId ] = it.value();
            scriptRequests.emplace( std::move( script ), std::move( request ) );
        }
        else {
            auto& items = scriptIt->second[ "items" ];
            assert( items.is_object() );
            items[ itemId ] = it.value();
        }
    }

    /* scriptRequests = mapM scriptPath, { items: [ { params[i] } ] } */
    for( const auto& scriptRequest : scriptRequests ) {
        const std::string& script = scriptRequest.first;
        if( script == GatewayBuiltInScripts::SetItemValueScript ) {
            auto itemsIt = scriptRequest.second.find( "items" );
            if( itemsIt != scriptRequest.second.end() ) {
                for( auto itemIt = itemsIt->begin(); itemIt != itemsIt->end(); itemIt++ ) {
                    auto itemId = itemIt.key();
                    const Json& json = itemIt.value();

                    JsonObject updateParams = makeValueForUpdate( itemId, json );
                    handleUpdateItemValue( updateParams );
                }
            }
        }
        else {
            sendExecutePluginScript( script, scriptRequest.second );
        }
    }
    return customResponse;
}

/** Makes json formatted for updateItemValue out of json passed by hub.item.value.set command. */
Json CoreService::makeValueForUpdate( const ItemID& itemId, const JsonObject& json ) {
    LOGGER_WARN( cc::j( json ) );

    auto item = m_deviceManager->fetchItem( itemId );
    const auto typeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
    const auto valueType = Model::ValueType::fromString( typeStr );
    Json itemValue = JSON_EMPTY_OBJ;
    json_get( json, RPCParams::Value, itemValue );

    auto valueLoader = [&itemValue]( const Model::ValueType& valueType ) {
        return HubPlatform::CoreD::Model::ValueFactory::createItemValue( valueType, itemValue );
    };

    auto newValue = valueLoader( valueType );

    JsonObject updateParams = JSON( RPCParams::ItemId << itemId << RPCParams::ItemType << typeStr
                                                      << RPCParams::ItemValue << newValue->toJson() );

    auto valueMinIt = json.find( RPCParams::ItemValueMin );
    if( valueMinIt != json.end() ) {
        updateParams[ ItemFields::ValueMinFieldName ] = valueMinIt.value();
    }

    auto valueMaxIt = json.find( RPCParams::ItemValueMax );
    if( valueMaxIt != json.end() ) {
        updateParams[ ItemFields::ValueMaxFieldName ] = valueMaxIt.value();
    }

    return updateParams;
}

bool CoreService::handleSetSingleItemValue( const JsonObject& json,
                                            const std::string& operationId,
                                            const Json& source ) {
    const ItemID itemId = json_get_str( json, "_id" );
    auto gateway = getDeviceGateway( itemId );
    auto script = getScript( gateway );
    if( !script.empty() ) {
        if( script == GatewayBuiltInScripts::SetItemValueScript ) {
            JsonObject updateParams = makeValueForUpdate( itemId, json );
            bool customResponse = isCustomResponse( updateParams, operationId, gateway );
            handleUpdateItemValue( updateParams );
            return customResponse;
        }
        JsonObject request = JSON( "item_id" << itemId << "value" << json[ "value" ] );
        if( !source.empty() )
            request[ "source" ] = source;
        bool customResponse = isCustomResponse( request, operationId, gateway );
        sendExecutePluginScript( script, request );
        return customResponse;
    }
    return false;
}

void CoreService::handleUpdateItemDictionaryValue( const JsonObject& json ) {
    LOGGER_DEBUG( cc::debug( "Updating item dictionary value: " ) + cc::j( json ) );
    const ItemID itemId = json_get_str( json, RPCParams::ItemId );
    const auto& itemDictionaryId = json_get_str( json, RPCParams::ItemDictionaryId );
    const auto& newValueJson = json[ RPCParams::ItemDictionaryValue ];

    if( itemId.empty() ) {
        Log::warn( cc::warn( "CoreService::handleUpdateItemDictionaryValue, item id is bad" ) );
        return;
    }

    static std::mutex dictionaryValueModificationMutex;
    std::lock_guard< std::mutex > lock( dictionaryValueModificationMutex );

    JsonObject item = getItem( itemId );
    if( item.empty() )
        throw ItemNotFoundError{ itemId };

    const auto& itemValueJson = item[ Model::Resources::ItemFields::ValueFieldName ];
    const auto typeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
    auto itemValue = Model::DictionaryValue::createValue( Model::ValueType::fromString( typeStr ), itemValueJson );

    auto newValue = newValueJson.empty() ?
                        CoreDRemoteInterface::ValueUPtr() :
                        Model::ValueFactory::createItemValue( itemValue->getValuesType(), newValueJson );
    const auto oldValue = itemValue->getValue( itemDictionaryId );
    Json oldValueJson = ( oldValue ) ? oldValue->toJson() : JSON_EMPTY_OBJ;

    itemValue->setValue( itemDictionaryId, std::move( newValue ) );

    JsonObject encryptedValue;
    encryptedValue[ ItemFields::TypeFieldName ] = typeStr;
    encryptedValue[ ItemFields::ValueFieldName ] = itemValue->toJson();
    Model::Item::encryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), encryptedValue );
    json_remove( encryptedValue, ItemFields::TypeFieldName );

    auto dictionary = getDictionaryOperation( itemDictionaryId, oldValueJson, newValueJson );
    m_itemsDb->updateDictionary( itemId, encryptedValue, dictionary );
    m_houseModeAlarms->checkSecurityThreat( itemId );
}

void CoreService::handleUpdateGatewaySettingValue( const JsonObject& json ) {
    const GatewaySettingID settingId = json_get_str( json, RPCParams::SettingId );
    const auto valueIt = json.find( RPCParams::Value );
    const auto valueType = json_get_str( json, RPCParams::ValueType );

    if( settingId.empty() ) {
        LOGGER_ERROR( cc::error( "failed to update gateway setting value, gateway setting id is invalid" ) );
        return;
    }

    if( valueIt == json.end() ) {
        LOGGER_ERROR( cc::error( "failed to update a gateway setting value, " ) + cc::note( RPCParams::Value ) +
                      cc::error( " is missing in a request" ) );
        return;
    }

    auto result =
        JSON( GatewaySettingFields::Value << valueIt.value() << GatewaySettingFields::ValueType << valueType );
    m_gatewaySettingsDb->update( settingId, result );

    LOGGER_TRACE( cc::trace( "Gateway setting value has been updated" ) );
}

void CoreService::handleUpdateGatewaySettingStatus( const JsonObject& json ) {
    const GatewaySettingID settingId = json_get_str( json, RPCParams::SettingId );
    const auto status = json_get_str( json, RPCParams::SynchronizationStatus );

    if( settingId.empty() ) {
        LOGGER_ERROR( cc::error( "failed to update gateway setting status, gateway setting id is invalid" ) );
        return;
    }

    auto result = JSON( GatewaySettingFields::SynchronizationStatus << status );
    m_gatewaySettingsDb->update( settingId, result );

    LOGGER_TRACE( cc::trace( "Gateway setting status has been updated" ) );
}

void CoreService::handleStartGatewayProcess( const JsonObject& json ) {
    auto gatewayId = json_get_str( json, RPCParams::GatewayId );

    auto processString = json_get_str( json, RPCParams::Process );
    auto process = Model::gatewayProcessFromString( processString );

    auto previousProcess = Model::getGatewayCurrentProcess( getDBClient(), gatewayId );
    if( previousProcess != process && previousProcess != Model::GatewayProcess::Unknown ) {
        stopGatewayProcess( gatewayId, previousProcess, Model::GatewayProcessResult::Fail );
    }

    auto prevRunning = Model::isProcessRunning( getDBClient(), process );

    getDBClient().update( TempCoreGatewayProcessesDBName,
                          JSON_EMPTY_OBJ,
                          JSON( db::ID_FIELD_NAME << gatewayId << "process" << Model::toString( process ) ),
                          true );

    auto newRunning = Model::isProcessRunning( getDBClient(), process );

    // TODO: implement more complex resolve logic
    if( !prevRunning && newRunning ) {
        sendBroadcast( GATEWAY_PROCESS_STARTED, JSON( "process" << Model::toString( process ) ) );
        CommonUtils::LockHelper::global().onTransactionRunning( static_cast< int >( process ) );
    }
}

void CoreService::handleStopGatewayProcess( const JsonObject& json ) {
    auto gatewayId = json_get_str( json, RPCParams::GatewayId );

    auto processString = json_get_str( json, RPCParams::Process );
    auto process = Model::gatewayProcessFromString( processString );

    auto resultString = json_get_str( json, RPCParams::ProcessResult );
    auto result = Model::gatewayProcessResultFromString( resultString );

    stopGatewayProcess( gatewayId, process, result );
}

void CoreService::stopGatewayProcess( const GatewayID& gatewayId,
                                      Model::GatewayProcess process,
                                      Model::GatewayProcessResult result ) {
    auto previousProcess = Model::getGatewayCurrentProcess( getDBClient(), gatewayId );
    if( previousProcess != process ) {
        // Ignore request to stop process that is not started
        return;
    }

    auto prevRunning = Model::isProcessRunning( getDBClient(), process );

    getDBClient().remove( TempCoreGatewayProcessesDBName, JSON( db::ID_FIELD_NAME << gatewayId ) );

    auto newRunning = Model::isProcessRunning( getDBClient(), process );

    // TODO: implement more complex resolve logic
    if( prevRunning && !newRunning ) {
        sendBroadcast(
            GATEWAY_PROCESS_STOPPED,
            JSON( "process" << Model::toString( previousProcess ) << "result" << Model::toString( result ) ) );
        CommonUtils::LockHelper::global().onTransactionStopped( static_cast< int >( previousProcess ) );
    }
}

void CoreService::handleUpdateSettingValue( const JsonObject& json ) {
    LOGGER_DEBUG( cc::debug( "Updating setting value: " ) + cc::j( json ) );

    const DeviceSettingID settingId = json_get_str( json, RPCParams::SettingId );
    const auto valueIt = json.find( RPCParams::Value );
    const auto valueType = json_get_str( json, RPCParams::ValueType );
    const auto status = json_get_str( json, RPCParams::SynchronizationStatus );

    if( settingId.empty() ) {
        Log::warn( "CoreService::handleUpdateSettingValue, setting id is bad" );
        return;
    }

    if( valueIt == json.end() ) {
        Log::warn( std::string( "CoreService::handleUpdateSettingValue, " ) + RPCParams::Value + " missing" );
        return;
    }

    auto result = JSON( DeviceSettingFields::Value << valueIt.value() << DeviceSettingFields::ValueType << valueType
                                                   << DeviceSettingFields::SynchronizationStatus << status );

    m_deviceSettingsDb->update( settingId, result );
}

void CoreService::handleUpdateSettingDictionaryValue( const JsonObject& json ) {
    LOGGER_DEBUG( cc::debug( "Updating setting dictionary value: " ) + cc::j( json ) );

    static std::mutex dictionaryValueModificationMutex;

    const ItemID settingId = json_get_str( json, RPCParams::SettingId );

    if( settingId.empty() ) {
        Log::warn( "CoreService::handleUpdateSettingDictionaryValue, setting id is bad" );
        return;
    }

    std::lock_guard< std::mutex > lock( dictionaryValueModificationMutex );

    JsonObject setting = getSetting( settingId );
    if( setting.empty() )
        throw DeviceSettingNotFoundError{ settingId };

    const auto& settingValuesJson = setting[ Model::Resources::DeviceSettingFields::Value ];
    const auto type = json_get_str( setting, Model::Resources::DeviceSettingFields::ValueType );
    auto settingValues = Model::DictionaryValue::createValue( Model::ValueType::fromString( type ), settingValuesJson );

    const auto key = json_get_str( json, RPCParams::DictionaryKey );

    const auto& newValueJson = json[ RPCParams::DictionaryValue ];
    auto newValue = newValueJson.empty() ?
                        CoreDRemoteInterface::ValueUPtr() :
                        Model::ValueFactory::createItemValue( settingValues->getValuesType(), newValueJson );

    const auto oldValue = settingValues->getValue( key );
    Json oldValueJson = ( oldValue ) ? oldValue->toJson() : JSON_EMPTY_OBJ;

    settingValues->setValue( key, std::move( newValue ) );

    auto data = JSON( DeviceSettingFields::Value << settingValues->toJson() );

    auto operation = getDictionaryOperation( key, oldValueJson, newValueJson );
    m_deviceSettingsDb->updateDictionary( settingId, data, operation );
}

void CoreService::handleUpdateSettingStatus( const JsonObject& json ) {
    LOGGER_DEBUG( cc::debug( "Updating setting synchronization status: " ) + cc::j( json ) );

    const DeviceSettingID settingId = json_get_str( json, RPCParams::SettingId );
    const auto status = json_get_str( json, RPCParams::SynchronizationStatus );

    if( settingId.empty() ) {
        Log::warn( "CoreService::handleUpdateSettingValue, setting id is bad" );
        return;
    }

    auto result = JSON( DeviceSettingFields::SynchronizationStatus << status );
    m_deviceSettingsDb->update( settingId, result );
}

bool CoreService::handleAddItemDictionaryValue( const JsonObject& json, const std::string& operationId ) {
    bool customResponse = false;
    const auto& db = getDBClient();

    const ItemID itemId = json_get_str( json, "_id" );
    const JsonObject item = db.findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item.empty() )
        throw ItemNotFoundError{ itemId };

    const DeviceID deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() )
        throw DeviceNotFoundError{ deviceId };

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( gateway, GatewayFields::AddItemDictionaryValueCommandFieldName );
    if( script.empty() )
        return customResponse;

    jsonCheckObject( json, DictionaryEditRequestFields::ElementFieldName );

    const auto& jsonElement = json[ DictionaryEditRequestFields::ElementFieldName ];
    jsonCheckString( jsonElement, ItemFields::TypeFieldName, JsonStringChecks::NON_EMPTY );

    auto scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "item_id" ] = itemId;
    scriptParams[ "value_type" ] = jsonElement[ DictionaryEditRequestFields::ValueTypeFieldName ];
    scriptParams[ "value" ] = jsonElement[ DictionaryEditRequestFields::ValueFieldName ];
    if( !operationId.empty() && json_get_str( gateway, GatewayFields::SetItemValueResponsePolicyFieldName ) ==
                                    SetItemValueResponsePolicy::Custom ) {
        scriptParams[ "operation_id" ] = operationId;
        customResponse = true;
    }
    Json source = JSON_EMPTY_OBJ;
    if( json_get( json, RPCParams::SourceField, source ) && !source.empty() )
        scriptParams[ "source" ] = source;

    sendExecutePluginScript( script, scriptParams );
    return customResponse;
}

bool CoreService::handleSetItemDictionaryValue( const JsonObject& json, const std::string& operationId ) {
    bool customResponse = false;
    const auto& db = getDBClient();

    const ItemID itemId = json_get_str( json, "_id" );
    const JsonObject item = db.findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item.empty() )
        throw ItemNotFoundError{ itemId };

    const DeviceID deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() )
        throw DeviceNotFoundError{ deviceId };

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( gateway, GatewayFields::SetItemDictionaryValueCommandFieldName );
    if( script.empty() )
        return customResponse;

    jsonCheckString( json, DictionaryEditRequestFields::KeyFieldName, JsonStringChecks::NON_EMPTY );
    jsonCheckObject( json, DictionaryEditRequestFields::ElementFieldName );

    const auto& jsonElement = json[ DictionaryEditRequestFields::ElementFieldName ];
    jsonCheckString( jsonElement, DictionaryEditRequestFields::ValueTypeFieldName, JsonStringChecks::NON_EMPTY );

    auto scriptParams = JSON_EMPTY_OBJ;
    scriptParams[ "item_id" ] = itemId;
    scriptParams[ "key" ] = json[ DictionaryEditRequestFields::KeyFieldName ];
    scriptParams[ "value_type" ] = jsonElement[ DictionaryEditRequestFields::ValueTypeFieldName ];
    scriptParams[ "value" ] = jsonElement[ DictionaryEditRequestFields::ValueFieldName ];
    if( !operationId.empty() && json_get_str( gateway, GatewayFields::SetItemValueResponsePolicyFieldName ) ==
                                    SetItemValueResponsePolicy::Custom ) {
        scriptParams[ "operation_id" ] = operationId;
        customResponse = true;
    }
    Json source = JSON_EMPTY_OBJ;
    if( json_get( json, RPCParams::SourceField, source ) && !source.empty() )
        scriptParams[ "source" ] = source;

    sendExecutePluginScript( script, scriptParams );
    return customResponse;
}

bool CoreService::handleRemoveItemDictionaryValue( const JsonObject& json, const std::string& operationId ) {
    bool customResponse = false;
    const auto& db = getDBClient();

    const ItemID itemId = json_get_str( json, "_id" );
    const JsonObject item = db.findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item.empty() )
        throw ItemNotFoundError{ itemId };

    const DeviceID deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() )
        throw DeviceNotFoundError{ deviceId };

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( gateway, GatewayFields::RemoveItemDictionaryValueCommandFieldName );
    if( script.empty() )
        return customResponse;

    jsonCheckString( json, DictionaryEditRequestFields::KeyFieldName, JsonStringChecks::NON_EMPTY );

    auto scriptParams = JSON( "item_id" << itemId << "key" << json[ DictionaryEditRequestFields::KeyFieldName ] );
    if( !operationId.empty() && json_get_str( gateway, GatewayFields::SetItemValueResponsePolicyFieldName ) ==
                                    SetItemValueResponsePolicy::Custom ) {
        scriptParams[ "operation_id" ] = operationId;
        customResponse = true;
    }
    Json source = JSON_EMPTY_OBJ;
    if( json_get( json, RPCParams::SourceField, source ) && !source.empty() )
        scriptParams[ "source" ] = source;

    sendExecutePluginScript( script, scriptParams );
    return customResponse;
}

bool CoreService::handleClearItemDictionary( const JsonObject& json, const std::string& operationId ) {
    bool customResponse = false;
    const auto& db = getDBClient();

    const ItemID itemId = json_get_str( json, "_id" );
    const JsonObject item = db.findOne( DBNames::ItemsDB, JSON( ItemFields::IdFieldName << itemId ), nullptr );
    if( item.empty() )
        throw ItemNotFoundError{ itemId };

    const DeviceID deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() )
        throw DeviceNotFoundError{ deviceId };

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( gateway, GatewayFields::ClearItemDictionaryCommandFieldName );
    if( script.empty() )
        return customResponse;

    auto scriptParams = JSON( "item_id" << itemId );
    if( !operationId.empty() && json_get_str( gateway, GatewayFields::SetItemValueResponsePolicyFieldName ) ==
                                    SetItemValueResponsePolicy::Custom ) {
        scriptParams[ "operation_id" ] = operationId;
        customResponse = true;
    }
    Json source = JSON_EMPTY_OBJ;
    if( json_get( json, RPCParams::SourceField, source ) && !source.empty() )
        scriptParams[ "source" ] = source;

    sendExecutePluginScript( script, scriptParams );
    return customResponse;
}

void CoreService::handleSetGatewaySettingValue( const JsonObject& json ) {
    const auto& db = getDBClient();

    const auto settingId = json_get_str( json, GatewaySettingFields::Id );
    const auto joSetting =
        db.findOne( DBNames::GatewaysSettingsDB, JSON( GatewaySettingFields::Id << settingId ), nullptr );
    if( joSetting.empty() ) {
        LOGGER_ERROR(
            cc::error( "failed to perform a gateway.setting.set request, gateway setting not found by id: " ) +
            cc::note( settingId ) );
        return;
    }

    const GatewayID gatewayID = json_get_str( joSetting, GatewaySettingFields::GatewayId );
    const auto joGateway = db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );
    if( joGateway.empty() ) {
        LOGGER_ERROR( cc::error( "failed to perform a gateway.setting.set request, gateway not found by id: " ) +
                      cc::note( gatewayID ) );
        return;
    }

    auto script = json_get_str( joGateway, GatewayFields::SetGatewaySettingValueCommandFieldName );
    if( script.empty() ) {
        LOGGER_WARN( cc::warn( "failed to perform a gateway.setting.set request, script " ) +
                     cc::note( GatewayFields::SetGatewaySettingValueCommandFieldName ) +
                     cc::warn( " not found by gateway id: " ) + cc::note( gatewayID ) );
        return;
    }

    auto scriptParams = JSON( "setting_id" << settingId << "value" << json[ "value" ] );
    sendExecutePluginScript( script, scriptParams );

    LOGGER_TRACE( cc::trace( "redirect a gateway.setting.set request to a script " ) +
                  cc::note( GatewayFields::SetGatewaySettingValueCommandFieldName ) );
}

JsonObject CoreService::handleSetDeviceSettingValue( const JsonObject& json ) {
    const auto& db = getDBClient();

    const DeviceSettingID settingId = json_get_str( json, DeviceSettingFields::Id );
    const auto joSetting =
        db.findOne( DBNames::DevicesSettingsDB, JSON( DeviceSettingFields::Id << settingId ), nullptr );
    if( joSetting.empty() ) {
        LOGGER_ERROR( cc::error( "Device setting not found by id: " ) + settingId );
        return JSON_EMPTY_OBJ;
    }

    const DeviceID deviceId = json_get_str( joSetting, DeviceSettingFields::DeviceId );
    const auto joDevice = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( joDevice.empty() ) {
        LOGGER_ERROR( cc::error( "Device not found by id: " ) + deviceId );
        return JSON_EMPTY_OBJ;
    }

    const GatewayID gatewayID = json_get_str( joDevice, DeviceFields::GatewayIdFieldName );
    const auto joGateway = db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( joGateway, GatewayFields::SetSettingValueCommandFieldName );

    if( script.empty() ) {
        LOGGER_DEBUG( cc::debug( "Script '" ) + cc::p( GatewayFields::SetSettingValueCommandFieldName ) +
                      cc::debug( "' not found by gateway id: " ) + gatewayID );
        return JSON_EMPTY_OBJ;
    }

    auto scriptParams = JSON( "setting_id" << settingId << "value" << json[ "value" ] );
    sendExecutePluginScript( script, scriptParams );
    return JSON_EMPTY_OBJ;
}

JsonObject CoreService::handleSetDeviceSettingDictionaryValue( const JsonObject& json ) {
    const auto& db = getDBClient();

    const DeviceSettingID settingId = json_get_str( json, DeviceSettingFields::Id );
    const auto joSetting =
        db.findOne( DBNames::DevicesSettingsDB, JSON( DeviceSettingFields::Id << settingId ), nullptr );
    if( joSetting.empty() ) {
        LOGGER_ERROR( cc::error( "Device setting not found by id: " ) + settingId );
        return JSON_EMPTY_OBJ;
    }

    const DeviceID deviceId = json_get_str( joSetting, DeviceSettingFields::DeviceId );
    const auto joDevice = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( joDevice.empty() ) {
        LOGGER_ERROR( cc::error( "Device not found by id: " ) + deviceId );
        return JSON_EMPTY_OBJ;
    }

    const GatewayID gatewayID = json_get_str( joDevice, DeviceFields::GatewayIdFieldName );
    const auto joGateway = db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( joGateway, GatewayFields::SetSettingDictionaryValueCommandFieldName );

    if( script.empty() ) {
        LOGGER_DEBUG( cc::debug( "Script '" ) + cc::p( GatewayFields::SetSettingDictionaryValueCommandFieldName ) +
                      cc::debug( "' not found by gateway id: " ) + gatewayID );
        return JSON_EMPTY_OBJ;
    }

    auto scriptParams = Json::object();
    scriptParams[ "setting_id" ] = settingId;
    scriptParams[ "key" ] = json[ "key" ];

    auto itValue = json.find( "value" );
    if( itValue != json.end() ) {
        scriptParams[ "value" ] = *itValue;
    }

    sendExecutePluginScript( script, scriptParams );
    return JSON_EMPTY_OBJ;
}

JsonObject CoreService::handleResetDeviceSetting( const JsonObject& json ) {
    const auto& db = getDBClient();

    JsonObject scriptParams;

    DeviceID deviceId = json_get_str( json, DeviceSettingFields::DeviceId );
    if( deviceId.empty() ) {
        const DeviceSettingID settingId = json_get_str( json, DeviceSettingFields::Id );
        if( settingId.empty() ) {
            LOGGER_ERROR( cc::error( "No setting or device ID specified" ) );
            return JSON_EMPTY_OBJ;
        }

        const auto joSetting =
            db.findOne( DBNames::DevicesSettingsDB, JSON( DeviceSettingFields::Id << settingId ), nullptr );
        if( joSetting.empty() ) {
            LOGGER_ERROR( cc::error( "Device setting not found by id: " ) + settingId );
            return JSON_EMPTY_OBJ;
        }

        deviceId = json_get_str( joSetting, DeviceSettingFields::DeviceId );

        scriptParams[ "setting_id" ] = settingId;
    }
    else {
        scriptParams[ "device_id" ] = deviceId;
    }

    const auto joDevice = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( joDevice.empty() ) {
        LOGGER_ERROR( cc::error( "Device not found by id: " ) + deviceId );
        return JSON_EMPTY_OBJ;
    }

    const GatewayID gatewayID = json_get_str( joDevice, DeviceFields::GatewayIdFieldName );
    const auto joGateway = db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( joGateway, GatewayFields::ResetSettingCommandFieldName );
    if( script.empty() ) {
        LOGGER_DEBUG( cc::debug( "Script '" ) + cc::p( GatewayFields::ResetSettingCommandFieldName ) +
                      cc::debug( "' not found by gateway id: " ) + gatewayID );
        return JSON_EMPTY_OBJ;
    }

    sendExecutePluginScript( script, scriptParams );
    return JSON_EMPTY_OBJ;
}

//  json = {
//      "device_id": "deviceId",
//      "check": "value|status", (opt)
//      "items": ["itemId1", "itemId2"] (opt)
//  }
JsonObject CoreService::handleCheckDevice( const JsonObject& json ) {
    auto result = JSON_EMPTY_OBJ;

    const auto& db = getDBClient();
    const DeviceID deviceId = json_get_str( json, "device_id" );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() ) {
        setError( result, Errors::NO_DEVICE_FOUND );
        return result;
    }

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );

    auto script = json_get_str( gateway, GatewayFields::CheckDeviceCommandFieldName );
    if( script.empty() ) {
        setError( result, Errors::DEVICE_CHECK_NOT_SUPPORTED );
        return result;
    }

    sendExecutePluginScript( script, json );
    return result;
}

void CoreService::handleRemoveDeviceForce( const JsonObject& json ) {
    const auto& db = getDBClient();
    const DeviceID deviceId = json_get_str( json, RPCParams::DeviceId );
    const JsonObject device = db.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    if( device.empty() )
        throw DeviceNotFoundError{ deviceId };
    if( json_get_bool( device, DeviceFields::PersistentFieldName, false ) == true ) {
        LOGGER_DEBUG( cc::debug( "Trying to force-remove a persistent device, id: " ) + deviceId );
        throw PersistentDeviceForceRemoveDeniedError{ deviceId };
    }

    if( json_get_bool( device, DeviceFields::ReachableFieldName ) ) {
        Log::info( "CoreService::handleRemoveDeviceForce call force_remove command for reachable device." );
    }

    const GatewayID gatewayID = json_get_str( device, DeviceFields::GatewayIdFieldName );
    const JsonObject gateway =
        db.findOne( DBNames::GatewaysDB, JSON( GatewayFields::IdFieldName << gatewayID ), nullptr );
    auto script = json_get_str( gateway, GatewayFields::ForceRemoveDeviceCommandFieldName );
    if( script.empty() ) {
        Log::warn( "CoreService::handleRemoveDeviceForce ExecutePluginScript is empty." );
        return;
    }

    sendExecutePluginScript( script, JSON( RPCParams::DeviceId << deviceId ) );
}

void CoreService::handleUpdateReachableState( const JsonObject& json ) {
    const auto deviceId = getParameter< std::string >( json, RPCParams::DeviceId );
    const auto reachable = getParameter< bool >( json, RPCParams::ReachableState );

    m_devicesDb->update( deviceId, JSON( DeviceFields::ReachableFieldName << reachable ) );
}

void CoreService::handleUpdateDeviceStatus( const JsonObject& json ) {
    LOGGER_DEBUG( cc::j( json ) );

    const auto deviceId = getParameter< std::string >( json, RPCParams::DeviceId );
    const auto status = getParameter< std::string >( json, RPCParams::DeviceStatus );

    m_devicesDb->update( deviceId, JSON( DeviceFields::StatusFieldName << status ) );
}

void CoreService::handleUpdateReadyState( const JsonObject& json ) {
    const auto deviceId = getParameter< std::string >( json, RPCParams::DeviceId );
    const auto ready = getParameter< bool >( json, RPCParams::ReadyState );

    m_devicesDb->update( deviceId, JSON( DeviceFields::ReadyFieldName << ready ) );
}

void CoreService::handleUpdateDeviceName( const JsonObject& json ) {
    const auto deviceId = getParameter< std::string >( json, RPCParams::DeviceId );
    const auto deviceName = getParameter< std::string >( json, RPCParams::DeviceName );

    m_devicesDb->update( deviceId, JSON( DeviceFields::NameFieldName << deviceName ) );
}

void CoreService::handleUpdateDeviceRoomId( const JsonObject& json ) {
    const auto deviceId = getParameter< std::string >( json, RPCParams::DeviceId );
    const auto deviceRoomId = getParameter< std::string >( json, RPCParams::DeviceRoomId );
    bool parentRoom = json_get_bool( json, RPCParams::ParentRoom, CoreD::Model::Resources::Rooms::ParentRoomDefault );

    std::set< std::string > separateChildDevices;
    JsonArray strings;
    if( json_get( json, RPCParams::SeparateChildDevices, strings ) ) {
        try {
            fromJsonStringArray( strings, [&separateChildDevices]( const std::string& value ) {
                separateChildDevices.emplace( value );
            } );
        }
        catch( ... ) {
            JsonObject result;
            Log::error( "separateChildDevices field has incorrect type" );
        }
    }

    m_deviceManager->placeDevice( deviceId, deviceRoomId, separateChildDevices );
}

JsonObject CoreService::handleSetHubCoordinates( const JsonObject& json ) {
    JsonObject::const_iterator latIt = json.find( CoreDRemoteInterface::SettingsFields::LAT_NAME );
    JsonObject::const_iterator lngIt = json.find( CoreDRemoteInterface::SettingsFields::LNG_NAME );
    if( latIt != json.cend() && lngIt != json.cend() ) {
        try {
            float lat = latIt->get< float >();
            float lng = lngIt->get< float >();
            getDBClient().update( HUBSettings,
                                  JSON_EMPTY_OBJ,
                                  JSON( db::ID_FIELD_NAME << CoreDRemoteInterface::SettingsFields::LAT_NAME
                                                          << CoreDRemoteInterface::SettingsFields::LAT_NAME << lat ),
                                  true,
                                  false );
            getDBClient().update( HUBSettings,
                                  JSON_EMPTY_OBJ,
                                  JSON( db::ID_FIELD_NAME << CoreDRemoteInterface::SettingsFields::LNG_NAME
                                                          << CoreDRemoteInterface::SettingsFields::LNG_NAME << lng ),
                                  true,
                                  false );
            return JSON_EMPTY_OBJ;
        }
        catch( const std::exception& ex ) {
            Log::error( "CoreService::handleSetHubCoordinates has failed, error : " + std::string( ex.what() ) );
            JsonObject error;
            setError( error, Errors::INTERNAL_OPERATION_ERROR );
            return error;
        }
    }
    Log::error( "CoreService::handleSetHubCoordinates has wrong arguments" );
    JsonObject invalidParams;
    setError( invalidParams, Errors::WRONG_PARAMS );
    return invalidParams;
}

//  "json": {
//      "feature": "protect",
//      "status": false
//  }
void CoreService::handleSetFeatureStatus( const JsonObject& json ) {
    if( !m_featuresManager ) {
        return;
    }
    auto feature = json_get_str( json, "feature" );
    bool status = json_get_bool( json, "status" );
    m_featuresManager->setFeatureStatus( feature, status );
}

//  "json": {
//      "name": "scale.temperature",
//      "value": {...}
//  }
void CoreService::handleSetSettingValue( const JsonObject& json ) {
    LOGGER_INFO( cc::j( json ) );
    if( !m_settingsManager ) {
        return;
    }
    auto name = json_get_str( json, "name" );
    Json value;
    json_get( json, "value", value );
    if( m_settingsManager->setSettingValue( name, value ) ) {
        LOGGER_INFO( cc::trace( "sending broadcast: HUB_SETTING_UPDATED: " ) + cc::j( json ) );
        sendBroadcast( MessageSubClasses::HUB_SETTING_UPDATED, json, GroupsNames::BROADCAST_GROUP );
        handleSettingUpdated( json );
    }
}

void CoreService::handleSettingUpdated( const JsonObject& json ) {
    auto name = json_get_str( json, "name" );
    if( name == Resources::Settings::LogsRotationInterval ) {
        addTask( []() {
            LOGGER_DEBUG_RAW( cc::debug( "Apply logs rotation interval" ) );

            auto promise = std::make_shared< std::promise< void > >();
            auto future = promise->get_future();

            auto threadTask = [promise]() {
                std::string command = Environment::getBinariesFolder() + "/logcmd --update-settings";
                auto result = HubPlatform::System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
                if( result.second != 0 )
                    LOGGER_ERROR_RAW( cc::error( "Failed to apply logs rotation interval: " ) +
                                      cc::note( result.first ) );
                promise->set_value();
            };

            std::thread thread( std::move( threadTask ) );
            const std::chrono::seconds commandTimeout( 10 );
            auto status = future.wait_for( commandTimeout );
            if( status == std::future_status::ready ) {
                if( thread.joinable() )
                    thread.join();
            }
            else {
                LOGGER_ERROR_RAW( cc::error( "Failed to apply logs rotation interval: " ) +
                                  cc::note( "logcmd timeout" ) );
                thread.detach();
            }
        } );
    }
}

void CoreService::handleRemoveGatewaySetting( const JsonObject& json ) {
    const auto settingId = json_get_str( json, Model::Resources::RPCParams::SettingId );
    removeGatewaySetting( settingId );
}

void CoreService::removePluginGateways( const PluginID& pluginId ) {
    const auto query = JSON( GatewayFields::PluginIdFieldName << pluginId );
    const auto fieldsToReturn = JSON( GatewayFields::IdFieldName << "id" );

    const auto gateways = getDBClient().findAll( DBNames::GatewaysDB, query, &fieldsToReturn );
    for( const auto& gate : gateways ) {
        removeGateway( json_get_str( gate, "id" ) );
    }
}

void CoreService::removeGateway( const GatewayID& gatewayId ) {
    removeGatewayDevices( gatewayId );

    removeGatewaySettings( gatewayId );

    stopGatewayProcesses( gatewayId );

    m_gatewaysDb->remove( gatewayId );
}

void CoreService::removeGatewaySettings( const GatewayID& gatewayId ) {
    const auto query = JSON( GatewaySettingFields::GatewayId << gatewayId );
    const auto fieldsToReturn = JSON( GatewaySettingFields::Id << "id" );

    const auto settings = getDBClient().findAll( DBNames::GatewaysSettingsDB, query, &fieldsToReturn );
    for( const auto& setting : settings ) {
        removeGatewaySetting( json_get_str( setting, "id" ) );
    }
}

void CoreService::stopGatewayProcesses( const GatewayID& gatewayId ) {
    auto process = Model::getGatewayCurrentProcess( getDBClient(), gatewayId );
    if( process != Model::GatewayProcess::Unknown ) {
        stopGatewayProcess( gatewayId, process, Model::GatewayProcessResult::Fail );
    }
}

void CoreService::removeGatewaySetting( const GatewaySettingID& settingId ) {
    m_gatewaySettingsDb->remove( settingId );
    LOGGER_TRACE( cc::trace( "gateway setting has been removed, id:" ) + cc::note( settingId ) );
}

void CoreService::removeGatewayDevices( const GatewayID& gatewayId ) {
    const auto query =
        JSON( DeviceFields::GatewayIdFieldName << gatewayId << DeviceFields::ParentDeviceIdFieldName << "" );
    const auto fieldsToReturn = JSON( DeviceFields::IdFieldName << "id" );

    const auto devices = getDBClient().findAll( DBNames::DevicesDB, query, &fieldsToReturn );
    for( const auto& device : devices ) {
        removeDevice( json_get_str( device, "id" ) );
    }
}

void CoreService::removeDevice( const DeviceID& deviceId ) {
    auto removeSingleDevice = [this]( const std::string& deviceId ) {
        removeCamerasOffDevice( deviceId );  // call before removeDeviceItems
        CoreDHouseModes::removeCamera( getDBClient(), deviceId );
        CoreDHouseModes::removeProtectDevice( getDBClient(), deviceId );
        removeBypassDevice( deviceId );
        removeDeviceItems( deviceId );
        removeDeviceSettings( deviceId );
        eraseDisarmedDevice( deviceId );
        removeAlarmsOffDevice( deviceId );  // need more info about logic
        removeProtectButton( deviceId );
        CoreDHouseModes::removeSecurityDevice( getDBClient(), deviceId );
        m_devicesDb->remove( deviceId );

        auto deviceGroupBroadcastSender = [this]( MessageSubClasses messageSubClass, const JsonObject& message ) {
            sendDeviceItemGroupBroadcast( messageSubClass, message );
        };
        DeviceGroupUpdater::removeDeviceFromGroupOperations( deviceId, getDBClient(), deviceGroupBroadcastSender );
    };

    for( const auto& childDeviceId : getChildDeviceIds( deviceId ) ) {
        removeSingleDevice( childDeviceId );
    }
    removeSingleDevice( deviceId );
}

void CoreService::removeDeviceItems( const DeviceID& deviceId ) {
    const auto query = JSON( ItemFields::DeviceIdFieldName << deviceId );
    const auto fieldsToReturn = JSON( ItemFields::IdFieldName << "id" );

    const auto items = getDBClient().findAll( DBNames::ItemsDB, query, &fieldsToReturn );
    for( const auto& item : items ) {
        removeItem( json_get_str( item, "id" ) );
    }
}

void CoreService::removeDeviceSettings( const DeviceID& deviceId ) {
    const auto query = JSON( DeviceSettingFields::DeviceId << deviceId );
    const auto fieldsToReturn = JSON( DeviceSettingFields::Id << "id" );

    const auto settings = getDBClient().findAll( DBNames::DevicesSettingsDB, query, &fieldsToReturn );
    for( const auto& setting : settings ) {
        removeDeviceSetting( json_get_str( setting, "id" ) );
    }
}

void CoreService::removeItem( const ItemID& itemId ) {
    m_itemsDb->remove( itemId );
}

void CoreService::removePolicy( const PolicyID& policyId ) {
    m_policiesDb->remove( policyId );
}

void CoreService::removeDeviceSetting( const DeviceSettingID& settingId ) {
    m_deviceSettingsDb->remove( settingId );
}

void CoreService::deleteRoom( const CoreService::RoomID& roomId ) {
    const JsonObject fieldsToReturn = JSON( "_id" << 1 << DeviceFields::ParentDeviceIdFieldName << 1 );
    const auto devices =
        getDBClient().findAll( DBNames::DevicesDB, JSON( DeviceFields::RoomIdFieldName << roomId ), &fieldsToReturn );

    m_roomManager->deleteRoom( roomId );

    for( auto& device : devices ) {
        const std::string parentDeviceId = json_get_str( device, DeviceFields::ParentDeviceIdFieldName );
        const std::string deviceId = json_get_str( device, DeviceFields::IdFieldName );
        m_deviceManager->unplaceDevice( deviceId );
    }
}

std::vector< std::string > CoreService::getChildDeviceIds( const DeviceID& parentDeviceId ) const {
    const auto query = JSON( DeviceFields::ParentDeviceIdFieldName << parentDeviceId );
    const auto fieldsToReturn = JSON( ItemFields::IdFieldName << "id" );

    const auto devices = getDBClient().findAll( DBNames::DevicesDB, query, &fieldsToReturn );

    std::vector< std::string > ids;
    for( const auto& device : devices ) {
        auto id = json_get_str( device, "id" );
        assert( !id.empty() );

        auto childDeviceIds = getChildDeviceIds( id );

        ids.push_back( std::move( id ) );
        ids.insert( ids.end(),
                    std::make_move_iterator( childDeviceIds.begin() ),
                    std::make_move_iterator( childDeviceIds.end() ) );
    }

    return ids;
}

JsonObject CoreService::getHubStatus( const JsonObject& request ) {
    LOGGER_INFO( "" );

    HubPlatform::SystemUtils::ProcFS::MemoryInfo memInfo;
    JsonObject result = JSON_EMPTY_OBJ;

    if( memInfo.parseMeminfo() != 0 ) {
        result[ "ram" ] = JSON( "availablePct" << memInfo.memAvailablePercent << "available"
                                               << JSON( "value" << memInfo.memAvailableKib << "scale"
                                                                << "Kb" )
                                               << "total"
                                               << JSON( "value" << memInfo.memTotalKib << "scale"
                                                                << "Kb" ) );
    }
    else {
        setError( result, Errors::GET_RAM_INFO_ERROR );
        return result;
    }

    if( memInfo.hasFlashInfo() != false ) {
        JsonArrayBuilder fsInfos;
        for( auto itMap = memInfo.flashDirectories.begin(); itMap != memInfo.flashDirectories.end(); ++itMap ) {
            fsInfos << JSON( "availablePct" << ( itMap->second )[ 4 ] << "units"
                                            << "Mb"
                                            << "available" << ( itMap->second )[ 1 ] << "filesystem" << itMap->first
                                            << "total" << ( itMap->second )[ 0 ] );
        }
        result[ "fileSystems" ] = fsInfos;
    }
    else {
        setError( result, Errors::GET_FS_INFO_ERROR );

        return result;
    }

    if( memInfo.hasProcessInfo() != false ) {
        JsonArrayBuilder processesInfo;
        for( auto itLP = memInfo.processList.begin(); itLP != memInfo.processList.end(); ++itLP ) {
            auto pid = itLP->first;
            int64_t pidNumber = 0;
            try {
                pidNumber = std::stoi( pid );
            }
            catch( const std::exception& ex ) {
                LOGGER_ERROR( cc::error( "Failed to parse int from pid string: " ) + cc::note( pid ) +
                              cc::error( ", name: " ) + itLP->second.name +
                              ", error: " + CommonUtils::whatError( ex ) );
                continue;
            }
            processesInfo << JSON( "processName" << itLP->second.name << "memoryUsage" << itLP->second.memoryTotal
                                                 << "rssAnon" << itLP->second.rssAnon << "vmRss" << itLP->second.vmRss
                                                 << "pid" << pidNumber << "units"
                                                 << "Kb" );
        }
        result[ "processes" ] = processesInfo;
    }
    else {
        setError( result, Errors::GET_PROCESSES_INFO_ERROR );
        return result;
    }

    WebRtcGroupInfo controlGroupsInfo;
    Json oom = controlGroupsInfo.getOomControl();
    Json limits = controlGroupsInfo.getMemoryLimitAndUsage();
    if( !oom.empty() && !limits.empty() ) {
        Json webrtc = JSON( "oom" << oom << "limits" << limits );
        JsonObject controlGroups;
        controlGroups[ "webrtc" ] = webrtc;
        result[ "controlGroups" ] = controlGroups;
    }
    else {
        LOGGER_TRACE( cc::trace( "CGroup 'webrtc' not found" ) );
    }

    return result;
}

GatewayDbModifier& CoreService::getGatewaysDbModifier() {
    return *m_gatewaysDb;
}

DeviceDbModifier& CoreService::getDevicesDbModifier() {
    return *m_devicesDb;
}

ItemDbModifier& CoreService::getItemsDbModifier() {
    return *m_itemsDb;
}

PolicyDbModifier& CoreService::getPoliciesDbModifier() {
    return *m_policiesDb;
}

GatewaySettingDbModifier& CoreService::getGatewaySettingsDbModifier() {
    return *m_gatewaySettingsDb;
}

DeviceSettingDbModifier& CoreService::getDeviceSettingsDbModifier() {
    return *m_deviceSettingsDb;
}

RoomDbModifier& CoreService::getRoomDbModifier() {
    return *m_roomsDb;
}

JsonObject CoreService::sendBackupRequest( const std::string& service,
                                           const JsonObject& request,
                                           MessageSubClasses processType ) {
    return sendRequest( service, request, processType );
}

void CoreService::sendExecutePluginScript( const std::string& script, const Json& scriptParams ) {
    LOGGER_DEBUG( cc::debug( "Execute script: " ) + cc::p( script ) + cc::debug( ", with params: " ) +
                  cc::j( scriptParams ) );
    auto data = prepareMethod( "executeScript" );
    data[ "params" ][ "script" ] = script;
    if( !scriptParams.empty() )
        data[ "params" ][ "scriptParams" ] = scriptParams;

    Services::Message message = { "addon.lua", JSON_RPC, CALL_METHOD, data };
    send( message );
}

DeviceManager& CoreService::getDeviceManager() {
    return *m_deviceManager;
}

JsonObject CoreService::handleBackupVeraRequest( const Json& request ) {
    auto directoryIt = request.find( "directory" );
    if( directoryIt == request.end() || !directoryIt->is_string() ) {
        return generateVeraBackupResponse( "failed", "Failed to read directory path" );
    }

    auto veraModel = readVeraNodesInformation( directoryIt->get< std::string >() + "/etc/cmh/user_data.json.lzo" );

    if( veraModel.empty() ) {
        return generateVeraBackupResponse( "finished", "" );
    }

    std::unordered_map< int, std::string > veraToEzloRoomMappings;
    auto roomsMigrationResult = m_roomManager->migrateVeraRooms( veraModel, veraToEzloRoomMappings );
    if( roomsMigrationResult.first != "finished" ) {
        return generateVeraBackupResponse( roomsMigrationResult.first, roomsMigrationResult.second );
    }

    const auto& devices = veraModel[ "devices" ];
    for( const auto& deviceInfo : devices ) {
        auto veraDeviceId = json_get_int( deviceInfo, "id" );
        auto deviceId = findEzloDeviceId( veraDeviceId );
        if( deviceId.empty() ) {
            continue;
        }

        auto veraRoomIdStr = json_get_str( deviceInfo, "room" );
        if( veraRoomIdStr.empty() || !std::all_of( veraRoomIdStr.begin(), veraRoomIdStr.end(), []( unsigned char c ) {
                return std::isdigit( c );
            } ) ) {
            continue;
        }
        auto veraRoomId = std::stoi( veraRoomIdStr );
        auto ezloRoomIdIt = veraToEzloRoomMappings.find( veraRoomId );
        if( ezloRoomIdIt != veraToEzloRoomMappings.end() ) {
            m_deviceManager->placeDevice( deviceId, ezloRoomIdIt->second, std::set< std::string >() );
        }
    }

    return generateVeraBackupResponse( "finished", "" );
}

Json CoreService::readVeraNodesInformation( const std::string& pathToFile ) const {
    std::string target;
    if( common::lzo_utils::decompressFile( pathToFile, target ) ) {
        try {
            auto json = Json::parse( target );
            return json;
        }
        catch( const std::exception& e ) {
            Log::error( cc::error( "Unable to read vera nodes information" ) );
            return JSON_EMPTY_OBJ;
        }
    }

    return JSON_EMPTY_OBJ;
}

std::string CoreService::findEzloDeviceId( int veraDeviceId ) const {
    auto deviceMapping =
        getDBClient().findOne( TempVeraDeviceMappingsDBName, JSON( db::ID_FIELD_NAME << veraDeviceId ), nullptr );
    auto deviceDataIt = deviceMapping.find( "deviceData" );
    if( deviceDataIt != deviceMapping.end() ) {
        return json_get_str( *deviceDataIt, "device_id" );
    }

    return {};
}

JsonObject CoreService::generateVeraBackupResponse( const std::string& status,
                                                    const std::string& errorDescription ) const {
    auto response = JSON( "status" << status );
    if( !errorDescription.empty() ) {
        response[ "error_description" ] = errorDescription;
    }

    return response;
}

bool CoreService::initLogOptionsFromFirmwareConfig( const std::string& firmwareCfgPath,
                                                    const ConfigReader& configReader,
                                                    const UpdateManager::RemoteInterface& umRemoteInterface ) {
    if( filesystem::isFileExist( firmwareCfgPath ) ) {
        int32_t modificationTimestamp = filesystem::fileModificationTime( firmwareCfgPath );
        LOGGER_DEBUG( cc::debug( "File exists, its modification time is: " ) + cc::num10( modificationTimestamp ) );

        Json record = getDBClient().findOne(
            SysFlagsDBName,
            JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::FirmwareCfgModificationTime ),
            nullptr );
        int prevTime = 0;
        if( record.empty() ) {
            auto cfgModificationTimeRecord =
                JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::FirmwareCfgModificationTime << "value"
                                        << modificationTimestamp );
            LOGGER_DEBUG( cc::debug( "Record is empty, insert new one: " ) + cc::j( cfgModificationTimeRecord ) );
            getDBClient().insert( SysFlagsDBName, cfgModificationTimeRecord );
        }
        else {
            prevTime = json_get_int( record, "value" );
        }

        if( prevTime && modificationTimestamp ) {
            LOGGER_DEBUG( cc::debug( "Modification times differ, previous time: " ) + cc::num( prevTime ) );
            if( modificationTimestamp > prevTime ) {
                auto updateState = umRemoteInterface.getCurrentUpdateState();
                auto status = !updateState.empty() ? json_get_str( updateState, "status" ) : "";
                if( status == Resources::PendingUpdateFields::Process ) {
                    getDBClient().update(
                        SysFlagsDBName,
                        JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::FirmwareCfgModificationTime ),
                        JSON( "$set" << modificationTimestamp ) );

                    return false;
                }

                std::unique_ptr< std::string > logLevelUPtr;
                std::unique_ptr< bool > logColorUPtr;
                std::unique_ptr< int > logIndentUPtr;
                try {
                    auto logLevel = configReader.getString( HubPlatform::ConfigKeys::LogLevel );
                    if( !logLevel.empty() ) {
                        logLevelUPtr = std::make_unique< std::string >( std::move( logLevel ) );
                    }
                }
                catch( const ConfigReader::ReadingError& ex ) {
                    LOGGER_ERROR( cc::error( "ReadingError exception: " ) + cc::error( ex.what() ) );
                }

                try {
                    auto logColor = configReader.getBool( HubPlatform::ConfigKeys::LogColor );
                    logColorUPtr = std::make_unique< bool >( logColor );
                }
                catch( const ConfigReader::ReadingError& ex ) {
                    LOGGER_ERROR( cc::error( "ReadingError exception: " ) + cc::error( ex.what() ) );
                }
                try {
                    int logIndent = configReader.getInt( HubPlatform::ConfigKeys::LogIndent );
                    logIndentUPtr = std::make_unique< int >( logIndent );
                }
                catch( const ConfigReader::ReadingError& ex ) {
                    LOGGER_ERROR( cc::error( "ReadingError exception: " ) + cc::error( ex.what() ) );
                }

                LOGGER_DEBUG( cc::debug( "Save settings" ) );
                using namespace Services::Resources;
                int changed = 0;
                Hub::SettingsManager& settingsManager = *m_settingsManager;
                const char* localLogLevel = Log::toString( static_cast< Log::PriorityLevel >( Log::getLogLevel() ) );

                if( logLevelUPtr && *logLevelUPtr != localLogLevel ) {
                    LOGGER_TRACE( cc::trace( "update setting: logs.color" ) );
                    handleSetSettingValue( { { "name", HubSettings::LogsLevel }, { "value", *logLevelUPtr } } );
                    changed++;
                }
                if( logColorUPtr && *logColorUPtr != Log::getLocalColors() ) {
                    LOGGER_TRACE( cc::trace( "update setting: logs.color" ) );
                    handleSetSettingValue( { { "name", HubSettings::LogsColor }, { "value", *logColorUPtr } } );
                    changed++;
                }
                if( logIndentUPtr && *logIndentUPtr != Log::getLocalIndent() ) {
                    LOGGER_TRACE( cc::trace( "update setting: logs.indent" ) );
                    handleSetSettingValue(
                        { { "name", HubSettings::LogsIndent }, { "value", std::to_string( *logIndentUPtr ) } } );
                    changed++;
                }

                getDBClient().update(
                    SysFlagsDBName,
                    JSON_EMPTY_OBJ,
                    JSON( db::ID_FIELD_NAME << Resources::PendingUpdateFields::FirmwareCfgModificationTime << "value"
                                            << modificationTimestamp ),
                    true,
                    false );

                if( changed ) {
                    std::string newLogLevelString =
                        logLevelUPtr ? logLevelUPtr->c_str() :
                                       Log::toString( static_cast< Log::PriorityLevel >( Log::getLogLevel() ) );

                    addTask( [this, newLogLevelString]() {
                        LOGGER_INFO_RAW( cc::trace( "firmware.cfg was changed, logLevel: " ) + newLogLevelString );
                        handleReloadGlobalLogOptions( JSON( "logLevel" << newLogLevelString ) );
                    } );
                    return true;
                }
            }
            else {
                LOGGER_TRACE(
                    cc::trace( "Seems modification time of firmware.cfg is not less then current, Skipped." ) );
            }
        }
    }
    return false;
}

void CoreService::handleReloadGlobalLogOptions( const JsonObject& json ) {
    LOGGER_TRACE( cc::trace( "ask all to update settings" ) );
    Services::Service::resetLogOptions();
    Services::Message message = { GroupsNames::BROADCAST_GROUP, LOG_CMD, SET_HA_LOG_LEVEL };
    send( message );
}

JsonObject CoreService::handleRequestProvisionData( const JsonObject& params ) {
    auto& provisionData = ProvisionDataUtils::getManager().getProvisionData();
    auto result = provisionData.toJson();
    result[ "controllerKey" ] =
        tiny::tools::base64::encode( provisionData.getControllerKey()->rawKey(), CryptoUtils::KeyAes256::ms_keySize );
    return result;
}

JsonObject CoreService::handleRequestDBData( const JsonObject& params ) {
    return m_busDBManager->handleRequestDBData( params );
}

void CoreService::handlePerformDBOperation( const JsonObject& params ) {
    m_busDBManager->handlePerformDBOperation( params );
}

JsonObject CoreService::handleAddVeraDeviceMapping( const JsonObject& params ) {
    int veraDeviceId;
    JsonObject deviceData;

    if( !json_get( params, RPCParams::VeraDeviceId, veraDeviceId ) ||
        !json_get( params, RPCParams::DeviceData, deviceData ) ) {
        JsonObject result;
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    getDBClient().update( TempVeraDeviceMappingsDBName,
                          JSON_EMPTY_OBJ,
                          JSON( db::ID_FIELD_NAME << veraDeviceId << "deviceData" << deviceData ),
                          true );

    return JSON_EMPTY_OBJ;
}

void CoreService::sendCloudVideoforwardRequest( MessageSubClasses subClass, const Json& broadcast ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    if( subClass != HUB_ITEM_UPDATED ||
        json_get_str( broadcast, DictionaryEditRequestFields::NameFieldName ) != "doorbell_state" ||
        json_get_str( broadcast, DictionaryEditRequestFields::ValueFieldName ) != "ring" ) {
        return;
    }
    auto query =
        JSON( ItemFields::DeviceIdFieldName << json_get_str( broadcast, DictionaryEditRequestFields::DeviceIdFieldName )
                                            << ItemFields::NameFieldName << "camera_stream" );
    auto cameraStreamItem = m_itemsDb->getDBClient().findOne( DBNames::ItemsDB, std::move( query ), nullptr );
    if( !cameraStreamItem.is_object() || cameraStreamItem.empty() ) {
        LOGGER_TRACE( cc::warn( "Cannot send cloud.videoforward request: camera_stream item is not found, "
                                "doorbell_state itemId=" ) +
                      cc::note( json_get_str( broadcast, ItemFields::IdFieldName ) ) );
        return;
    }
    auto streamId = json_get_str( cameraStreamItem, ItemFields::ValueFieldName );
    if( streamId.empty() ) {
        LOGGER_TRACE( cc::warn( "Cannot send cloud.videoforward request: camera_stream item value is empty, "
                                "doorbell_state itemId=" ) +
                      cc::note( json_get_str( broadcast, ItemFields::IdFieldName ) ) );
        return;
    }
    sendRequest( GroupsNames::UI_GROUP, createCloudVideoforwardRequestData( streamId ), CLOUD_MESSAGE );
}

Json CoreService::createCloudVideoforwardRequestData( const std::string& identity ) {
    Json request = { { "method", "cloud.videoforward" }, { "api", "1.0" } };
    auto& params = request[ "params" ];
    params[ "target" ][ "identity" ] = identity;

    Json data = { { "action", "initiate_to_start_stream" }, { "req_rtsp_link", false }, { "rtsp_valid_for", 1000 } };
    data[ "identity" ] = identity;
    data[ "controller_uuid" ] = HubInfoHelper::getControllerUUID();
    data[ "rnd_client_id" ] = db::DbId::genOID();
    params[ "data" ] = data;
    return request;
}

void CoreService::handleBackupRestore( const JsonObject& request ) {
    Json res;
    if( !request.empty() || request.is_object() ) {
        JsonObject error;
        json_get_obj( request, "error", error );
        auto status = json_get_int( request, "completed" );
        if( error.empty() && status == 100 ) {
            m_swingerShutdownDevicesManager->clean();
            if( m_settingsManager->setSettingValue( "first_start", true ) ) {
                LOGGER_INFO( cc::info( "Set first_start flag to true successfully " ) );
            }
        }
    }
}

JsonObject CoreService::handleBackupCreate( const JsonObject& request ) {
    Log::info( cc::info( "Flush temporary values before backup..." ) );
    m_permanentDBWriteScheduler->cancelSchedule();
    m_permanentDBWriteScheduler->doNow();
    m_permanentDBWriteScheduler->schedule( WriteToPermanentDBInterval );
    LOGGER_DEBUG( cc::debug( "Flush temporary values before backup: completed" ) );

    CommonUtils::BackupHelper helper( getDBClient(), request );
    for( auto& tableName : DBNames::BackupTableNames ) {
        if( tableName == Model::Resources::DBNames::ItemsDB ) {
            if( !helper.backupDatabase( tableName, []( Json& item ) {
                    Model::Item::decryptItemAsJson( ProvisionDataUtils::getManager().getProvisionData(), item );
                } ) ) {
                return helper.getResponse();
            }
        }
        else {
            if( !helper.backupDatabase( tableName ) ) {
                return helper.getResponse();
            }
        }
    }

    return helper.getResponse();
}

void CoreService::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EVENT:
        handleEventMessage( message.msgSubClass, message.data );
        break;
    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void CoreService::handleEventMessage( const int subClass, const JsonObject& message ) {
    try {
        switch( subClass ) {
        case HOUSE_MODE_SWITCHED:
            handleHouseModeSwitched( message );
            break;
        default:;
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "Event handler error: " ) + cc::note( CommonUtils::whatError( ex ) ) );
    }
}

/* @brief handle house mode switched/done event: when switching from armed/protected
 *      to disarmed/not protected house mode, turn off all sirens.
 */
void CoreService::handleHouseModeSwitched( const JsonObject& message ) {
    m_houseModeAlarms->houseModeSwitchFromArmedToDisarmedMode( message );
}

void CoreService::swingerShutdownDeviceRemovedHandler( const Json& json ) {
    std::string deviceId = json_get_str( json, DeviceFields::IdFieldName );
    m_swingerShutdownDevicesManager->markDeviceRecordsAsDeleted( deviceId );
}

void CoreService::swingerShutdownItemUpdatedHandler( const Json& broadcastData ) {
    std::string deviceId = json_get_str( broadcastData, ItemFields::DeviceIdFieldName );
    bool syncNotification = json_get_bool( broadcastData, Model::Resources::System::SyncNotification );
    if( syncNotification ) {
        m_swingerShutdownDevicesManager->markDeviceRecordsAsDeleted( deviceId );
    }
}

void CoreService::swingerShutdownItemRemovedHandler( const Json& broadcastData ) {
    std::string deviceId = json_get_str( broadcastData, ItemFields::DeviceIdFieldName );
    m_swingerShutdownDevicesManager->markDeviceRecordsAsDeleted( deviceId );
}

JsonObject CoreService::getSwingerShutdownList( const JsonObject& request ) {
    return m_swingerShutdownDevicesManager->list( request );
}

JsonObject CoreService::swingerShutdownReset( const JsonObject& request ) {
    return m_swingerShutdownDevicesManager->swingerShutdownReset( request );
}

JsonObject CoreService::swingerShutdownDisableAdd( const JsonObject& request ) {
    return m_swingerShutdownDevicesManager->swingerShutdownDisableAdd( request );
}

JsonObject CoreService::swingerShutdownDisableRemove( const JsonObject& request ) {
    return m_swingerShutdownDevicesManager->swingerShutdownDisableRemove( request );
}

JsonObject CoreService::swingerShutdownLimitsSet( const JsonObject& request ) {
    return m_swingerShutdownDevicesManager->swingerShutdownLimitsSet( request );
}

JsonObject CoreService::swingerShutdownLimitsReset( const JsonObject& request ) {
    return m_swingerShutdownDevicesManager->swingerShutdownLimitsReset( request );
}

void CoreService::stopWalkTest() {
    LOGGER_INFO_RAW( "Finish walk test" );
    if( m_walkTest ) {
        m_securityItemsDbModifier->setTestMode( false );
        m_houseModeAlarms->setTestMode( false );
        m_walkTest.reset();
    }
}

JsonObject CoreService::protectSystemTest( const JsonObject& request ) {
    Json response;
    if( m_walkTest ) {
        {
            setError( response, Errors::WALK_TEST_ALREADY_STARTED );
            return response;
        }
    }

    m_walkTest = std::make_unique< HubPlatform::CoreD::WalkTest::WalkTest >( *this, *m_configReader );
    response = m_walkTest->protectSystemTestStart( request );
    if( hasError( response ) ) {
        return response;
    }
    LOGGER_INFO_RAW( "Start walk test" );
    m_securityItemsDbModifier->setTestMode( true );
    m_houseModeAlarms->setTestMode( true );
    scheduleStopWalkTest( m_walkTest, *this, m_walkTest->getIntervalSeconds(), [this]() { stopWalkTest(); } );

    return response;
}

JsonObject CoreService::protectSystemTestStop( const JsonObject& request ) {
    Json response;
    if( m_walkTest ) {
        response = m_walkTest->protectSystemTestStop( request );
        if( hasError( response ) ) {
            return response;
        }

        stopWalkTestIfNeeded();
    }
    else {
        setError( response, Errors::WALK_TEST_NOT_STARTED );
    }

    return response;
}

void CoreService::cleanWalkTestStopTask() {
    if( m_walkTestStopTask && m_walkTestStopTask->isActive() ) {
        m_walkTestStopTask->stop();
        m_walkTestStopTask.reset();
    }
}

void CoreService::stopWalkTestIfNeeded() {
    cleanWalkTestStopTask();
    stopWalkTest();
}

Services::PostponedTaskPtr CoreService::scheduleStopWalkTest( std::unique_ptr< WalkTest::WalkTest >& walkTest,
                                                              Services::Service& service,
                                                              std::chrono::seconds interval,
                                                              const WalkTestStopCallback& onWalkTestFinished ) {
    cleanWalkTestStopTask();
    m_walkTestStopTask = service.createPostponedTask( [&, onWalkTestFinished]( Services::PostponedTask& ) {
        auto timeLeft = walkTest->getLeftTime();
        if( walkTest && walkTest->getLeftTime() == std::chrono::seconds( 0 ) ) {
            if( onWalkTestFinished ) {
                onWalkTestFinished();
            }
        }
        else {
            scheduleStopWalkTest( walkTest, service, timeLeft, std::move( onWalkTestFinished ) );
        }
    } );
    m_walkTestStopTask->start( interval, false );
    return m_walkTestStopTask;
}

JsonObject CoreService::protectSystemTestExtend( const JsonObject& request ) {
    LOGGER_INFO_RAW( "Extend walk test" );
    Json response;
    if( m_walkTest ) {
        response = m_walkTest->protectSystemTestExtend( request );
        if( hasError( response ) ) {
            return response;
        }
    }
    else {
        setError( response, Errors::WALK_TEST_NOT_STARTED );
    }

    return response;
}

JsonObject CoreService::handleLocalAlarmOff( const JsonObject& params ) {
    try {
        auto result = m_houseModeAlarms->turnOffLocalAlarm( params );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to turn off local alarms: " ) + cc::note( ex.what() ) );
        JsonObject result;
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

void CoreService::setUpPredefinedItemGroups() {
    auto& dbClient = getDBClient();

    try {
        GroupOperationsUtils::ensurePredefinedItemGroupsInDb( dbClient, *m_configReader );
    }
    catch( const std::exception& error ) {
        LOGGER_ERROR( cc::error( "Cannot read predefine item groups configuration: " ) +
                      cc::error( CommonUtils::whatError( error ) ) );
    }
}

JsonObject CoreService::handleForceDisarm( const JsonObject& params ) {
    JsonObject result;
    if( params.empty() ) {
        setError( result, Errors::EMPTY_FIELD, "parameters should not be empty" );
        return result;
    }
    else {
        if( !params[ "modeId" ].is_string() ) {
            setError( result, Errors::WRONG_FIELD, "mode id must be a string" );
            return result;
        }
    }
    try {
        result = m_houseModeAlarms->forceDisarm( params );
        return result;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to force disarm: " ) + cc::note( ex.what() ) );
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }
}

void CoreService::setUpPredefinedDeviceGroups() {
    auto& dbClient = getDBClient();

    try {
        GroupOperationsUtils::ensurePredefinedDeviceGroupsInDb( dbClient, *m_configReader );
    }
    catch( const std::exception& error ) {
        LOGGER_ERROR( cc::error( "Cannot read predefined device groups configuration: " ) +
                      cc::error( CommonUtils::whatError( error ) ) );
    }
}

void CoreService::setUpPredefinedContactIds() {
    auto& dbClient = getDBClient();

    try {
        using namespace Protect;
        ProtectUtils::ensurePredefinedContactIdsInDb( dbClient, *m_configReader );
    }
    catch( const std::exception& error ) {
        LOGGER_ERROR( cc::error( "Cannot read predefined contact ids configuration: " ) +
                      cc::error( CommonUtils::whatError( error ) ) );
    }
}

void CoreService::setUpPredefinedSecurityConfigs() {
    auto& dbClient = getDBClient();

    try {
        using namespace Protect;
        ProtectUtils::ensurePredefinedSecurityConfigsInDb( dbClient, *m_configReader );
    }
    catch( const std::exception& error ) {
        LOGGER_ERROR( cc::error( "Cannot read predefined security configurations: " ) +
                      cc::error( CommonUtils::whatError( error ) ) );
    }
}

void CoreService::setUpPredefinedPolicies() {
    auto& dbClient = getDBClient();

    try {
        PolicyUpdater::ensurePredefinedPoliciesInDb( dbClient, *m_configReader );
    }
    catch( const std::exception& error ) {
        LOGGER_ERROR( cc::error( "Cannot read predefined policy configurations: " ) +
                      cc::error( CommonUtils::whatError( error ) ) );
    }
}

void CoreService::setUpGeneratedSensorGroups() {
    using namespace HubPlatform::CoreD::Resources;
    using namespace Policies;
    using namespace DeviceGroup;
    using namespace Protect;
    using namespace Protect::Resources;

    /* get all policies and iterate through them */
    JsonObject policies = m_policyHandler->processPolicyListRequest( JSON_EMPTY_OBJ )[ "policies" ];
    for( const auto& policy : policies ) {
        PolicyInfo policyInfo;
        policyInfo.fromJson( policy );
        auto policyId = policyInfo.getId();

        /* check if device group exists */
        if( m_deviceGroupHandler->isExistDeviceGroupWithName( policyInfo.getName() ) == false ) {
            /* get security config pointed to by policy */
            auto securityConfig =
                Protect::ProtectUtils::getSecurityConfigObject( getDBClient(), policyInfo.getSecurityConfig() );

            /* get entry delay type */
            auto entryDelay = json_get_str( securityConfig, SecurityConfig::EntryDelay );
            if( entryDelay.empty() ) {
                entryDelay = "none";
            }

            /* create new device group */
            Json deviceGroup;
            deviceGroup[ DeviceGroup::Name ] = policyInfo.getName();
            deviceGroup[ DeviceGroup::Categories ] = Json::array();
            deviceGroup[ DeviceGroup::Devices ] = Json::array();
            deviceGroup[ DeviceGroup::Exceptions ] = Json::array();
            deviceGroup[ DeviceGroup::ParentPolicyId ] = policyId;
            deviceGroup[ DeviceGroup::Persistent ] = false;
            deviceGroup[ DeviceGroup::EntryDelay ] = entryDelay;
            deviceGroup[ DeviceGroup::FollowEntry ] = false;
            deviceGroup[ DeviceGroup::Role ] = "sensor_group";
            deviceGroup[ DeviceGroup::PackageId ] = "";
            auto result = m_deviceGroupHandler->processCreateDeviceGroupRequest( deviceGroup );
            if( result.empty() ) {
                /* new entry appears to be created, now try and find it's ID ! */
                std::string devicesId = "";
                auto allGroups =
                    m_deviceGroupHandler->processDeviceGroupsListRequest( JSON_EMPTY_OBJ )[ "deviceGroups" ];
                for( const auto& group : allGroups ) {
                    DeviceGroupInfo deviceInfo;
                    deviceInfo.fromJson( group );
                    if( deviceInfo.getName() == policyInfo.getName() ) {
                        devicesId = deviceInfo.getId();
                    }
                }

                /* have we found it... */
                if( devicesId != "" ) {
                    /* ensure profile points to a device group instance */
                    m_policyHandler->processPolicyUpdateRequest(
                        JSON( "id" << policyId << PolicyFields::DeviceGroup << devicesId ) );
                }
            }
        }
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
