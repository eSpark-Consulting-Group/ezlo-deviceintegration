#include "cored/house_mode_alarms.h"
#include "cored/model/resources/cored_resources.h"

#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/script_events/house_mode_alarmed.h"
#include "cored/house_modes/resources/house_mode_resources.h"

#include "service_interfaces/postponed_task.h"
#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"

#include "config_reader/config_reader.h"
#include "common_utils/cpp14.h"

#include "common_utils/hub_info_helper.h"
#include "service_interfaces/task_loop.h"
#include "jsonrpc/jsonrpc.h"
#include "cored/house_modes/script_events/house_mode_entry_delay_started.h"
#include "cored/house_modes/script_events/house_mode_entry_delay_finished.h"
#include "cored/house_modes/script_events/house_mode_local_alarm_off.h"
#include "cored/protect/protect_utils.h"
#include "cored/protect/security_config.h"

#include <chrono>
#include <functional>
#include <utility>

namespace HubPlatform {
namespace CoreD {

namespace {

std::vector< std::string > getSirenItemNames() {
    return { "siren", "switch", "basic" };
}
}  // namespace

using namespace HubPlatform::CoreD::Model::Resources;

HouseModeAlarms::HouseModeAlarms( Services::Service& service, SetItemValue setItemValue )
: m_service( service )
, m_setItemValue( setItemValue ) {}

void HouseModeAlarms::addDeviceToHouseModesAlarmedList( const JsonObject& json, const std::string& id, int delay ) {
    if( delay > 0 ) {
        const auto type = json.find( ItemFields::TypeFieldName );
        auto timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
                             std::chrono::system_clock::now().time_since_epoch() )
                             .count();

        CoreDHouseModes::addDeviceToHouseModesAlarmedList(
            m_service.getDBClient(), id, timestamp, delay, m_silentAlarm );
    }
}

void HouseModeAlarms::deleteDeviceFromHouseModesAlarmedList( const std::string& id ) {
    CoreDHouseModes::deleteDeviceFromHouseModesAlarmedList( m_service.getDBClient(), id );
}

void HouseModeAlarms::checkSecurityThreat( const std::string& itemId ) {
    if( m_testMode )
        return;

    const JsonObject jsonTemp = m_service.getDBClient().findOne(
        Model::Resources::DBNames::TempItemsDB, JSON( Model::Resources::ItemFields::IdFieldName << itemId ), nullptr );
    const JsonObject json = m_service.getDBClient().findOne(
        Model::Resources::DBNames::ItemsDB, JSON( Model::Resources::ItemFields::IdFieldName << itemId ), nullptr );

    const auto itemNameIt = json.find( ItemFields::NameFieldName );
    const auto deviceIdIt = json.find( ItemFields::DeviceIdFieldName );
    const auto valueIt = jsonTemp.find( ItemFields::ValueFieldName );

    const bool allFieldsPresent = itemNameIt != json.end() && deviceIdIt != json.end() && valueIt != jsonTemp.end();
    if( !allFieldsPresent ) {
        return;
    }
    else if( !valueIt.value().is_boolean() ) {
        if( !valueIt.value().is_string() ) {
            return;
        }
    }

    const auto itemName = itemNameIt.value().get< std::string >();
    const std::string deviceId = deviceIdIt.value().get< std::string >();
    const JsonObject deviceObject =
        m_service.getDBClient().findOne( Model::Resources::DBNames::DevicesDB,
                                         JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ),
                                         nullptr );
    auto securityConfigId = json_get_str( deviceObject, Model::Resources::DeviceFields::ProtectConfig );
    auto securityConfigObject =
        Protect::ProtectUtils::getSecurityConfigObject( m_service.getDBClient(), securityConfigId );
    Protect::SecurityConfigInfo securityConfig;
    securityConfig.fromJson( securityConfigObject );
    m_soundType = securityConfig.getSoundType();
    auto chime = securityConfig.getChime();
    if( chime == "yes" ) {
        m_chime = true;
    }
    else {
        m_chime = false;
    }
    std::string protectAlarmValue;
    bool value = false;
    if( itemName == CoreDHouseModes::Resources::Items::ProtectAlarm ) {
        protectAlarmValue = valueIt.value().get< std::string >();
    }
    else {
        value = valueIt.value().get< bool >();
    }

    if( protectAlarmValue == CoreDHouseModes::Resources::ItemValues::AlarmActiveSilent || m_soundType == "silent" ) {
        m_silentAlarm = true;
    }
    else {
        m_silentAlarm = false;
    }
    bool deviceArmed = getDeviceArmedStatusByItem( json );

    if( !deviceArmed ) {
        deleteDeviceFromHouseModesAlarmedList( deviceId );
    }

    int delay = 0;
    if( ( isSecurityThreatEvent( deviceArmed, itemName, value ) ||
          isProtectAlarmEvent( itemName, protectAlarmValue ) ) &&
        securityConfig.getAlarmFlag() ) {
        LOGGER_INFO_RAW( cc::info( "House Modes: Security Threat Detected, itemData: " ) + cc::j( json ) );
        auto securityConfigDelay = securityConfig.getEntryDelay();
        if( securityConfigId == "default" || securityConfigDelay.empty() ) {
            auto deviceGroupDelay = CoreDHouseModes::fetchEntryDelayValue( m_service.getDBClient(), deviceId );
            if( deviceGroupDelay == -1 ) {
                delay = CoreDHouseModes::getAlarmDelay( m_service.getDBClient() );
            }
            else {
                delay = deviceGroupDelay;
            }
        }
        else {
            auto entryDelayObject = CoreDHouseModes::getEntryDelay( m_service.getDBClient() );
            delay = json_get_int( entryDelayObject, securityConfigDelay );
        }
        auto currentHouseModePtr = CoreDHouseModes::getCurrentHouseMode( m_service.getDBClient() );
        const bool bypassDevice = currentHouseModePtr->m_bypassDeviceIds.count( deviceId ) > 0;
        addDeviceToHouseModesAlarmedList( json, deviceId, delay );

        auto delayBypassThreadLambda = [this, json, deviceId, &delay]( Services::PostponedTask& ) -> void {
            bool deviceArmed = getDeviceArmedStatusByItem( json );
            if( deviceArmed ) {
                runSecurityThreatNotificationsWithBroadcast(
                    json, deviceId, [&]() { sendBypassDoneBroadcast( deviceId, delay ); } );
            }

            clearPostponedTasks();
            CoreDHouseModes::clearDeviceFromHouseModesAlarmedList( m_service.getDBClient() );
        };

        const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );

        auto delayThreadLambda = [this, json, deviceId, modeId, &delay]( Services::PostponedTask& ) -> void {
            std::lock_guard< std::mutex > lock( m_postponedMutex );
            bool deviceArmed = getDeviceArmedStatusByItem( json );
            auto itemName = json_get_str( json, ItemFields::NameFieldName );
            if( deviceArmed || itemName == "protect_alarm" ) {
                runSecurityThreatNotificationsWithBroadcast( json, deviceId, [&]() {
                    sendEntryDelayDoneBroadcast( deviceId, delay );
                    sendMainBeginBroadcast( deviceId, delay );
                    m_curentPhaseDeviceId = deviceId;
                    CoreDHouseModes::setAlarmPhase(
                        m_service.getDBClient(), HouseModeAlarmStatus::Main, HouseModeAlarmStatus::Begin );
                } );
                clearPostponedTasks();
                CoreDHouseModes::clearDeviceFromHouseModesAlarmedList( m_service.getDBClient() );
            }
        };

        auto updateTasks = [this]( Services::PostponedTaskPtr& task, const std::string deviceId ) {
            auto timestamp = std::chrono::duration_cast< std::chrono::milliseconds >(
                                 std::chrono::system_clock::now().time_since_epoch() )
                                 .count();
            auto newTask = std::make_pair( task, timestamp );
            auto tasks = m_postponedTasks[ deviceId ];
            tasks.push_back( newTask );
            m_postponedTasks[ deviceId ] = tasks;
        };

        if( bypassDevice ) {
            auto task = m_service.createPostponedTask( delayBypassThreadLambda );
            if( m_postponedTasks.empty() ) {
                task->start( std::chrono::seconds( delay ), false );
            }

            updateTasks( task, deviceId );
        }
        else {
            if( delay > 0 ) {
                std::lock_guard< std::mutex > lock( m_postponedMutex );
                auto task = m_service.createPostponedTask( delayThreadLambda );
                if( m_postponedTasks.empty() ) {
                    sendEntryDelayBeginBroadcast( deviceId, delay );
                    task->start( std::chrono::seconds( delay ), false );
                    CoreDHouseModes::setAlarmPhase(
                        m_service.getDBClient(), HouseModeAlarmStatus::EntryDelay, HouseModeAlarmStatus::Begin );
                }

                updateTasks( task, deviceId );
            }
            else {
                runSecurityThreatNotificationsWithBroadcast( json, deviceId, [this, deviceId, &delay]() {
                    sendMainBeginBroadcast( deviceId, delay );
                    m_curentPhaseDeviceId = deviceId;
                    CoreDHouseModes::setAlarmPhase(
                        m_service.getDBClient(), HouseModeAlarmStatus::Main, HouseModeAlarmStatus::Begin );
                } );
            }
        }
    }
}

bool HouseModeAlarms::getDeviceArmedStatusByItem( const JsonObject& item ) {
    const auto deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    const JsonObject device =
        m_service.getDBClient().findOne( DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );
    return json_get_bool( device, DeviceFields::ArmedFieldName, false );
}

void HouseModeAlarms::sendEntryDelayBeginBroadcast( const std::string& deviceId, int delay ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast for deviceId: " ) + cc::note( deviceId ) );
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Begin,
                                                    HouseModeAlarmStatus::EntryDelay,
                                                    deviceId,
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeEntryDelayStarted(
        m_silentAlarm, m_chime, m_soundType, CoreDHouseModes::getTimestamp() ) );
}

void HouseModeAlarms::sendEntryDelayDoneBroadcast( const std::string& deviceId, int delay ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast for deviceId: " ) + cc::note( deviceId ) );
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Done,
                                                    HouseModeAlarmStatus::EntryDelay,
                                                    deviceId,
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeEntryDelayFinished(
        m_silentAlarm,
        m_chime,
        m_soundType,
        CoreDHouseModes::getTimestamp(),
        CoreDHouseModes::Resources::EntryDelayStatus::DONE ) );
}

void HouseModeAlarms::sendEntryDelayCanceledBroadcast() {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast" ) );
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    auto delay = CoreDHouseModes::getAlarmDelay( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Canceled,
                                                    HouseModeAlarmStatus::EntryDelay,
                                                    "",
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeEntryDelayFinished(
        m_silentAlarm,
        m_chime,
        m_soundType,
        CoreDHouseModes::getTimestamp(),
        CoreDHouseModes::Resources::EntryDelayStatus::CANCELED ) );
}

void HouseModeAlarms::sendEntryDelaySkipBroadcast() {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast" ) );
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    auto delay = CoreDHouseModes::getAlarmDelay( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Begin,
                                                    HouseModeAlarmStatus::Main,
                                                    "",
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeEntryDelayFinished(
        m_silentAlarm,
        m_chime,
        m_soundType,
        CoreDHouseModes::getTimestamp(),
        CoreDHouseModes::Resources::EntryDelayStatus::SKIPPED ) );
}

void HouseModeAlarms::sendMainDoneBroadcast( const std::string& deviceId, int delay ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast for deviceId: " ) + cc::note( deviceId ) );
    m_silentAlarm = false;
    m_chime = false;
    m_soundType = "";
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Done,
                                                    HouseModeAlarmStatus::Main,
                                                    deviceId,
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeAlarmed( modeId,
                                                                     deviceId,
                                                                     HouseModeAlarmStatus::Main,
                                                                     HouseModeAlarmStatus::Done,
                                                                     delay,
                                                                     m_silentAlarm,
                                                                     m_chime,
                                                                     m_soundType ) );
}

void HouseModeAlarms::sendMainBeginBroadcast( const std::string& deviceId, int delay ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast for deviceId: " ) + cc::note( deviceId ) );
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Begin,
                                                    HouseModeAlarmStatus::Main,
                                                    deviceId,
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeAlarmed( modeId,
                                                                     deviceId,
                                                                     HouseModeAlarmStatus::Main,
                                                                     HouseModeAlarmStatus::Begin,
                                                                     delay,
                                                                     m_silentAlarm,
                                                                     m_chime,
                                                                     m_soundType ) );
}

void HouseModeAlarms::sendBypassDoneBroadcast( const std::string& deviceId, int delay ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: emitting broadcast for deviceId: " ) + cc::note( deviceId ) );
    const auto modeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
    CoreDHouseModes::sendHouseModeAlarmedBroadcast( m_service,
                                                    modeId,
                                                    HouseModeAlarmStatus::Done,
                                                    HouseModeAlarmStatus::Bypass,
                                                    deviceId,
                                                    m_silentAlarm,
                                                    m_chime,
                                                    m_soundType,
                                                    delay );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeAlarmed( modeId,
                                                                     deviceId,
                                                                     HouseModeAlarmStatus::Done,
                                                                     HouseModeAlarmStatus::Bypass,
                                                                     delay,
                                                                     m_silentAlarm,
                                                                     m_chime,
                                                                     m_soundType ) );
}

void HouseModeAlarms::clearPostponedTasks( const std::string& exceptedId ) {
    for( PostponedTasks::iterator it = m_postponedTasks.begin(); it != m_postponedTasks.end(); ) {
        const auto& deviceId = it->first;

        if( deviceId != exceptedId ) {
            auto& tasks = it->second;

            for( auto taskItem : tasks ) {
                auto task = taskItem.first;
                task->stop();
                task.reset();
            }

            it = m_postponedTasks.erase( it );
        }
        else {
            it++;
        }
    }
}

void HouseModeAlarms::runSecurityThreatNotificationsWithBroadcast( const JsonObject& json,
                                                                   const DeviceID& deviceId,
                                                                   std::function< void() > emitBroadcast ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: Security Threat Run, itemData: " ) + cc::j( json ) );
    static const std::vector< std::string > sirenItemNames = getSirenItemNames();
    static const std::vector< std::string > cameraItemNames = { CoreDHouseModes::Resources::Items::MakeRecording };

    auto currentHouseModePtr = CoreDHouseModes::getCurrentHouseMode( m_service.getDBClient() );
    const bool bypassDevice = currentHouseModePtr->m_bypassDeviceIds.count( deviceId ) > 0;

    auto itemValue = json_get_str( json, ItemFields::ValueFieldName );
    if( emitBroadcast != nullptr ) {
        emitBroadcast();
    }

    if( !bypassDevice && currentHouseModePtr != nullptr &&
        itemValue != CoreDHouseModes::Resources::ItemValues::AlarmActiveSilent && !m_testMode ) {
        notifyAllAlarms( sirenItemNames, *currentHouseModePtr );
    }

    if( currentHouseModePtr != nullptr && !m_testMode ) {
        notifyAllCameras( cameraItemNames, *currentHouseModePtr );
    }

    deleteDeviceFromHouseModesAlarmedList( deviceId );
}

void HouseModeAlarms::notifyAllAlarms( const std::vector< std::string >& alarmItemNames,
                                       CoreDHouseModes::HouseModeEntry& currentHouseMode ) {
    const auto& alarmOffDevices = currentHouseMode.m_alarmsOffDeviceIds;
    auto alarmDeviceIdsVector = CoreDHouseModes::getAlarmDeviceIDsAffectedByHouseModes( m_service.getDBClient() );
    auto alarmDeviceIds = std::set< std::string >( alarmDeviceIdsVector.begin(), alarmDeviceIdsVector.end() );
    std::set< std::string > deviceIds;
    std::set_difference( alarmDeviceIds.begin(),
                         alarmDeviceIds.end(),
                         alarmOffDevices.begin(),
                         alarmOffDevices.end(),
                         std::inserter( deviceIds, deviceIds.begin() ) );
    for( const auto& alarmDeviceId : deviceIds ) {
        auto items = m_service.getDBClient().findAll(
            DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << alarmDeviceId ), nullptr );
        for( auto& sirenItemName : alarmItemNames ) {
            LOGGER_INFO_RAW( cc::info( "House Modes Notify: Checking alarm item name, sirenItemName: " ) +
                             cc::note( sirenItemName ) );
            auto it = std::find_if( items.begin(), items.end(), [sirenItemName]( const JsonObject& item ) {
                return json_get_str( item, ItemFields::NameFieldName ) == sirenItemName;
            } );
            if( it != items.end() ) {
                const auto itemId = json_get_str( *it, ItemFields::IdFieldName );
                LOGGER_INFO_RAW( cc::info( "House Modes Notify: Found item for siren, itemId: " ) +
                                 cc::note( itemId ) );
                const JsonObject setItemValueRequest{ { db::ID_FIELD_NAME, itemId },
                                                      { ItemFields::ValueFieldName, true },
                                                      { RPCParams::SourceField,
                                                        { { ItemSource::TypeField, ItemSource::HouseMode } } } };
                m_setItemValue( setItemValueRequest );
                break;
            }
        }
    }
}

void HouseModeAlarms::notifyAllCameras( const std::vector< std::string >& cameraItemNames,
                                        CoreDHouseModes::HouseModeEntry& currentHouseMode ) {
    using namespace HubPlatform::CoreDHouseModes::Resources;
    const auto& camerasOffDevices = currentHouseMode.m_camerasOffDeviceIds;
    std::set< std::string > cameraDeviceIds;
    std::map< std::string, std::string > cameraDeviceItems;

    auto buildCollections = [&cameraDeviceIds, &cameraDeviceItems]( const Json& fields ) {
        const auto& deviceId = json_get_str( fields, db::ID_FIELD_NAME );
        const auto& itemId = json_get_str( fields, Items::MakeRecording );
        cameraDeviceIds.insert( deviceId );
        cameraDeviceItems.emplace( deviceId, itemId );
    };

    if( 0 == CoreDHouseModes::getCamerasDeviceItemPairs( m_service.getDBClient(), JSON_EMPTY_OBJ, buildCollections ) )
        return;

    std::set< std::string > deviceIdsToNotify;
    std::set_difference( cameraDeviceIds.begin(),
                         cameraDeviceIds.end(),
                         camerasOffDevices.begin(),
                         camerasOffDevices.end(),
                         std::inserter( deviceIdsToNotify, deviceIdsToNotify.begin() ) );
    for( const auto& cameraDeviceId : deviceIdsToNotify ) {
        auto foundIt = cameraDeviceItems.find( cameraDeviceId );
        if( foundIt != cameraDeviceItems.end() ) {
            const auto& itemId = foundIt->second;
            LOGGER_INFO_RAW( cc::info( "House Modes Notify: Found item for camera recording, itemId: " ) +
                             cc::note( itemId ) );
            const int DefaultPlaybackTime = 0;
            const JsonObject setItemValueRequest{ { db::ID_FIELD_NAME, itemId },
                                                  { ItemFields::ValueFieldName, DefaultPlaybackTime },
                                                  { RPCParams::SourceField,
                                                    { { ItemSource::TypeField, ItemSource::HouseMode } } } };
            m_setItemValue( setItemValueRequest );
            LOGGER_TRACE( cc::trace( "handleSetItemValue called for itemId=" ) + itemId );

            break;
        }
    }
}

void HouseModeAlarms::turnOffAlarms( const std::vector< std::string >& alarmItemNames ) {
    LOGGER_INFO_RAW( cc::info( "House Modes: Starting to turn off alarms" ) );
    CoreDHouseModes::DeviceIDs alarmIds =
        CoreDHouseModes::getAlarmDeviceIDsAffectedByHouseModes( m_service.getDBClient() );

    Json fields = JSON( ItemFields::IdFieldName << 1 << ItemFields::NameFieldName << 1 );
    for( const auto& alarmDeviceId : alarmIds ) {
        auto items = m_service.getDBClient().findAll(
            DBNames::ItemsDB, JSON( ItemFields::DeviceIdFieldName << alarmDeviceId ), &fields );
        const bool alarmNewValue = false;

        for( auto& sirenItemName : alarmItemNames ) {
            auto it = std::find_if( items.begin(), items.end(), [sirenItemName]( const JsonObject& item ) {
                return json_get_str( item, ItemFields::NameFieldName ) == sirenItemName;
            } );
            if( it != items.end() ) {
                const auto itemId = json_get_str( *it, ItemFields::IdFieldName );
                LOGGER_INFO_RAW( cc::info( "House Modes: Found item to turn off, itemId: " + cc::note( itemId ) ) );
                const JsonObject setItemValueRequest{ { db::ID_FIELD_NAME, itemId },
                                                      { ItemFields::ValueFieldName, alarmNewValue },
                                                      { RPCParams::SourceField,
                                                        { { ItemSource::TypeField, ItemSource::HouseMode } } } };
                m_setItemValue( setItemValueRequest );
                break;
            }
        }
    }
}

void HouseModeAlarms::houseModeSwitchFromArmedToDisarmedMode( const JsonObject& message ) {
    int switchToDelay = json_get_int( message, "switchToDelay", -1 );
    if( switchToDelay != 0 )
        return;
    std::string status = json_get_str( message, "status" );
    if( status != "done" )
        return;
    std::string modeFrom = json_get_str( message, "from" );
    std::string modeTo = json_get_str( message, "to" );
    LOGGER_INFO_RAW( cc::info( "House Modes: Switch to disarmed initiated, " + cc::j( message ) ) );

    auto modeFromEntry = CoreDHouseModes::getHouseMode( m_service.getDBClient(), modeFrom );
    if( modeFromEntry && modeFromEntry->m_armed ) {
        auto modeToEntry = CoreDHouseModes::getHouseMode( m_service.getDBClient(), modeTo );
        if( modeToEntry && !modeToEntry->m_armed ) {
            LOGGER_INFO_RAW( cc::info( "House Modes: Turning off alarms and clearing security events" ) );
            turnOffAlarms( getSirenItemNames() );
            if( !m_postponedTasks.empty() ) {
                clearAllPendingSecurityEventsAndCountdown();
            }
            {
                std::lock_guard< std::mutex > lock( m_mutex );
                std::string phase;
                std::string status;

                std::tie( phase, status ) = CoreDHouseModes::getAlarmPhase( m_service.getDBClient() );
                if( phase == HouseModeAlarmStatus::Main && status == HouseModeAlarmStatus::Begin ) {
                    CoreDHouseModes::setAlarmPhase(
                        m_service.getDBClient(), HouseModeAlarmStatus::Main, HouseModeAlarmStatus::Done );
                    auto delay = CoreDHouseModes::getAlarmDelay( m_service.getDBClient() );
                    sendMainDoneBroadcast( m_curentPhaseDeviceId, delay );
                }
            }
        }
    }
}

void HouseModeAlarms::clearAllPendingSecurityEventsAndCountdown() {
    clearPostponedTasks();
    sendEntryDelayCanceledBroadcast();
    CoreDHouseModes::clearDeviceFromHouseModesAlarmedList( m_service.getDBClient() );
    CoreDHouseModes::setAlarmPhase(
        m_service.getDBClient(), HouseModeAlarmStatus::EntryDelay, HouseModeAlarmStatus::Canceled );
}

void HouseModeAlarms::cancelEntryDelay() {
    LOGGER_INFO_RAW( cc::info( "House Modes: Received request to cancel entry delay" ) );
    clearAllPendingSecurityEventsAndCountdown();
}

JsonObject HouseModeAlarms::skipEntryDelay() {
    LOGGER_INFO_RAW( cc::info( "House Modes: Received request to skip entry delay" ) );
    if( !m_postponedTasks.empty() ) {
        clearPostponedTasks();
        const JsonObject json;
        runSecurityThreatNotificationsWithBroadcast( json, "", [this]() { sendEntryDelaySkipBroadcast(); } );
        return JSON_EMPTY_OBJ;
    }
    else {
        LOGGER_ERROR( cc::error( "House Modes system is not in an Entry Delay state" ) );
        JsonObject result;
        setError( result, Errors::HOUSE_MODES_INVALID_PHASE );
        return result;
    }
}

JsonObject HouseModeAlarms::setEntryDelay( const JsonObject& params ) {
    CoreDHouseModes::setEntryDelay( m_service, m_service.getDBClient(), params );
    return JSON_EMPTY_OBJ;
}

JsonObject HouseModeAlarms::resetEntryDelay() {
    CoreDHouseModes::resetEntryDelay( m_service, m_service.getDBClient() );
    return JSON_EMPTY_OBJ;
}

JsonObject HouseModeAlarms::turnOffLocalAlarm( const JsonObject& params ) {
    if( !params.empty() ) {
        LOGGER_INFO_RAW(
            cc::info( "House Modes: Received request to turn off local alarm with params: " + cc::j( params ) ) );
        auto modeId = json_get_str( params, "modeId" );
        auto armed = json_get_bool( params, "armed", false );
        auto currentModeId = CoreDHouseModes::getCurrentHouseModeId( m_service.getDBClient() );
        if( !modeId.empty() && modeId == currentModeId ) {
            turningOffLocalAlarms( modeId );
        }
        else {
            auto houseMode = CoreDHouseModes::getHouseMode( m_service.getDBClient(), currentModeId );
            if( json_contains( params, "armed" ) && armed == houseMode->m_armed ) {
                turningOffLocalAlarms( modeId );
            }
            else {
                JsonObject result;
                setError( result, Errors::HOUSE_MODE_ID_DOES_NOT_CORRESPOND );
                return result;
            }
        }
    }
    else {
        JsonObject result;
        setError( result, Errors::EMPTY_FIELD );
        return result;
    }
    return JSON_EMPTY_OBJ;
}

void HouseModeAlarms::turningOffLocalAlarms( std::string& modeId ) {
    turnOffAlarms( getSirenItemNames() );
    LOGGER_INFO_RAW( cc::info( "House Modes: sending local alarms turned off broadcast" ) );
    m_service.sendBroadcast( HOUSE_MODES_LOCAL_ALARMS_TURNED_OFF, JSON_EMPTY_OBJ );
    m_service.emit( CoreDHouseModes::ScriptEvents::HouseModeLocalAlarmOff( modeId, CoreDHouseModes::getTimestamp() ) );
}

JsonObject HouseModeAlarms::forceDisarm( const JsonObject& params ) {
    auto result = JSON_EMPTY_OBJ;
    auto modeId = json_get_str( params, "modeId" );
    auto currentHouseMode = CoreDHouseModes::getCurrentHouseMode( m_service.getDBClient() );
    if( modeId == currentHouseMode->m_id && !currentHouseMode->m_armed ) {
        if( !m_postponedTasks.empty() ) {
            clearPostponedTasks();
        }
        turnOffAlarms( getSirenItemNames() );
        CoreDHouseModes::clearDeviceFromHouseModesAlarmedList( m_service.getDBClient() );
        CoreDHouseModes::setAlarmPhase( m_service.getDBClient(), "idle", "" );
        sendMainDoneBroadcast( "", 0 );
    }
    else {
        setError( result, Errors::HOUSE_MODE_ID_DOES_NOT_CORRESPOND );
    }
    return result;
}

bool HouseModeAlarms::isProtectAlarmEvent( const std::string& itemName, const std::string& itemValue ) {
    return itemName == CoreDHouseModes::Resources::Items::ProtectAlarm &&
           ( itemValue == CoreDHouseModes::Resources::ItemValues::AlarmActive ||
             itemValue == CoreDHouseModes::Resources::ItemValues::AlarmActiveSilent );
}

bool HouseModeAlarms::isSecurityThreatEvent( bool deviceArmed, const std::string& itemName, bool itemValue ) {
    return deviceArmed && itemName == CoreDHouseModes::Resources::Items::SecurityThreat && itemValue;
}

void HouseModeAlarms::setTestMode( bool testMode ) {
    m_testMode = testMode;
}

}  // namespace CoreD
}  // namespace HubPlatform
