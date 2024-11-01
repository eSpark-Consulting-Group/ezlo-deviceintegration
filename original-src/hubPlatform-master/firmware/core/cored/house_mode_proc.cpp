#include "cored/house_mode_proc.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/script_events/house_mode_switched.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/core_service.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "jsonrpc/message.h"
#include "log_utils/ezlo_log.h"
#include "cored/house_modes/script_events/house_mode_exit_time_started.h"
#include "cored/house_modes/script_events/house_mode_exit_time_finished.h"
#include "cored/house_modes/script_events/house_mode_exit_time_runs_out.h"
#include "cored/protect/protect_utils.h"
#include "cored/protect/security_config.h"

#include <chrono>
#include <cassert>

namespace HubPlatform {
namespace CoreD {

using namespace HubPlatform::CoreD::Model::Resources;
using namespace HubPlatform::CoreDHouseModes::Resources;

HouseModeProcessor::HouseModeProcessor( CoreService& service, IDbClient& dbClient )
: m_dbClient( dbClient )
, m_hostService( service )
, m_configReader()
, m_isSwitchingNow( false ) {}

HouseModeProcessor::HouseModeProcessor( CoreService& service, IDbClient& dbClient, ConfigReader&& configReader )
: m_dbClient( dbClient )
, m_hostService( service )
, m_configReader( std::move( configReader ) )
, m_isSwitchingNow( false ) {}

HouseModeProcessor::~HouseModeProcessor() {
    cancelSwitchingHouseMode();
}

void HouseModeProcessor::switchThreadFunction( const std::string& houseModeId,
                                               const CoreDHouseModes::HouseModeEntry& newHouseMode,
                                               int delay ) {
    using CoreDHouseModes::ScriptEvents::HouseModeSwitched;

    m_isSwitchingNow = true;
    bool isRunsOutTime = false;

    m_dbClient.update(
        HUBTempSettings,
        JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ),
        JSON( "$set" << JSON( CoreDHouseModes::HOUSE_MODE_SWITCH_ENTRY
                              << newHouseMode.m_id << CoreDHouseModes::HOUSE_MODE_TIME_TO_SWITCH << delay ) ) );

    using namespace CoreDHouseModes::Resources::BroadcastFields;
    auto infoSwitchStarted =
        JSON( "from" << houseModeId << "to" << newHouseMode.m_id << "status"
                     << "begin"
                     << "switchToDelay" << delay << DeviceTimestampFieldName << CoreDHouseModes::now() );
    m_hostService.sendBroadcast( HOUSE_MODE_SWITCHED, infoSwitchStarted );
    CoreDHouseModes::setSwitchToPhase( m_dbClient, "begin" );
    m_hostService.emit( HouseModeSwitched( houseModeId, newHouseMode.m_id, "begin", delay ) );
    if( newHouseMode.m_id != "1" ) {
        m_hostService.emit(
            CoreDHouseModes::ScriptEvents::HouseModeExitTimeStarted( m_silentAlarm, CoreDHouseModes::getTimestamp() ) );
    }

    if( delay > 0 ) {
        auto startTime = std::chrono::steady_clock::now();
        auto endTime = startTime + std::chrono::seconds( delay );
        std::chrono::seconds timeToSwitch( delay );
        if( timeToSwitch < ExitTimeRunsOut ) {
            isRunsOutTime = true;
        }

        do {
            m_dbClient.update(
                HUBTempSettings,
                JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ),
                JSON( "$set" << JSON( CoreDHouseModes::HOUSE_MODE_TIME_TO_SWITCH << timeToSwitch.count() ) ) );

            std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
            timeToSwitch =
                std::chrono::duration_cast< std::chrono::seconds >( endTime - std::chrono::steady_clock::now() );
            if( timeToSwitch <= ExitTimeRunsOut && !isRunsOutTime && m_isSwitchingNow && newHouseMode.m_id != "1" &&
                !m_silentAlarm ) {
                m_hostService.emit( CoreDHouseModes::ScriptEvents::HouseModeExitTimeRunsOut(
                    m_silentAlarm, CoreDHouseModes::getTimestamp() ) );
                isRunsOutTime = true;
            }
        } while( m_isSwitchingNow && ( timeToSwitch.count() > 0 ) );
    }

    if( m_isSwitchingNow ) {
        setHouseMode( newHouseMode.m_id );
        auto infoSwitchDone =
            JSON( "from" << houseModeId << "to" << newHouseMode.m_id << "status"
                         << "done"
                         << "switchToDelay" << 0 << DeviceTimestampFieldName << CoreDHouseModes::now() );
        m_hostService.sendBroadcast( HOUSE_MODE_SWITCHED, infoSwitchDone );
        m_hostService.emit( HouseModeSwitched( houseModeId, newHouseMode.m_id, "done", 0 ) );
        if( newHouseMode.m_id != "1" ) {
            m_hostService.emit( CoreDHouseModes::ScriptEvents::HouseModeExitTimeFinished(
                m_silentAlarm, CoreDHouseModes::getTimestamp() ) );
        }
        CoreDHouseModes::setSwitchToPhase( m_dbClient, "" );
        CoreDHouseModes::removeAllBypassDevices( m_hostService, houseModeId );
        m_hostService.handleHouseModeSwitched( infoSwitchDone );
    }
    else {
        auto infoSwitchCancel =
            JSON( "from" << houseModeId << "to" << newHouseMode.m_id << "status"
                         << "cancel"
                         << "switchToDelay" << 0 << DeviceTimestampFieldName << CoreDHouseModes::now() );
        m_hostService.sendBroadcast( HOUSE_MODE_SWITCHED, infoSwitchCancel );
        m_hostService.emit( HouseModeSwitched( houseModeId, newHouseMode.m_id, "cancel", 0 ) );
        if( newHouseMode.m_id != "1" ) {
            m_hostService.emit( CoreDHouseModes::ScriptEvents::HouseModeExitTimeFinished(
                m_silentAlarm, CoreDHouseModes::getTimestamp() ) );
        }
        CoreDHouseModes::setSwitchToPhase( m_dbClient, "" );
    }

    m_dbClient.update( HUBTempSettings,
                       JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ),
                       JSON( "$set" << JSON( CoreDHouseModes::HOUSE_MODE_SWITCH_ENTRY
                                             << "" << CoreDHouseModes::HOUSE_MODE_TIME_TO_SWITCH << 0 ) ) );

    m_isSwitchingNow = false;
}

JsonObject HouseModeProcessor::handleSetHouseMode( const std::string& nextHouseModeId ) {
    JsonObject result = JSON_EMPTY_OBJ;

    auto possibleModes = CoreDHouseModes::getHouseModesList( m_dbClient );
    const CoreDHouseModes::HouseModeEntry* nextHouseModePtr = nullptr;
    for( const auto& elem : possibleModes ) {
        if( elem.m_id == nextHouseModeId ) {
            nextHouseModePtr = &elem;
            break;
        }
    }

    if( !nextHouseModePtr ) {
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    const auto currentHouseModeId = CoreDHouseModes::getCurrentHouseModeId( m_dbClient );
    if( nextHouseModePtr->m_id == currentHouseModeId ) {
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    const auto switchToHouseModeId = CoreDHouseModes::getHouseModeSwitchTo( m_dbClient );
    if( nextHouseModePtr->m_id == switchToHouseModeId ) {
        const int remainingDelay = CoreDHouseModes::getSwitchDelayLeft( m_dbClient );
        if( remainingDelay < 0 ) {
            setError( result, Errors::INTERNAL_OPERATION_ERROR );
            return result;
        }

        result[ "result" ][ "switchToDelay" ] = remainingDelay;
        return result;
    }

    auto newHouseMode = *nextHouseModePtr;
    int delay = 0;
    if( !newHouseMode.m_ignoreDelay ) {
        delay = CoreDHouseModes::getSwitchDelay( m_dbClient );
        if( delay < 0 ) {
            setError( result, Errors::INTERNAL_OPERATION_ERROR );
            return result;
        }
    }
    cancelSwitchingHouseMode();
    m_switchingThread = std::thread( [this, currentHouseModeId, newHouseMode, delay]() {
        switchThreadFunction( currentHouseModeId, newHouseMode, delay );
    } );

    result[ "result" ][ "switchToDelay" ] = delay;
    return result;
}

void HouseModeProcessor::cancelSwitchingHouseMode() {
    m_isSwitchingNow = false;
    wait();

    assert( !m_switchingThread.joinable() );
}

JsonObject HouseModeProcessor::handleAddDisarmedDevice( const std::string& houseModeId, const std::string& deviceId ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    if( !CoreDHouseModes::addDisarmedDevice(
            m_hostService, m_hostService.getDevicesDbModifier(), houseModeId, deviceId ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    m_hostService.sendBroadcast( HOUSE_MODE_DISARMED_DEVICES_ADDED,
                                 JSON( BroadcastFields::ModeIdFieldName
                                       << houseModeId << BroadcastFields::DisarmedDeviceFieldName << deviceId ) );

    return result;
}

JsonObject HouseModeProcessor::handleRemoveDisarmedDevice( const std::string& houseModeId,
                                                           const std::string& deviceId,
                                                           bool changeArmedState ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::WRONG_PARAMS );
        return result;
    }

    if( !CoreDHouseModes::removeDisarmedDevice(
            m_hostService, m_hostService.getDevicesDbModifier(), houseModeId, deviceId, changeArmedState ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    m_hostService.sendBroadcast( HOUSE_MODE_DISARMED_DEVICES_REMOVED,
                                 JSON( BroadcastFields::ModeIdFieldName
                                       << houseModeId << BroadcastFields::DisarmedDeviceFieldName << deviceId ) );

    return result;
}

JsonObject HouseModeProcessor::handleSetNotifications( const std::string& houseModeId,
                                                       bool notifyAll,
                                                       const std::vector< std::string >& userIds ) {
    JsonObject result = JSON_EMPTY_OBJ;

    for( auto& userId : userIds ) {
        if( userId.empty() || userId.length() > 64 ) {
            setError( result, Errors::WRONG_PARAMS );
            return result;
        }
    }

    if( !CoreDHouseModes::setNotifications(
            m_dbClient, m_hostService, m_hostService.getDevicesDbModifier(), houseModeId, notifyAll, userIds ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    return result;
}

bool HouseModeProcessor::setHouseMode( const std::string& houseModeId ) {
    try {
        m_dbClient.update( HUBSettings,
                           JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ),
                           JSON( "$set" << JSON( CoreDHouseModes::HOUSE_MODE_ENTRY << houseModeId ) ) );
        if( !updateDisarmedDevice( houseModeId ) ) {
            Log::error( "HouseModeProcessor::setHouseMode updateDisarmedDevice is failed" );
            return false;
        }
    }
    catch( const std::exception& ex ) {
        Log::error( "HouseModeProcessor::setHouseMode is failed, exception : " + std::string( ex.what() ) );
        return false;
    }
    catch( ... ) {
        Log::error( "HouseModeProcessor::setHouseMode is failed, unknown error" );
        return false;
    }
    return true;
}

bool HouseModeProcessor::updateDisarmedDevice( const std::string& houseModeId ) {
    auto disarmedDeviceIds = CoreDHouseModes::getHouseMode( m_dbClient, houseModeId )->m_disarmedDeviceIds;
    auto deviceIds = CoreDHouseModes::getArmedDeviceIDsAffectedByHouseModes( m_dbClient );
    bool deviceArmed;

    for( const auto& deviceId : deviceIds ) {
        const JsonObject device =
            m_dbClient.findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
        if( device.empty() ) {
            Log::error( "HouseModeProcessor::updateDisarmedDevice: device " + deviceId + " empty " );
            return false;
        }

        if( !json_get( device, "armed", deviceArmed ) ) {
            Log::error( "HouseModeProcessor::updateDisarmedDevice: device " + deviceId + " empty field armed " );
            return false;
        }

        auto securityConfigId = json_get_str( device, DeviceFields::ProtectConfig );

        if( securityConfigId.empty() || securityConfigId == "default" ) {
            auto it = disarmedDeviceIds.find( deviceId );
            if( it == disarmedDeviceIds.end() && deviceArmed )
                continue;

            else if( it == disarmedDeviceIds.end() && !deviceArmed ) {
                m_hostService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::ArmedFieldName << true ) );
            }
            else if( it != disarmedDeviceIds.end() && deviceArmed ) {
                m_hostService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::ArmedFieldName << false ) );
            }
        }
        else {
            auto securityConfigJson = Protect::ProtectUtils::getSecurityConfigObject( m_dbClient, securityConfigId );
            auto houseModeArmedWhen = houseModeIdToArmedWhenField( houseModeId );
            Protect::SecurityConfigInfo securityConfig;
            securityConfig.fromJson( securityConfigJson );
            auto armedWhenArray = securityConfig.getArmedWhen();
            bool armed = false;
            for( auto& armedWhen : armedWhenArray ) {
                if( armedWhen == houseModeArmedWhen ) {
                    armed = true;
                    break;
                }
            }
            if( armed ) {
                m_hostService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::ArmedFieldName << true ) );
            }
            else {
                m_hostService.getDevicesDbModifier().update( deviceId, JSON( DeviceFields::ArmedFieldName << false ) );
            }
        }
    }
    return true;
}

Json HouseModeProcessor::fetchDeviceById( const std::string& deviceId ) const {
    return m_dbClient.findOne( DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );
}

bool HouseModeProcessor::hasDeviceById( const std::string& deviceId ) const {
    const auto device = fetchDeviceById( deviceId );
    return !device.empty();
}

void HouseModeProcessor::wait() {
    if( m_switchingThread.joinable() )
        m_switchingThread.join();
}

JsonObject HouseModeProcessor::handleAddAlarmsOffDevice( const std::string& modeId, const std::string& deviceId ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::WRONG_PARAMS, "No device by this ID" );
        return result;
    }

    const bool absentModeIsError = true;
    const auto houseMode = CoreDHouseModes::getHouseMode( m_dbClient, modeId, absentModeIsError );
    if( !houseMode ) {
        setError( result, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return result;
    }

    using namespace CoreDHouseModes;
    if( !addAlarmsOffDevice( m_dbClient, modeId, deviceId ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    using namespace CoreDHouseModes::Resources;
    m_hostService.sendBroadcast(
        HOUSE_MODE_ALARMS_OFF_ADDED,
        JSON( BroadcastFields::ModeIdFieldName << modeId << BroadcastFields::AlarmsOffDeviceFieldName << deviceId ) );

    if( houseMode->m_disarmedDefault ) {
        setDisarmedDefault( this->m_hostService, m_hostService.getDevicesDbModifier(), modeId, false );
    }

    return result;
}

JsonObject HouseModeProcessor::handleRemoveAlarmsOffDevice( const std::string& modeId, const std::string& deviceId ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::WRONG_PARAMS, "No device by this ID" );
        return result;
    }

    const bool absentModeIsError = true;
    const auto houseMode = CoreDHouseModes::getHouseMode( m_dbClient, modeId, absentModeIsError );
    if( !houseMode ) {
        setError( result, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return result;
    }

    using namespace CoreDHouseModes;
    using namespace CoreDHouseModes::Resources;
    if( !removeAlarmsOffDevice( m_dbClient, modeId, deviceId ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    m_hostService.sendBroadcast(
        HOUSE_MODE_ALARMS_OFF_REMOVED,
        JSON( BroadcastFields::ModeIdFieldName << modeId << BroadcastFields::AlarmsOffDeviceFieldName << deviceId ) );

    if( houseMode->m_disarmedDefault ) {
        setDisarmedDefault( this->m_hostService, m_hostService.getDevicesDbModifier(), modeId, false );
    }

    return result;
}

JsonObject HouseModeProcessor::handleAddCamerasOffDevice( const std::string& modeId, const std::string& deviceId ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::WRONG_PARAMS, "No device by this ID" );
        return result;
    }

    const bool absentModeIsError = true;
    const auto houseMode = CoreDHouseModes::getHouseMode( m_dbClient, modeId, absentModeIsError );
    if( !houseMode ) {
        setError( result, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return result;
    }

    using namespace CoreDHouseModes;
    using namespace CoreDHouseModes::Resources;
    if( !addCamerasOffDevice( m_dbClient, modeId, deviceId ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    m_hostService.sendBroadcast(
        HOUSE_MODE_CAMERAS_OFF_ADDED,
        JSON( BroadcastFields::ModeIdFieldName << modeId << BroadcastFields::CamerasOffDeviceFieldName << deviceId ) );

    if( houseMode && houseMode->m_disarmedDefault ) {
        setDisarmedDefault( this->m_hostService, m_hostService.getDevicesDbModifier(), modeId, false );
    }

    return result;
}

JsonObject HouseModeProcessor::handleRemoveCamerasOffDevice( const std::string& modeId, const std::string& deviceId ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::WRONG_PARAMS, "No device by this ID" );
        return result;
    }

    const bool absentModeIsError = true;
    const auto houseMode = CoreDHouseModes::getHouseMode( m_dbClient, modeId, absentModeIsError );
    if( !houseMode ) {
        setError( result, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return result;
    }

    using namespace CoreDHouseModes;
    using namespace CoreDHouseModes::Resources;

    if( !removeCamerasOffDevice( m_dbClient, modeId, deviceId ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    m_hostService.sendBroadcast(
        HOUSE_MODE_CAMERAS_OFF_REMOVED,
        JSON( BroadcastFields::ModeIdFieldName << modeId << BroadcastFields::CamerasOffDeviceFieldName << deviceId ) );

    if( houseMode->m_disarmedDefault ) {
        setDisarmedDefault( this->m_hostService, m_hostService.getDevicesDbModifier(), modeId, false );
    }

    return result;
}

JsonObject HouseModeProcessor::handleAddBypassDevices( const std::string& modeId,
                                                       const std::vector< std::string >& deviceIds ) {
    JsonObject result = JSON_EMPTY_OBJ;

    const bool absentModeIsError = true;
    const auto houseMode = CoreDHouseModes::getHouseMode( m_dbClient, modeId, absentModeIsError );
    if( !houseMode ) {
        setError( result, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return result;
    }

    std::vector< std::string > actualIds = filterExistingDevicesIds( deviceIds, result );
    for( auto it = actualIds.begin(); it != actualIds.end(); ) {
        if( !isBypassAllowedForDevice( *it ) ) {
            it = actualIds.erase( it );
        }
        else {
            ++it;
        }
    }
    if( actualIds.empty() ) {
        setError( result, Errors::WRONG_PARAMS, "No valid devices to add" );
        return result;
    }

    const bool added = CoreDHouseModes::addBypassDevices( m_hostService, modeId, actualIds );
    if( !added ) {
        LOGGER_DEBUG( cc::debug( "All devices already present in bypass list" ) );
    }

    return result;
}

JsonObject HouseModeProcessor::handleRemoveBypassDevices( const std::string& modeId,
                                                          const std::vector< std::string >& deviceIds ) {
    JsonObject result = JSON_EMPTY_OBJ;

    const bool absentModeIsError = true;
    const auto houseMode = CoreDHouseModes::getHouseMode( m_dbClient, modeId, absentModeIsError );
    if( !houseMode ) {
        setError( result, Errors::HOUSE_MODE_DOES_NOT_EXIST );
        return result;
    }

    const std::vector< std::string > actualIds = filterExistingDevicesIds( deviceIds, result );
    if( actualIds.empty() ) {
        setError( result, Errors::WRONG_PARAMS, "No valid devices to remove" );
        return result;
    }

    const bool removed = CoreDHouseModes::removeBypassDevices( m_hostService, modeId, actualIds );
    if( !removed ) {
        LOGGER_DEBUG( cc::debug( "None of devices was present in bypass list" ) );
    }

    return result;
}

JsonObject HouseModeProcessor::handleSetDisarmedDefault( const std::string& houseModeId, bool disarmedDefault ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !CoreDHouseModes::setDisarmedDefault(
            m_hostService, m_hostService.getDevicesDbModifier(), houseModeId, disarmedDefault ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    return result;
}

JsonObject HouseModeProcessor::handleSetProtect( const std::string& houseModeId, bool protect ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( !CoreDHouseModes::setProtect( m_hostService, m_hostService.getDevicesDbModifier(), houseModeId, protect ) ) {
        setError( result, Errors::INTERNAL_OPERATION_ERROR );
        return result;
    }

    return result;
}

JsonObject HouseModeProcessor::handleSetProtectWithTimestamp( const std::string& houseModeId,
                                                              bool protect,
                                                              std::chrono::milliseconds timestamp ) {
    JsonObject result = JSON_EMPTY_OBJ;

    if( CoreDHouseModes::setProtectTimestamp( m_hostService.getDevicesDbModifier(), houseModeId, timestamp ) ) {
        if( !CoreDHouseModes::setProtect(
                m_hostService, m_hostService.getDevicesDbModifier(), houseModeId, protect ) ) {
            setError( result, Errors::INTERNAL_OPERATION_ERROR );
            return result;
        }
    }
    else {
        setError( result, Errors::HOUSE_MODE_COMMAND_IS_OUTDATED );
        return result;
    }

    return result;
}

JsonObject HouseModeProcessor::handleAddDevice( const std::string& deviceId, const JsonObject& device ) {
    Json houseModesOptions = JSON_EMPTY_OBJ;
    if( json_get( device, DeviceFields::HouseModesOptionsFieldName, houseModesOptions ) ) {
        if( !houseModesOptions.empty() ) {
            LOGGER_DEBUG( "house_modes_options fetched: " + cc::j( houseModesOptions ) );
        }

        JsonArray disarmedModes;
        const char* const disarmedField = DeviceFields::HouseModesOptions::DisarmedFieldName;
        if( json_get_arr( houseModesOptions, disarmedField, disarmedModes ) && disarmedModes.is_array() ) {
            for( const auto& it : disarmedModes ) {
                auto const& modeId = it.get< std::string >();
                CoreDHouseModes::addDisarmedDevice(
                    m_hostService, m_hostService.getDevicesDbModifier(), modeId, deviceId, true );
            }
        }

        JsonArray alarmsOffModes;
        const char* const alarmsOffField = DeviceFields::HouseModesOptions::AlarmsOffFieldName;
        if( json_get_arr( houseModesOptions, alarmsOffField, alarmsOffModes ) && alarmsOffModes.is_array() ) {
            for( const auto& it : alarmsOffModes ) {
                auto const& modeId = it.get< std::string >();
                CoreDHouseModes::addAlarmsOffDevice( m_hostService.getDBClient(), modeId, deviceId );
            }
        }
    }

    return JSON( "id" << deviceId );
}

JsonObject HouseModeProcessor::handleModifyDevice( const std::string& deviceId, const JsonObject& device ) {
    HubPlatform::CoreDHouseModes::modifyDevice( m_hostService, m_hostService.getDevicesDbModifier(), deviceId, device );

    return JSON( "id" << deviceId );
}

JsonObject HouseModeProcessor::handleProtectButtonsSet( const std::string& deviceId,
                                                        CoreDHouseModes::ProtectServiceEnum service ) {
    JsonObject result = JSON_EMPTY_OBJ;
    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::NO_DEVICE_FOUND );
    }
    else {
        CoreDHouseModes::protectButtonsSet( m_hostService, deviceId, service );
    }

    return result;
}

JsonObject HouseModeProcessor::handleAddProtectDevices( const std::vector< std::string >& deviceIds ) {
    LOGGER_TRACE( "" + cc::j( deviceIds ) );
    JsonObject result = JSON_EMPTY_OBJ;
    const std::vector< std::string > actualIds = filterExistingDevicesIds( deviceIds, result );
    if( !CoreDHouseModes::protectDevicesAdd( m_hostService, actualIds ) ) {
        LOGGER_WARN( cc::warn( "Nothing to add " ) );
        setError( result, Errors::WRONG_PARAMS, "Nothing to add" );
    }
    return result;
}

JsonObject HouseModeProcessor::handleRemoveProtectDevices( const std::vector< std::string >& deviceIds ) {
    LOGGER_TRACE( "" + cc::j( deviceIds ) );
    JsonObject result = JSON_EMPTY_OBJ;
    const std::vector< std::string > actualIds = filterExistingDevicesIds( deviceIds, result );
    if( !CoreDHouseModes::protectDevicesRemove( m_hostService, actualIds ) ) {
        LOGGER_WARN( cc::warn( "Nothing to remove" ) );
        setError( result, Errors::WRONG_PARAMS, "Nothing to remove" );
    }
    return result;
}

HouseModeProcessor::CamerasOffMap HouseModeProcessor::fetchCamerasOffOptions( const std::string& deviceId ) {
    CamerasOffMap result;
    auto device = fetchDeviceById( deviceId );
    if( !device.empty() ) {
        Json camerasOff;
        using namespace Model::Resources::DeviceFields;
        const auto& fieldName = Model::Resources::DeviceFields::HouseModesOptionsFieldName;
        if( json_get( device, { fieldName, HouseModesOptions::CamerasOffFieldName }, camerasOff ) ) {
            if( camerasOff.is_object() && !camerasOff.empty() ) {
                for( auto fieldIt = camerasOff.begin(); fieldIt != camerasOff.end(); ++fieldIt ) {
                    const auto& modeId = fieldIt.key();

                    if( fieldIt.value().is_boolean() ) {
                        const bool isCameraOff = fieldIt.value().get< bool >();
                        result.emplace( modeId, isCameraOff );
                    }
                }
            }
        }
    }
    return result;
}

JsonObject HouseModeProcessor::handleAddItem( const std::string& deviceId,
                                              const std::string& itemId,
                                              const JsonObject& item,
                                              AddDeviceToHouseModesCallback addToHouseModes ) {
    JsonObject result = JSON_EMPTY_OBJ;
    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::NO_DEVICE_FOUND );
    }
    else {
        if( isCameraMakeRecordingItem( item ) ) {
            CoreDHouseModes::addCamera( m_hostService.getDBClient(), deviceId, itemId );
            // read the device.house_modes_options.camerasOff{ <modeId>: bool }
            //      and for those that have modeId: true, add to modes[modeId].camerasOffDevices[]; for false - remove.
            auto pairs = fetchCamerasOffOptions( deviceId );
            for( const auto& pair : pairs ) {
                auto modeId = pair.first;
                auto isCameraOff = pair.second;

                if( isCameraOff ) {
                    CoreDHouseModes::addCamerasOffDevice( m_hostService.getDBClient(), modeId, deviceId );
                }
                else {
                    CoreDHouseModes::removeCamerasOffDevice( m_hostService.getDBClient(), modeId, deviceId );
                }
            }
        }
        else if( isSecurityThreatItem( item ) ) {
            CoreDHouseModes::addSecurityDevice( m_hostService.getDBClient(), deviceId, itemId );

            auto device = fetchDeviceById( deviceId );
            handleAddDevice( deviceId, device );
            if( addToHouseModes ) {
                Json response = addToHouseModes( deviceId, device );
                setErrorIfHas( result, response );
            }
        }
    }

    return result;
}

JsonObject HouseModeProcessor::handleRemoveItem( const JsonObject& item ) {
    auto deviceId = json_get_str( item, ItemFields::DeviceIdFieldName );
    JsonObject result = JSON_EMPTY_OBJ;
    if( !hasDeviceById( deviceId ) ) {
        setError( result, Errors::NO_DEVICE_FOUND );
    }
    else {
        if( isCameraMakeRecordingItem( item ) ) {
            auto houseModes = CoreDHouseModes::getHouseModesList( m_hostService.getDBClient() );
            for( auto mode : houseModes ) {
                if( mode.m_camerasOffDeviceIds.find( deviceId ) != mode.m_camerasOffDeviceIds.end() ) {
                    handleRemoveCamerasOffDevice( mode.m_id, deviceId );
                }
            }

            CoreDHouseModes::removeCamera( m_hostService.getDBClient(), deviceId );
        }
        else if( isSecurityThreatItem( item ) ) {
            removeSecurityDevice( deviceId );
            result = JSON( "SecurityThreatRemoved" << true );
        }
    }

    return result;
}

bool HouseModeProcessor::isCameraMakeRecordingItem( const JsonObject& item ) {
    if( json_get_bool( item, ItemFields::HasSetterFieldName, false ) ) {
        return json_get_str( item, ItemFields::NameFieldName ) == Items::MakeRecording;
    }
    return false;
}

bool HouseModeProcessor::isSecurityThreatItem( const JsonObject& item ) {
    if( json_get_bool( item, ItemFields::HasGetterFieldName, false ) ) {
        return json_get_str( item, ItemFields::NameFieldName ) == Items::SecurityThreat;
    }
    return false;
}

std::vector< std::string > HouseModeProcessor::filterExistingDevicesIds( const std::vector< std::string >& deviceIds,
                                                                         Json& answer ) const {
    std::vector< std::string > actualIds, missingIds;
    for( const auto& id : deviceIds ) {
        auto& container = hasDeviceById( id ) ? actualIds : missingIds;
        container.push_back( id );
    }
    LOGGER_TRACE( cc::trace( "Actual ids size: " ) + cc::num10( actualIds.size() ) +
                  cc::trace( ", missing ids size: " ) + cc::num10( missingIds.size() ) );

    if( !missingIds.empty() ) {
        LOGGER_WARN( cc::warn( "Has no devices for identifiers: " ) + cc::num10( missingIds.size() ) +
                     cc::warn( ", they are: " ) + cc::j( Json( missingIds ) ) );
        setError( answer, Errors::WRONG_PARAMS, "Has no devices for identifiers: " + json_write( missingIds ) );
    }

    return actualIds;
}

JsonObject HouseModeProcessor::handleModifyItem( const std::string& deviceId,
                                                 const std::string& itemId,
                                                 const JsonObject& item,
                                                 const JsonObject& itemPrevValue ) {
    JsonObject result = JSON_EMPTY_OBJ;
    auto isCameraMakeRecording = isCameraMakeRecordingItem( item );

    LOGGER_TRACE( "check if camera device, item = " + cc::j( item ) + ", itemPrevValue = " + cc::j( itemPrevValue ) );

    IDbClient& dbClient = m_hostService.getDBClient();
    auto deviceItemPairs =
        CoreDHouseModes::getCamerasDeviceItemPairs( dbClient, JSON( db::ID_FIELD_NAME << deviceId ) );

    // check if an item is becoming a make_recording
    if( isCameraMakeRecording ) {
        if( deviceItemPairs.empty() ) {
            CoreDHouseModes::addCamera( dbClient, deviceId, itemId );
        }
        else if( deviceItemPairs[ 0 ].second != itemId ) {
            CoreDHouseModes::removeCamera( dbClient, deviceId );
            CoreDHouseModes::addCamera( dbClient, deviceId, itemId );
        }

        return result;
    }
    else if( !deviceItemPairs.empty() ) {
        // it's going to stop being a camera
        if( deviceItemPairs[ 0 ].second == itemId ) {
            auto entries = CoreDHouseModes::getHouseModesList( dbClient );
            for( auto entry : entries ) {
                if( entry.m_camerasOffDeviceIds.find( deviceId ) != entry.m_camerasOffDeviceIds.end() ) {
                    handleRemoveCamerasOffDevice( entry.m_id, deviceId );
                }
            }
            CoreDHouseModes::removeCamera( dbClient, deviceId );
        }
        return result;
    }

    std::map< std::string, std::string > itemToDeviceId;
    auto fillResultSet = [&itemToDeviceId]( const Json& pair ) {
        itemToDeviceId.emplace( json_get_str( pair, db::ID_FIELD_NAME ), json_get_str( pair, Items::SecurityThreat ) );
    };

    auto queryItemRecord = JSON( db::ID_FIELD_NAME << deviceId << Items::SecurityThreat << itemId );
    size_t hasSecurityItem = CoreDHouseModes::getSecurityDeviceItemPairs( m_dbClient, queryItemRecord, fillResultSet );
    bool isSecurityItem = isSecurityThreatItem( item );

    if( !hasSecurityItem && isSecurityItem ) {
        // became SecurityThreat -> add device to house modes
        CoreDHouseModes::addSecurityDevice( m_dbClient, deviceId, itemId );

        auto device = fetchDeviceById( deviceId );
        this->handleAddDevice( deviceId, device );
    }
    else if( hasSecurityItem && !isSecurityItem ) {
        // became non-SecurityThreat -> remove device from house modes
        removeSecurityDevice( deviceId );
        result = JSON( "SetDeviceArmedFalse" << true );
    }

    return result;
}

void HouseModeProcessor::removeSecurityDevice( const std::string& deviceId ) {
    auto houseModes = CoreDHouseModes::getHouseModesList( m_dbClient );
    for( auto mode : houseModes ) {
        if( mode.m_disarmedDeviceIds.find( deviceId ) != mode.m_disarmedDeviceIds.end() ) {
            const bool changeArmedState = false;
            handleRemoveDisarmedDevice( mode.m_id, deviceId, changeArmedState );
        }
        if( mode.m_bypassDeviceIds.find( deviceId ) != mode.m_bypassDeviceIds.end() ) {
            CoreDHouseModes::removeBypassDevices( m_hostService, mode.m_id, { deviceId } );
        }
    }

    CoreDHouseModes::removeSecurityDevice( m_dbClient, deviceId );
}

bool HouseModeProcessor::isBypassAllowedForDevice( const std::string& id ) const {
    auto device = fetchDeviceById( id );
    auto securityConfigId = json_get_str( device, DeviceFields::ProtectConfig );
    if( !securityConfigId.empty() && securityConfigId != "default" ) {
        Protect::ProtectSecurityConfigInterface securityConfigInterface( m_hostService );
        if( !securityConfigInterface.isBypassSet( securityConfigId ) ) {
            LOGGER_INFO( cc::info( "Device: " ) + cc::note( id ) + cc::info( " is not allowed to bypass" ) );
            return false;
        }
    }
    return true;
}

std::string HouseModeProcessor::houseModeIdToArmedWhenField( const std::string& houseModeId ) {
    try {
        auto modeId = std::stoi( houseModeId );
        switch( modeId ) {
        case 1:
            return "Disarm";
        case 2:
            return "Away";
        case 3:
            return "Stay";
        case 4:
            return "Away";
        default:
            return "Disarm";
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to convert houseMode id to integer " ) + cc::warn( ex.what() ) );
        return "Disarm";
    }
}

}  // namespace CoreD
}  // namespace HubPlatform
