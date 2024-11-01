#include "cored/house_modes/house_mode_modify_device.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/model/resources/cored_resources.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "log_utils/ezlo_log.h"
#include <utility>

namespace HubPlatform {
namespace CoreDHouseModes {

using namespace HubPlatform::CoreD::Model::Resources;
using namespace HubPlatform::CoreDHouseModes::Resources;

std::map< std::string, DeviceEntry > DeviceHouseModeSettingsModifier::buildHouseModesEntries() {
    std::map< std::string, DeviceEntry > houseModeEntries;
    std::vector< CoreDHouseModes::HouseModeEntry > houseModesList =
        CoreDHouseModes::getHouseModesList( m_hostService.getDBClient() );

    for( const auto& houseMode : houseModesList ) {
        DeviceEntry entry{ houseMode.m_id };
        entry.m_disarmedDefault = houseMode.m_disarmedDefault;
        entry.m_alarmsOffDeviceIds = houseMode.m_alarmsOffDeviceIds;
        entry.m_disarmedDeviceIds = houseMode.m_disarmedDeviceIds;
        entry.m_camerasOffDeviceIds = houseMode.m_camerasOffDeviceIds;

        if( houseMode.m_disarmedDefault ) {
            {
                auto& collection = houseMode.m_alarmsOffDeviceIds;
                entry.m_alarmOff = collection.find( m_deviceId ) != collection.end();
            }
            {
                auto& collection = houseMode.m_disarmedDeviceIds;
                entry.m_disarmed = collection.find( m_deviceId ) != collection.end();
            }
            {
                auto& collection = houseMode.m_camerasOffDeviceIds;
                entry.m_cameraOff = collection.find( m_deviceId ) != collection.end();
            }
        }
        houseModeEntries.emplace( houseMode.m_id, entry );
        LOGGER_TRACE( "    houseModes entry: " + cc::j( entry.toJson() ) );
    }
    return houseModeEntries;
}

std::map< std::string, DeviceEntry > DeviceHouseModeSettingsModifier::buildDeviceHouseModesOptions() {
    std::map< std::string, DeviceEntry > deviceModeOptions;
    Json houseModesOptions;
    JsonArray modesAlarmsOff;
    JsonArray modesDisarmed;
    JsonObject modesCamerasOff;

    bool hasDisarmedOptions = false;

    if( json_get_obj( m_device, DeviceFields::HouseModesOptionsFieldName, houseModesOptions ) ) {
        if( json_get_arr( houseModesOptions, DeviceFields::HouseModesOptions::AlarmsOffFieldName, modesAlarmsOff ) ) {
            for( const auto& modeId : modesAlarmsOff ) {
                auto modeIdString = modeId.get< std::string >();
                if( !modeIdString.empty() ) {
                    deviceModeOptions[ modeId ].m_modeId = modeIdString;
                    deviceModeOptions[ modeId ].m_alarmOff = true;
                    LOGGER_TRACE( "    alarmOff device found: modeId=" + modeIdString );
                }
                else {
                    LOGGER_ERROR( "Ill-formed collection 'alarmsOff': item should be a string" );
                }
            }
        }

        if( json_get_arr( houseModesOptions, DeviceFields::HouseModesOptions::DisarmedFieldName, modesDisarmed ) ) {
            hasDisarmedOptions = true;
            for( const auto& modeId : modesDisarmed ) {
                auto modeIdString = modeId.get< std::string >();
                if( !modeIdString.empty() ) {
                    deviceModeOptions[ modeId ].m_modeId = modeIdString;
                    deviceModeOptions[ modeId ].m_disarmed = true;
                    LOGGER_TRACE( "    disarmed device found: modeId=" + modeIdString );
                }
                else {
                    LOGGER_ERROR( "Ill-formed collection 'disarmed': item should be a string" );
                }
            }
        }

        if( json_get_obj( houseModesOptions, DeviceFields::HouseModesOptions::CamerasOffFieldName, modesCamerasOff ) ) {
            for( auto it = modesCamerasOff.begin(); it != modesCamerasOff.end(); it++ ) {
                auto modeIdString = it.key();
                bool cameraOff = json_get_bool( modesCamerasOff, modeIdString, false );

                deviceModeOptions[ modeIdString ].m_modeId = modeIdString;
                deviceModeOptions[ modeIdString ].m_cameraOff = cameraOff;
                LOGGER_TRACE( "    cameraOff device found: modeId=" + modeIdString );
            }
        }
    }

    if( !hasDisarmedOptions ) {
        auto allModes = getHouseModesList( getDBClient() );
        auto category = json_get_str( m_device, "category" );
        auto subcategory = json_get_str( m_device, "subcategory" );
        auto affected = checkCategoryAffectedByHouseMode( getDBClient(), category, subcategory );

        for( const auto& mode : allModes ) {
            const std::string& modeId = mode.m_id;
            auto foundIt = affected.find( modeId );
            bool toBeArmed = foundIt != affected.end() && foundIt->second;

            deviceModeOptions[ modeId ].m_modeId = modeId;
            deviceModeOptions[ modeId ].m_disarmed = !toBeArmed;
        }
    }

    auto category = json_get_str( m_device, DeviceFields::CategoryFieldName );
    auto subCategory = json_get_str( m_device, DeviceFields::SubcategoryFieldName );

    {
        const auto statusDeviceByMode =
            CoreDHouseModes::checkCategoryAffectedByHouseMode( m_hostService.getDBClient(), category, subCategory );

        const bool hasDeviceHouseModesDisarmed =
            modesDisarmed.is_array() || ( modesDisarmed.is_object() && modesDisarmed.empty() );
        for( const auto& status : statusDeviceByMode ) {
            const auto& modeId = status.first;
            const bool shouldBeArmed = status.second;

            auto& entry = deviceModeOptions[ modeId ];
            if( entry.m_modeId.empty() )
                entry.m_modeId = modeId;

            if( !hasDeviceHouseModesDisarmed ) {
                entry.m_disarmed |= !shouldBeArmed;
            }
        }
    }

    {
        const auto statusDeviceByMode = CoreDHouseModes::checkAlarmCategoryAffectedByHouseMode(
            m_hostService.getDBClient(), category, subCategory );

        const bool hasDeviceHouseModesAlarmsOff =
            modesAlarmsOff.is_array() || ( modesAlarmsOff.is_object() && modesAlarmsOff.empty() );
        for( const auto& status : statusDeviceByMode ) {
            const auto& modeId = status.first;
            const bool isAlarm = status.second;

            auto& entry = deviceModeOptions[ modeId ];
            if( entry.m_modeId.empty() )
                entry.m_modeId = modeId;

            if( !hasDeviceHouseModesAlarmsOff ) {
                entry.m_alarmOff |= !isAlarm;
            }
        }
    }
    return deviceModeOptions;
}

bool DeviceHouseModeSettingsModifier::modifyDevice() {
    auto& dbClient = getDBClient();

    std::map< std::string, DeviceEntry > houseModeEntries = buildHouseModesEntries();
    std::map< std::string, DeviceEntry > deviceModeOptions = buildDeviceHouseModesOptions();

    // main merging loop, all data should be already set up
    const bool changeArmedState = false;

    for( const auto& entryIt : houseModeEntries ) {
        const auto& modeId = entryIt.first;
        const DeviceEntry& alreadySet = entryIt.second;

        LOGGER_TRACE( "  house modes alreadySet: " + cc::j( alreadySet.toJson() ) );
        if( alreadySet.m_disarmedDefault == false ) {
            LOGGER_TRACE( "    skip, disarmedDefault=" + cc::c( alreadySet.m_disarmedDefault ) );
            continue;
        }

        auto deviceIt = deviceModeOptions.find( modeId );
        if( deviceIt != deviceModeOptions.end() ) {
            const DeviceEntry& deviceModeEntry = deviceIt->second;
            LOGGER_TRACE( "  has device options for that mode: " + cc::j( deviceModeEntry.toJson() ) );
            if( deviceModeEntry.m_alarmOff != alreadySet.m_alarmOff ) {
                if( deviceModeEntry.m_alarmOff ) {
                    addAlarmOffDevice( modeId );
                }
                else {
                    removeAlarmOffDevice( modeId );
                }
            }
            else {
                LOGGER_TRACE( "      equal alarmOff flags" );
            }

            if( deviceModeEntry.m_disarmed != alreadySet.m_disarmed ) {
                if( deviceModeEntry.m_disarmed ) {
                    addDisarmedDevice( modeId, changeArmedState );
                }
                else {
                    removeDisarmedDevice( modeId, changeArmedState );
                }
            }
            else {
                LOGGER_TRACE( "      equal disarmed flags" );
            }

            if( deviceModeEntry.m_cameraOff != alreadySet.m_cameraOff ) {
                if( deviceModeEntry.m_cameraOff ) {
                    addCameraOffDevice( modeId );
                }
                else {
                    removeCameraOffDevice( modeId );
                }
            }
            else {
                LOGGER_TRACE( "      equal cameraOff flags" );
            }
        }
        else {
            LOGGER_TRACE(
                std::string{ "remove all: modeId=" }.append( modeId ).append( ", deviceId=" ).append( m_deviceId ) );
            if( !alreadySet.m_alarmOff ) {
                if( alreadySet.m_alarmsOffDeviceIds.find( m_deviceId ) != alreadySet.m_alarmsOffDeviceIds.end() ) {
                    removeAlarmOffDevice( modeId );
                }
            }

            if( !alreadySet.m_cameraOff ) {
                if( alreadySet.m_camerasOffDeviceIds.find( m_deviceId ) != alreadySet.m_camerasOffDeviceIds.end() ) {
                    removeCameraOffDevice( modeId );
                }
            }

            if( !alreadySet.m_disarmed ) {
                if( alreadySet.m_disarmedDeviceIds.find( m_deviceId ) != alreadySet.m_disarmedDeviceIds.end() ) {
                    removeDisarmedDevice( modeId, changeArmedState );
                }
            }
        }
    }
    return m_isModified;
}

bool DeviceHouseModeSettingsModifier::removeDisarmedDevice( const std::string& modeId, const bool changeArmedState ) {
    LOGGER_TRACE( std::string{ "modeId=" }.append( modeId ).append( ", m_deviceId=" ).append( m_deviceId ) );
    bool done =
        CoreDHouseModes::removeDisarmedDevice( m_hostService, m_dbModifier, modeId, m_deviceId, changeArmedState );
    if( done ) {
        m_hostService.sendBroadcast( HOUSE_MODE_DISARMED_DEVICES_REMOVED,
                                     JSON( BroadcastFields::ModeIdFieldName
                                           << modeId << BroadcastFields::DisarmedDeviceFieldName << m_deviceId ) );
        m_isModified = true;
    }
    return done;
}

bool DeviceHouseModeSettingsModifier::addDisarmedDevice( const std::string& modeId, bool changeArmedState ) {
    LOGGER_TRACE( std::string{ "modeId=" }.append( modeId ).append( ", m_deviceId=" ).append( m_deviceId ) );

    const bool pretendDeviceIsNew = true;
    bool done = CoreDHouseModes::addDisarmedDevice(
        m_hostService, m_dbModifier, modeId, m_deviceId, pretendDeviceIsNew, changeArmedState );
    if( done ) {
        m_hostService.sendBroadcast( HOUSE_MODE_DISARMED_DEVICES_ADDED,
                                     JSON( BroadcastFields::ModeIdFieldName
                                           << modeId << BroadcastFields::DisarmedDeviceFieldName << m_deviceId ) );
        m_isModified = true;
    }
    return done;
}

bool DeviceHouseModeSettingsModifier::removeAlarmOffDevice( const std::string& modeId ) {
    LOGGER_TRACE( std::string{ "modeId=" }.append( modeId ).append( ", m_deviceId=" ).append( m_deviceId ) );
    bool done = CoreDHouseModes::removeAlarmsOffDevice( getDBClient(), modeId, m_deviceId );
    if( done ) {
        m_hostService.sendBroadcast( HOUSE_MODE_ALARMS_OFF_REMOVED,
                                     JSON( BroadcastFields::ModeIdFieldName
                                           << modeId << BroadcastFields::AlarmsOffDeviceFieldName << m_deviceId ) );
        m_isModified = true;
    }
    return done;
}

bool DeviceHouseModeSettingsModifier::addAlarmOffDevice( const std::string& modeId ) {
    bool done = CoreDHouseModes::addAlarmsOffDevice( getDBClient(), modeId, m_deviceId );
    LOGGER_TRACE( std::string{ "modeId=" }.append( modeId ).append( ", m_deviceId=" ).append( m_deviceId ) );
    if( done ) {
        m_hostService.sendBroadcast( HOUSE_MODE_ALARMS_OFF_ADDED,
                                     JSON( BroadcastFields::ModeIdFieldName
                                           << modeId << BroadcastFields::AlarmsOffDeviceFieldName << m_deviceId ) );
        m_isModified = true;
    }
    return done;
}

bool DeviceHouseModeSettingsModifier::addCameraOffDevice( const std::string& modeId ) {
    bool done = CoreDHouseModes::addCamerasOffDevice( getDBClient(), modeId, m_deviceId );
    LOGGER_TRACE( std::string{ "modeId=" }.append( modeId ).append( ", m_deviceId=" ).append( m_deviceId ) );
    if( done ) {
        m_hostService.sendBroadcast( HOUSE_MODE_CAMERAS_OFF_ADDED,
                                     JSON( BroadcastFields::ModeIdFieldName
                                           << modeId << BroadcastFields::CamerasOffDeviceFieldName << m_deviceId ) );
        m_isModified = true;
    }
    return done;
}

bool DeviceHouseModeSettingsModifier::removeCameraOffDevice( const std::string& modeId ) {
    LOGGER_TRACE( std::string{ "modeId=" }.append( modeId ).append( ", m_deviceId=" ).append( m_deviceId ) );
    bool done = CoreDHouseModes::removeCamerasOffDevice( getDBClient(), modeId, m_deviceId );
    if( done ) {
        m_hostService.sendBroadcast( HOUSE_MODE_CAMERAS_OFF_REMOVED,
                                     JSON( BroadcastFields::ModeIdFieldName
                                           << modeId << BroadcastFields::CamerasOffDeviceFieldName << m_deviceId ) );
        m_isModified = true;
    }
    return done;
}

IDbClient& DeviceHouseModeSettingsModifier::getDBClient() {
    return m_hostService.getDBClient();
}

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
