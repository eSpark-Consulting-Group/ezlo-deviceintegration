#include <utility>
#include <vector>
#include <set>
#include <fstream>

#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/house_mode_entry.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "cored/house_modes/protect_service_enum.h"
#include "cored/house_modes/house_mode_modify_device.h"
#include "cored/house_modes/script_events/sos_button_events.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/idb_modifier.h"
#include "cored/group_operations/resources.h"
#include "cored/group_operations/device_group_find_handler.h"
#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "service_interfaces/service.h"

#include "database_client/idbclient.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"
#include "jsonrpc/jsonrpc.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"

namespace HubPlatform {
namespace CoreDHouseModes {

using namespace Resources;

static std::set< std::string > fromJsonArray( const JsonArray& arr ) {
    std::set< std::string > res;
    for( const std::string& element : arr )
        res.insert( element );

    return res;
}

static std::vector< HouseModeEntry > getDefaultModeList( const ConfigReader& configReader ) {
    std::vector< HouseModeEntry > result;
    ConfigReader::Path path = ConfigReader::Path( configReader, ConfigKeys::HouseModes );
    std::size_t size = configReader.getArraySize( path );
    for( std::size_t i = 0; i < size; ++i ) {
        try {
            ConfigReader::Path idPath = path / i / "id";
            const std::string id = std::to_string( configReader.getInt( idPath ) );
            ConfigReader::Path namePath = path / i / "name";
            const std::string name = configReader.getString( namePath );
            ConfigReader::Path descriptionPath = path / i / "description";
            const std::string description = configReader.getString( descriptionPath );
            ConfigReader::Path disarmedDefaultPath = path / i / "disarmedDefault";
            bool disarmedDefault = configReader.getBool( disarmedDefaultPath );
            ConfigReader::Path ezloProtectPath = path / i / "protect";
            bool ezloProtect = configReader.getBool( ezloProtectPath );
            ConfigReader::Path armedPath = path / i / "armed";
            bool armed = configReader.getBool( armedPath );
            ConfigReader::Path armedCategoriesPath = path / i / "armedCategories";
            int armedCategoriesSize = configReader.getArraySize( armedCategoriesPath );
            HouseModeEntry::Categories armedCategories, alarmCategories;
            ConfigReader::Path ignoreDelayPath = path / i / "ignoreDelay";
            bool ignoreDelay = false;
            if( configReader.hasValue( ignoreDelayPath ) ) {
                ignoreDelay = configReader.getBool( ignoreDelayPath );
            }

            for( std::size_t j = 0; j < armedCategoriesSize; ++j ) {
                ConfigReader::Path armedCategoryNamePath = armedCategoriesPath / j / "category";
                const std::string category = configReader.getString( armedCategoryNamePath );
                auto& categorySet = armedCategories[ category ];

                int armedSubcategoriesSize;
                ConfigReader::Path armedSubcategoriesPath = armedCategoriesPath;
                try {
                    armedSubcategoriesPath = armedSubcategoriesPath / j / "subcategories";
                    armedSubcategoriesSize = configReader.getArraySize( armedSubcategoriesPath );
                }
                catch( const ConfigReader::ReadingError& ex ) {
                    armedSubcategoriesSize = 0;
                }

                for( std::size_t k = 0; k < armedSubcategoriesSize; ++k ) {
                    ConfigReader::Path subcategoryPath = armedSubcategoriesPath / k;
                    const std::string subcategory = configReader.getString( subcategoryPath );
                    categorySet.insert( subcategory );
                }
            }
            ConfigReader::Path alarmCategoriesPath = path / i / "alarmCategories";
            int alarmCategoriesSize = configReader.getArraySize( alarmCategoriesPath );

            for( std::size_t j = 0; j < alarmCategoriesSize; ++j ) {
                ConfigReader::Path alarmCategoryNamePath = alarmCategoriesPath / j / "category";
                const std::string category = configReader.getString( alarmCategoryNamePath );
                auto& categorySet = alarmCategories[ category ];

                int alarmSubcategoriesSize;
                ConfigReader::Path alarmSubcategoriesPath = alarmCategoriesPath;
                try {
                    alarmSubcategoriesPath = alarmSubcategoriesPath / j / "subcategories";
                    alarmSubcategoriesSize = configReader.getArraySize( alarmSubcategoriesPath );
                }
                catch( const ConfigReader::ReadingError& ex ) {
                    alarmSubcategoriesSize = 0;
                }

                for( std::size_t k = 0; k < alarmSubcategoriesSize; ++k ) {
                    ConfigReader::Path subcategoryPath = alarmSubcategoriesPath / k;
                    const std::string subcategory = configReader.getString( subcategoryPath );
                    categorySet.insert( subcategory );
                }
            }

            HouseModeEntry::Notifications notificationsDefault;
            try {
                ConfigReader::Path notificationsPath = path / i / "notificationsDefault";
                notificationsDefault = std::make_unique< std::set< std::string > >();
                const std::size_t arraySize = configReader.getArraySize( notificationsPath );
                for( std::size_t j = 0; j < arraySize; ++j ) {
                    notificationsDefault->insert( configReader.getString( notificationsPath / j ) );
                }
            }
            catch( const ConfigReader::ReadingError& ex ) {
                LOGGER_DEBUG( "Notification reading error: " + ex.what() );
            }

            result.emplace_back( id,
                                 name,
                                 description,
                                 disarmedDefault,
                                 ignoreDelay,
                                 ezloProtect,
                                 armed,
                                 nullptr,
                                 std::move( notificationsDefault ),
                                 std::move( armedCategories ),
                                 std::move( alarmCategories ) );
        }
        catch( const std::exception& ex ) {
            Log::error( "getDefaultModeList, exception at(" + std::to_string( i ) +
                        "),msg :" + std::string( ex.what() ) );
        }
    }
    return result;
}

void ensureHouseModesInDb( IDbClient& dbClient, const ConfigReader& configReader ) {
    auto houseModesInDb = dbClient.findAll( DBNames::HouseModesDB, JSON_EMPTY_OBJ, nullptr );
    if( houseModesInDb.empty() ) {
        auto defaultHouseModes = getDefaultModeList( configReader );
        for( auto& houseMode : defaultHouseModes )
            dbClient.insert( DBNames::HouseModesDB, houseMode.toJsonDb() );
    }
}

std::vector< HouseModeEntry > getHouseModesList( const IDbClient& dbClient ) {
    std::vector< HouseModeEntry > result;
    const auto fields =
        JSON( HouseModeFields::IdFieldName
              << 1 << HouseModeFields::ModeNameFieldName << 1 << HouseModeFields::DescriptionFieldName << 1
              << HouseModeFields::SwitchToDelayFieldName << 1 << HouseModeFields::DisarmedDevicesFieldName << 1
              << HouseModeFields::DisarmedDefaultFieldName << 1 << HouseModeFields::ArmedCategoriesFieldName << 1
              << HouseModeFields::NotificationsFieldName << 1 << HouseModeFields::AlarmsOffDevicesFieldName << 1
              << HouseModeFields::AlarmCategoriesFieldName << 1 << HouseModeFields::IgnoreSwitchDelay << 1
              << HouseModeFields::NotificationsDefaultFieldName << 1 << HouseModeFields::ProtectFieldName << 1
              << HouseModeFields::CamerasOffDevicesFieldName << 1 << HouseModeFields::BypassDevicesFieldName << 1
              << HouseModeFields::ArmedFieldName << 1 );
    auto houseModesJson = dbClient.findAll( DBNames::HouseModesDB, JSON_EMPTY_OBJ, &fields );

    for( const auto& houseMode : houseModesJson ) {
        result.emplace_back( houseMode );
    }

    return result;
}

std::unique_ptr< HouseModeEntry > getHouseModeImpl( const IDbClient& dbClient,
                                                    const std::string& modeId,
                                                    const std::string& modeName,
                                                    bool absentModeIsError ) {
    const auto fields =
        JSON( HouseModeFields::IdFieldName
              << 1 << HouseModeFields::ModeNameFieldName << 1 << HouseModeFields::DescriptionFieldName << 1
              << HouseModeFields::SwitchToDelayFieldName << 1 << HouseModeFields::DisarmedDevicesFieldName << 1
              << HouseModeFields::DisarmedDefaultFieldName << 1 << HouseModeFields::ArmedCategoriesFieldName << 1
              << HouseModeFields::NotificationsFieldName << 1 << HouseModeFields::AlarmsOffDevicesFieldName << 1
              << HouseModeFields::AlarmCategoriesFieldName << 1 << HouseModeFields::IgnoreSwitchDelay << 1
              << HouseModeFields::NotificationsDefaultFieldName << 1 << HouseModeFields::ProtectFieldName << 1
              << HouseModeFields::CamerasOffDevicesFieldName << 1 << HouseModeFields::BypassDevicesFieldName << 1
              << HouseModeFields::ArmedFieldName << 1 );

    Json houseModeJson;
    if( !modeId.empty() ) {
        houseModeJson =
            dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << modeId ), &fields );
    }
    else if( !modeName.empty() ) {
        houseModeJson =
            dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::ModeNameFieldName << modeName ), &fields );
    }

    if( absentModeIsError && houseModeJson.empty() ) {
        return nullptr;
    }

    try {
        auto houseMode = std::make_unique< HouseModeEntry >( houseModeJson );
        return houseMode;
    }
    catch( const std::exception& ex ) {
        Log::error( "getHouseMode, exception: " + std::string( ex.what() ) );
        // TODO: this nullptr is almost never checked...
        return nullptr;
    }
}

std::unique_ptr< HouseModeEntry > getHouseMode( const IDbClient& dbClient,
                                                const std::string& modeId,
                                                bool absentModeIsError ) {
    return getHouseModeImpl( dbClient, modeId, {}, absentModeIsError );
}

std::unique_ptr< HouseModeEntry > getHouseModeByName( const IDbClient& dbClient,
                                                      const std::string& modeName,
                                                      bool absentModeIsError ) {
    return getHouseModeImpl( dbClient, {}, modeName, absentModeIsError );
}

std::unique_ptr< HouseModeEntry > getCurrentHouseMode( const IDbClient& dbClient ) {
    return getHouseMode( dbClient, getCurrentHouseModeId( dbClient ) );
}

DeviceIDs getArmedDeviceIDsAffectedByHouseModes( const IDbClient& dbClient ) {
    // replace category/subcategory search by actual devices
    DeviceIDs deviceIds;
    CoreDHouseModes::getSecurityDeviceItemPairs( dbClient, JSON_EMPTY_OBJ, [&]( const Json& pair ) {
        deviceIds.push_back( json_get_str( pair, db::ID_FIELD_NAME ) );
    } );
    return deviceIds;
}

DeviceIDs getDefaultDisarmedDeviceIDs( const IDbClient& dbClient, const std::string& modeId ) {
    DeviceIDs deviceDisarmedIds;
    DeviceIDs deviceIds = getArmedDeviceIDsAffectedByHouseModes( dbClient );
    auto houseMode = getHouseMode( dbClient, modeId );

    for( const auto& deviceId : deviceIds ) {
        const auto device = dbClient.findOne(
            HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );

        namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
        bool isHouseModeDisarmedDevice = false;
        std::string category, subcategory;
        category = json_get_str( device, Fields::CategoryFieldName );  // what constant use ?
        if( !json_get( device, Fields::SubcategoryFieldName, subcategory ) )
            subcategory = "";

        auto it = houseMode->m_armedCategories.find( category );
        if( it == houseMode->m_armedCategories.end() )
            isHouseModeDisarmedDevice = true;

        if( !isHouseModeDisarmedDevice && !subcategory.empty() ) {
            const auto& subcategories = it->second;
            auto itSet = subcategories.find( subcategory );
            if( itSet == subcategories.end() )
                isHouseModeDisarmedDevice = true;
        }

        if( !isHouseModeDisarmedDevice ) {
            auto houseModesIt = device.find( Fields::HouseModesOptionsFieldName );
            auto endIt = houseModesIt->end();
            auto disarmedIt = houseModesIt != device.end() ?
                                  houseModesIt->find( Fields::HouseModesOptions::DisarmedFieldName ) :
                                  endIt;

            if( disarmedIt != endIt ) {
                for( const auto& mode : *disarmedIt ) {
                    if( mode.is_string() && mode.get< std::string >() == modeId ) {
                        isHouseModeDisarmedDevice = true;
                        break;
                    }
                }
            }
        }

        if( isHouseModeDisarmedDevice )
            deviceDisarmedIds.push_back( deviceId );
    }
    return deviceDisarmedIds;
}

DeviceIDs getAlarmDeviceIDsAffectedByHouseModes( const IDbClient& dbClient ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    DeviceIDs deviceIds;
    const auto deviceFields = JSON( db::ID_FIELD_NAME << 1 << DeviceFields::CategoryFieldName << 1
                                                      << DeviceFields::SubcategoryFieldName << 1 );
    const auto devices =
        dbClient.findAll( HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB, JSON_EMPTY_OBJ, &deviceFields );

    auto houseModes = getHouseModesList( dbClient );

    for( const auto& device : devices ) {
        std::string deviceId, category, subcategory;
        deviceId = json_get_str( device, db::ID_FIELD_NAME );
        category = json_get_str( device, DeviceFields::CategoryFieldName );  // what constant use ?
        if( !json_get( device, DeviceFields::SubcategoryFieldName, subcategory ) )
            subcategory = "";

        bool isHouseModeAlarmDevice = false;
        for( const auto& houseMode : houseModes ) {
            auto it = houseMode.m_alarmCategories.find( category );
            if( it == houseMode.m_alarmCategories.end() )
                continue;

            if( !subcategory.empty() ) {
                const auto& subcategories = it->second;
                auto itSet = subcategories.find( subcategory );
                if( itSet == subcategories.end() )
                    continue;
            }

            isHouseModeAlarmDevice = true;
        }
        if( isHouseModeAlarmDevice )
            deviceIds.push_back( deviceId );
    }
    return deviceIds;
}

size_t getCamerasDeviceItemPairs( const IDbClient& dbClient,
                                  const Json& query,
                                  const ProcessDbResultsetItem& callback ) {
    //
    size_t counter = 0;
    auto fields = JSON( db::ID_FIELD_NAME << 1 << Items::MakeRecording << 1 );
    auto records = dbClient.findAll( DBNames::HouseModesCamerasDB, query, &fields );
    for( const auto& record : records ) {
        callback( record );
        counter++;
    }
    return counter;
}

DeviceItemIDs getCamerasDeviceItemPairs( const IDbClient& dbClient, const Json& query ) {
    DeviceItemIDs result;
    getCamerasDeviceItemPairs( dbClient, query, [&result]( const Json& deviceItemPair ) {
        result.push_back( { json_get_str( deviceItemPair, db::ID_FIELD_NAME ),
                            json_get_str( deviceItemPair, Items::MakeRecording ) } );
    } );
    return result;
}

DeviceIDs getDefaultAlarmOffDeviceIDs( const IDbClient& dbClient, const std::string& modeId ) {
    DeviceIDs deviceAlarmOffIds;
    DeviceIDs deviceIds = getAlarmDeviceIDsAffectedByHouseModes( dbClient );
    auto houseMode = getHouseMode( dbClient, modeId );
    using namespace CoreD::Model::Resources;

    for( const auto& deviceId : deviceIds ) {
        const auto device = dbClient.findOne(
            HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );

        bool isHouseModeAlarmOffDevice = false;
        std::string category, subcategory;
        category = json_get_str( device, DeviceFields::CategoryFieldName );
        if( !json_get( device, DeviceFields::SubcategoryFieldName, subcategory ) )
            subcategory = "";

        auto it = houseMode->m_alarmCategories.find( category );
        if( it == houseMode->m_alarmCategories.end() )
            isHouseModeAlarmOffDevice = true;

        if( !isHouseModeAlarmOffDevice && !subcategory.empty() ) {
            const auto& subcategories = it->second;
            auto itSet = subcategories.find( subcategory );
            if( itSet == subcategories.end() )
                isHouseModeAlarmOffDevice = true;
        }

        namespace Fields = CoreD::Model::Resources::DeviceFields;
        if( !isHouseModeAlarmOffDevice ) {
            auto houseModesIt = device.find( Fields::HouseModesOptionsFieldName );
            auto endIt = houseModesIt->end();
            auto disarmedIt = houseModesIt != device.end() ?
                                  houseModesIt->find( Fields::HouseModesOptions::AlarmsOffFieldName ) :
                                  endIt;

            if( disarmedIt != endIt ) {
                for( const auto& mode : *disarmedIt ) {
                    if( mode.is_string() && mode.get< std::string >() == modeId ) {
                        isHouseModeAlarmOffDevice = true;
                        break;
                    }
                }
            }
        }

        if( isHouseModeAlarmOffDevice )
            deviceAlarmOffIds.push_back( deviceId );
    }
    return deviceAlarmOffIds;
}

DeviceIDs getDefaultCamerasOffDeviceIDs( const IDbClient& dbClient, const std::string& modeId ) {
    DeviceIDs deviceCamerasOffIds;
    DeviceIDs deviceIds;
    getCamerasDeviceItemPairs( dbClient, JSON_EMPTY_OBJ, [&deviceIds]( const Json& pair ) {
        deviceIds.push_back( json_get_str( pair, db::ID_FIELD_NAME ) );
    } );

    auto houseMode = getHouseMode( dbClient, modeId );
    using namespace CoreD::Model::Resources;

    for( const auto& deviceId : deviceIds ) {
        const auto device = dbClient.findOne(
            HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB, JSON( db::ID_FIELD_NAME << deviceId ), nullptr );

        bool isHouseModeCamerasOffDevice = false;

        namespace Fields = CoreD::Model::Resources::DeviceFields;
        Json camerasOffFlag;
        if( json_get( device,
                      { Fields::HouseModesOptionsFieldName, Fields::HouseModesOptions::CamerasOffFieldName, modeId },
                      camerasOffFlag ) ) {
            isHouseModeCamerasOffDevice = camerasOffFlag.is_boolean() && camerasOffFlag.get< bool >();
        }

        if( isHouseModeCamerasOffDevice )
            deviceCamerasOffIds.push_back( deviceId );
    }

    return deviceCamerasOffIds;
}

/** For all house modes it checks whether the pair (category, subcategory) should be marked as an alarm.
 * @return map of (modeId: string,  alarm: bool) pairs */
StatusDeviceByMode checkAlarmCategoryAffectedByHouseMode( const IDbClient& dbClient,
                                                          const std::string& category,
                                                          const std::string& subcategory ) {
    StatusDeviceByMode status;

    auto houseModes = getHouseModesList( dbClient );
    for( const auto& houseMode : houseModes ) {
        const auto& categories = houseMode.m_alarmCategories;
        auto itArmed = categories.find( category );
        if( itArmed == categories.end() )
            continue;

        if( !subcategory.empty() ) {
            const auto& subcategories = itArmed->second;
            auto it = subcategories.find( subcategory );
            if( it == subcategories.end() )
                continue;
        }

        status[ houseMode.m_id ] = true;
    }
    return status;
}

/** For all house modes it checks whether the pair (category, subcategory) should be marked as armed.
 * @return map of (modeId: string,  armed: bool) pairs for only those modeIds where armed is true. */
StatusDeviceByMode checkCategoryAffectedByHouseMode( const IDbClient& dbClient,
                                                     const std::string& category,
                                                     const std::string& subcategory ) {
    StatusDeviceByMode status;

    auto houseModes = getHouseModesList( dbClient );
    for( const auto& houseMode : houseModes ) {
        const auto& categories = houseMode.m_armedCategories;
        auto itArmed = categories.find( category );
        if( itArmed == categories.end() )
            continue;

        if( !subcategory.empty() ) {
            const auto& subcategories = itArmed->second;
            auto it = subcategories.find( subcategory );
            if( it == subcategories.end() )
                continue;
        }

        status[ houseMode.m_id ] = true;
    }
    return status;
}

std::string getCurrentHouseModeId( const IDbClient& dbClient ) {
    const auto field = JSON( HOUSE_MODE_ENTRY << 1 );
    auto json = dbClient.findOne( HUBSettings, JSON( HouseModeFields::IdFieldName << HOUSE_MODE_ENTRY ), &field );
    return json_get_str( json, HOUSE_MODE_ENTRY );
}

std::string getHouseModeSwitchTo( const IDbClient& dbClient ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBTempSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    if( houseModeSettings.empty() ) {
        return std::string();
    }

    return json_get_str( houseModeSettings, CoreDHouseModes::HOUSE_MODE_SWITCH_ENTRY );
}

void setSwitchDelay( IDbClient& dbClient, int delay ) {
    dbClient.update( HUBSettings,
                     JSON( "_id" << CoreDHouseModes::HOUSE_MODE_ENTRY ),
                     JSON( "$set" << JSON( CoreDHouseModes::HOUSE_MODE_SWITCH_DELAY << delay ) ) );
}

void setAlarmDelay( IDbClient& dbClient, int delay ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    Json entryDelay;
    json_get( houseModeSettings, CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY, entryDelay );
    entryDelay[ CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_NORMAL ] = delay;
    dbClient.update( HUBSettings,
                     JSON( "_id" << CoreDHouseModes::HOUSE_MODE_ENTRY ),
                     JSON( "$set" << JSON( CoreDHouseModes::HOUSE_MODE_ALARM_DELAY
                                           << delay << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY << entryDelay ) ) );
}

int getSwitchDelay( IDbClient& dbClient ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    if( houseModeSettings.empty() ) {
        return -1;
    }

    return json_get_int( houseModeSettings, CoreDHouseModes::HOUSE_MODE_SWITCH_DELAY, -1 );
}

int getSwitchDelayLeft( IDbClient& dbClient ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBTempSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    if( houseModeSettings.empty() ) {
        return -1;
    }

    return json_get_int( houseModeSettings, CoreDHouseModes::HOUSE_MODE_TIME_TO_SWITCH, -1 );
}

int getAlarmDelay( IDbClient& dbClient ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    if( houseModeSettings.empty() ) {
        return -1;
    }

    Json entryDelay;
    json_get( houseModeSettings, CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY, entryDelay );
    return json_get_int( entryDelay, CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_NORMAL );
}

Json getEntryDelay( IDbClient& dbClient ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    if( houseModeSettings.empty() ) {
        return JSON_EMPTY_OBJ;
    }
    Json entryDelay;
    json_get( houseModeSettings, CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY, entryDelay );
    return entryDelay;
}

void setEntryDelay( Services::Service& broadcaster, IDbClient& dbClient, const JsonObject& data ) {
    JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    houseModeSettings[ CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY ] = data;
    dbClient.update( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), houseModeSettings );

    LOGGER_DEBUG( "Updating entry delay values, entryDelay: " + cc::j( data ) );
    using namespace Resources::BroadcastFields;
    Json result = JSON( CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY << data << DeviceTimestampFieldName << now() );

    broadcaster.sendBroadcast( HOUSE_MODE_ENTRY_DELAY_CHANGED, result );
}

std::tuple< std::string, std::string > getAlarmPhase( IDbClient& dbClient ) {
    const JsonObject houseModePhase = dbClient.findOne( HaTempHouseModesAlarmPhaseDBName, JSON_EMPTY_OBJ, nullptr );

    if( houseModePhase.empty() ) {
        return std::make_tuple( "", "" );
    }

    auto phase = json_get_str( houseModePhase, AlarmPhase::Phase );
    auto status = json_get_str( houseModePhase, AlarmPhase::Status );

    return std::make_tuple( phase, status );
}

std::string getSwitchToPhase( IDbClient& dbClient ) {
    const JsonObject houseModePhase = dbClient.findOne( HaTempHouseModesSwitchToPhaseDBName, JSON_EMPTY_OBJ, nullptr );
    if( houseModePhase.empty() ) {
        return "";
    }
    auto status = json_get_str( houseModePhase, AlarmPhase::Status );
    return status;
}

void setAlarmPhase( IDbClient& dbClient, const std::string& phase, const std::string& status ) {
    const JsonObject houseModePhase = dbClient.findOne( HaTempHouseModesAlarmPhaseDBName, JSON_EMPTY_OBJ, nullptr );

    if( houseModePhase.empty() ) {
        dbClient.insert( HaTempHouseModesAlarmPhaseDBName,
                         JSON( AlarmPhase::Phase << phase << AlarmPhase::Status << status ) );
        return;
    }

    dbClient.update( HaTempHouseModesAlarmPhaseDBName,
                     JSON_EMPTY_OBJ,
                     JSON( AlarmPhase::Phase << phase << AlarmPhase::Status << status ) );
}

void setSwitchToPhase( IDbClient& dbClient, const std::string& status ) {
    const JsonObject houseModePhase = dbClient.findOne( HaTempHouseModesSwitchToPhaseDBName, JSON_EMPTY_OBJ, nullptr );

    if( houseModePhase.empty() ) {
        dbClient.insert( HaTempHouseModesSwitchToPhaseDBName, JSON( AlarmPhase::Status << status ) );
        return;
    }

    dbClient.update( HaTempHouseModesSwitchToPhaseDBName, JSON_EMPTY_OBJ, JSON( AlarmPhase::Status << status ) );
}

void resetEntryDelay( Services::Service& broadcaster, IDbClient& dbClient ) {
    auto data = getDefaultEntryDelay();

    JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    houseModeSettings[ CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY ] = data;
    dbClient.update( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), houseModeSettings );

    LOGGER_DEBUG( "Resting entry delay values, entryDelay: " + cc::j( data ) );
    using namespace Resources::BroadcastFields;
    Json result = JSON( CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY << data << DeviceTimestampFieldName << now() );

    broadcaster.sendBroadcast( HOUSE_MODE_ENTRY_DELAY_CHANGED, result );
}

JsonObject getDefaultEntryDelay() {
    HubPlatform::ConfigReader configReader;
    auto normalDelay = configReader.getInt( HubPlatform::ConfigKeys::HouseModeEntryDelayNormal );
    auto longExtendedDelay = configReader.getInt( HubPlatform::ConfigKeys::HouseModeEntryDelayLongExtended );
    auto extendedDelay = configReader.getInt( HubPlatform::ConfigKeys::HouseModeEntryDelayExtended );
    auto instantDelay = configReader.getInt( HubPlatform::ConfigKeys::HouseModeEntryDelayInstant );

    return JSON( CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_NORMAL
                 << normalDelay << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_LONG_EXTENDED << longExtendedDelay
                 << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_EXTENDED << extendedDelay
                 << CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY_INSTANT << instantDelay );
}

bool setDisarmedDefault( Services::Service& broadcaster,
                         IDbModifier& dBModifier,
                         const std::string& modeId,
                         bool disarmedDefault ) {
    IDbClient& dbClient = dBModifier.getDBClient();
    auto houseModeJson =
        dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << modeId ), nullptr );
    if( houseModeJson.empty() )
        return false;

    auto houseMode = getHouseMode( dbClient, modeId );
    bool oldDisarmedDefault = houseMode->m_disarmedDefault;
    dbClient.update( DBNames::HouseModesDB,
                     JSON( HouseModeFields::IdFieldName << modeId ),
                     JSON( "$set" << JSON( HouseModeFields::DisarmedDefaultFieldName << disarmedDefault ) ) );
    houseMode->m_disarmedDefault = disarmedDefault;
    auto currentHouseModeId = getCurrentHouseModeId( dbClient );

    if( disarmedDefault ) {
        houseMode->m_disarmedDeviceIds.clear();
        auto disarmedDeviceIds = getDefaultDisarmedDeviceIDs( dbClient, modeId );
        houseMode->m_disarmedDeviceIds.insert( disarmedDeviceIds.begin(), disarmedDeviceIds.end() );

        houseMode->m_alarmsOffDeviceIds.clear();
        auto alarmsOffDeviceIds = getDefaultAlarmOffDeviceIDs( dbClient, modeId );
        houseMode->m_alarmsOffDeviceIds.insert( alarmsOffDeviceIds.begin(), alarmsOffDeviceIds.end() );

        houseMode->m_camerasOffDeviceIds.clear();
        auto camerasOffDeviceIds = getDefaultCamerasOffDeviceIDs( dbClient, modeId );
        houseMode->m_camerasOffDeviceIds.insert( camerasOffDeviceIds.begin(), camerasOffDeviceIds.end() );

        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << modeId ),
                         JSON( "$set" << JSON( HouseModeFields::DisarmedDevicesFieldName
                                               << disarmedDeviceIds << HouseModeFields::AlarmsOffDevicesFieldName
                                               << alarmsOffDeviceIds << HouseModeFields::CamerasOffDevicesFieldName
                                               << camerasOffDeviceIds << HouseModeFields::NotificationsFieldName
                                               << HouseModeEntry::notificationsToJson(
                                                      houseMode->m_notificationUserIdsDefault ) ) ) );

        if( currentHouseModeId == modeId ) {
            auto allArmedDeviceIds = getArmedDeviceIDsAffectedByHouseModes( dbClient );
            for( const auto& deviceId : allArmedDeviceIds ) {
                bool defaultStatusArmed = true;
                auto it = houseMode->m_disarmedDeviceIds.find( deviceId );
                if( it != houseMode->m_disarmedDeviceIds.end() )
                    defaultStatusArmed = false;
                const JsonObject device = dbClient.findOne(
                    HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB,
                    JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::IdFieldName << deviceId ),
                    nullptr );
                bool deviceArmed = json_get_bool( device, "armed" );
                if( defaultStatusArmed != deviceArmed ) {
                    Log::info( "CoreDHouseModes::setDisarmedDefault In current mode change status device " + deviceId );
                    dBModifier.update( deviceId,
                                       JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName
                                             << defaultStatusArmed ) );
                }
            }
        }
    }

    if( oldDisarmedDefault ^ disarmedDefault ) {
        using namespace Resources::BroadcastFields;
        auto info =
            JSON( "modeId" << modeId << "disarmedDefault" << disarmedDefault << DeviceTimestampFieldName << now() );
        broadcaster.sendBroadcast( HOUSE_MODE_CHANGED, info );
    }

    return true;
}

bool addDisarmedDevice( Services::Service& broadcaster,
                        IDbModifier& dBModifier,
                        const std::string& modeId,
                        const std::string& deviceId,
                        bool newDevice,
                        bool changeArmedState ) {
    auto& dbClient = dBModifier.getDBClient();
    const auto deviceFields = JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName << 1 );
    auto deviceJson =
        dbClient.findOne( HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB,
                          JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::IdFieldName << deviceId ),
                          &deviceFields );
    if( deviceJson.empty() )
        return false;

    bool deviceArmed = json_get_bool( deviceJson, HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName );
    const auto fields = JSON( HouseModeFields::DisarmedDevicesFieldName << 1 );
    auto json = dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << modeId ), &fields );

    JsonArray deviceIdsJson = JSON_EMPTY_ARR;
    if( !json.empty() )
        json_get_arr( json, HouseModeFields::DisarmedDevicesFieldName, deviceIdsJson );

    auto deviceIds = fromJsonArray( deviceIdsJson );
    auto it = deviceIds.find( deviceId );
    if( it != deviceIds.end() )
        return false;

    deviceIds.insert( deviceId );
    dbClient.update( DBNames::HouseModesDB,
                     JSON( HouseModeFields::IdFieldName << modeId ),
                     JSON( "$set" << JSON( HouseModeFields::DisarmedDevicesFieldName << deviceIds ) ) );

    auto houseMode = getHouseMode( dbClient, modeId );
    houseMode->m_disarmedDeviceIds.insert( deviceId );

    if( changeArmedState ) {
        if( deviceArmed && getCurrentHouseModeId( dbClient ) == modeId ) {
            LOGGER_INFO( "In current mode change status device " + deviceId + " to disarmed" );
            dBModifier.update(
                deviceId, JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName << !deviceArmed ) );
        }

        if( !newDevice && houseMode->m_disarmedDefault ) {
            setDisarmedDefault( broadcaster, dBModifier, modeId, false );
        }
    }
    else {
        LOGGER_INFO( "Ignore disarmedDefault change" );
    }

    return true;
}

/** @param changeArmedState - should be false only for modify_device() migration calls to avoid changing
 * device.armed field. */
bool removeDisarmedDevice( Services::Service& broadcaster,
                           IDbModifier& dBModifier,
                           const std::string& modeId,
                           const std::string& deviceId,
                           bool changeArmedState ) {
    auto& dbClient = dBModifier.getDBClient();
    const auto deviceFields = JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName << 1 );
    auto deviceJson =
        dbClient.findOne( HubPlatform::CoreD::Model::Resources::DBNames::DevicesDB,
                          JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::IdFieldName << deviceId ),
                          &deviceFields );
    if( deviceJson.empty() )
        return false;

    bool deviceArmed = json_get_bool( deviceJson, HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName );

    const auto fields = JSON( HouseModeFields::DisarmedDevicesFieldName << 1 );
    auto json = dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << modeId ), &fields );
    if( json.empty() )
        return false;

    JsonArray deviceIdsJson;
    json_get_arr( json, HouseModeFields::DisarmedDevicesFieldName, deviceIdsJson );

    auto deviceIds = fromJsonArray( deviceIdsJson );
    auto it = deviceIds.find( deviceId );
    if( it == deviceIds.end() )
        return false;

    deviceIds.erase( it );
    dbClient.update( DBNames::HouseModesDB,
                     JSON( HouseModeFields::IdFieldName << modeId ),
                     JSON( "$set" << JSON( HouseModeFields::DisarmedDevicesFieldName << deviceIds ) ) );

    auto houseMode = getHouseMode( dbClient, modeId );
    houseMode->m_disarmedDeviceIds.erase( deviceId );

    if( changeArmedState ) {
        if( !deviceArmed && getCurrentHouseModeId( dbClient ) == modeId ) {
            LOGGER_INFO( "In current mode change status device " + deviceId + " to armed" );
            dBModifier.update(
                deviceId, JSON( HubPlatform::CoreD::Model::Resources::DeviceFields::ArmedFieldName << !deviceArmed ) );
        }

        if( houseMode->m_disarmedDefault ) {
            setDisarmedDefault( broadcaster, dBModifier, modeId, false );
        }
    }
    else {
        LOGGER_INFO( "Ignore disarmedDefault change" );
    }

    return true;
}

bool setNotifications( IDbClient& dbClient,
                       Services::Service& broadcaster,
                       IDbModifier& dBModifier,
                       const std::string& modeId,
                       bool notifyAll,
                       const std::vector< std::string >& userIds ) {
    const auto fields = JSON( HouseModeFields::NotificationsFieldName << 1 );
    auto json = dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << modeId ), &fields );

    auto oldHouseMode = CoreDHouseModes::getHouseMode( dbClient, modeId );

    if( notifyAll ) {
        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << modeId ),
                         JSON( "$set" << JSON( HouseModeFields::NotificationsFieldName << nullptr ) ) );
    }
    else {
        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << modeId ),
                         JSON( "$set" << JSON( HouseModeFields::NotificationsFieldName << userIds ) ) );
    }

    setDisarmedDefault( broadcaster, dBModifier, modeId, false );

    auto newHouseMode = CoreDHouseModes::getHouseMode( dbClient, modeId );

    if( oldHouseMode && newHouseMode ) {
        if( !oldHouseMode->m_notificationUserIds ^ !newHouseMode->m_notificationUserIds ) {
            auto info = JSON( "modeId" << modeId << "all" << !newHouseMode->m_notificationUserIds );
            broadcaster.sendBroadcast( HOUSE_MODE_NOTIFICATIONS_NOTIFYALL, info );
        }

        if( newHouseMode->m_notificationUserIds ) {
            auto oldNotifications = oldHouseMode->m_notificationUserIds ? oldHouseMode->m_notificationUserIds :
                                                                          std::make_shared< std::set< std::string > >();
            std::set< std::string > diff;
            std::set_difference( oldNotifications->begin(),
                                 oldNotifications->end(),
                                 newHouseMode->m_notificationUserIds->begin(),
                                 newHouseMode->m_notificationUserIds->end(),
                                 std::inserter( diff, diff.begin() ) );
            for( const auto& userId : diff ) {
                auto info = JSON( "modeId" << modeId << "notification" << userId );
                broadcaster.sendBroadcast( HOUSE_MODE_NOTIFICATIONS_REMOVED, info );
            }
            diff.clear();
            std::set_difference( newHouseMode->m_notificationUserIds->begin(),
                                 newHouseMode->m_notificationUserIds->end(),
                                 oldNotifications->begin(),
                                 oldNotifications->end(),
                                 std::inserter( diff, diff.begin() ) );
            for( const auto& userId : diff ) {
                auto info = JSON( "modeId" << modeId << "notification" << userId );
                broadcaster.sendBroadcast( HOUSE_MODE_NOTIFICATIONS_ADDED, info );
            }
        }
    }

    return true;
}

void sendHouseModeAlarmedBroadcast( Services::Service& broadcaster,
                                    const std::string& modeId,
                                    const std::string& deviceId ) {
    using namespace Resources::BroadcastFields;
    Json data = JSON( ModeIdFieldName << modeId << DeviceIdFieldName << deviceId << DeviceTimestampFieldName << now() );
    broadcaster.sendBroadcast( HOUSE_MODE_ALARMED, data );
}

void sendHouseModeAlarmedBroadcast( Services::Service& broadcaster,
                                    const std::string& modeId,
                                    const std::string& status,
                                    const std::string& phase,
                                    const std::string& deviceId,
                                    bool silent,
                                    bool chime,
                                    std::string& soundType,
                                    int pendingDelay ) {
    using namespace Resources::BroadcastFields;
    Json data = JSON( ModeIdFieldName << modeId << DeviceTimestampFieldName << now() << DeviceStatusFieldName << status
                                      << DevicePhaseFieldName << phase << Silent << silent << Chime << chime
                                      << SoundType << soundType );

    if( !deviceId.empty() ) {
        data[ DeviceIdFieldName ] = deviceId;
    }

    if( pendingDelay && phase != Model::Resources::HouseModeAlarmStatus::Main ) {
        data[ DevicePendingDelay ] = pendingDelay;
    }

    broadcaster.sendBroadcast( HOUSE_MODE_ALARMED, data );
}

bool setProtect( Services::Service& broadcaster, IDbModifier& dBModifier, const std::string& modeId, bool protect ) {
    IDbClient& dbClient = dBModifier.getDBClient();

    const auto houseMode = getHouseMode( dbClient, modeId, true );
    if( !houseMode ) {
        return false;
    }

    if( houseMode->m_protect != protect ) {
        using namespace Resources::BroadcastFields;
        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << modeId ),
                         JSON( "$set" << JSON( HouseModeFields::ProtectFieldName << protect ) ) );
        broadcaster.sendBroadcast(
            HOUSE_MODE_CHANGED,
            JSON( "modeId" << modeId << "protect" << protect << DeviceTimestampFieldName << now() ) );
    }

    return true;
}

bool setProtectTimestamp( IDbModifier& dBModifier, const std::string& modeId, std::chrono::milliseconds timestamp ) {
    IDbClient& dbClient = dBModifier.getDBClient();

    checkCommandTimestampDb( dbClient, modeId, timestamp );

    if( !checkCommandTimestampTempDb( dbClient, modeId, timestamp ) ) {
        auto jsonData =
            dbClient.findOne( HaTempHouseModesCommandTimestampsDBName,
                              JSON( HouseModeCommandFields::CommandFieldName
                                    << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << modeId ),
                              nullptr );
        auto timestampValue = json_get_int64( jsonData, "timestamp" );

        if( timestampValue == 0 ) {
            dbClient.update(
                HaTempHouseModesCommandTimestampsDBName,
                JSON( HouseModeCommandFields::CommandFieldName << "hub.modes.protect.set"
                                                               << HouseModeCommandFields::ModeIdFieldName << modeId ),
                JSON( "$set" << JSON( HouseModeCommandFields::TimestampFieldName << timestamp.count() ) ) );
        }
        else if( timestampValue < timestamp.count() ) {
            dbClient.update(
                HaTempHouseModesCommandTimestampsDBName,
                JSON( HouseModeCommandFields::CommandFieldName << "hub.modes.protect.set"
                                                               << HouseModeCommandFields::ModeIdFieldName << modeId ),
                JSON( "$set" << JSON( HouseModeCommandFields::TimestampFieldName << timestamp.count() ) ) );
        }
        else if( timestampValue > timestamp.count() ) {
            return false;
        }
    }

    return true;
}

/// NB: device should exist in devices db, check it outside.
bool protectButtonsSet( Services::Service& broadcaster, const std::string& deviceId, ProtectServiceEnum service ) {
    IDbClient& dbClient = broadcaster.getDBClient();
    const auto tableName = DBNames::HouseModesProtectButtonsDB;

    bool isFound = false;
    ProtectButton device;
    std::tie( device, isFound ) = findProtectButtonById( dbClient, deviceId );

    if( !badValue( service ) ) {
        if( isFound && !device.empty() ) {
            if( device.service() == service ) {
                LOGGER_TRACE( "protectButtons: service value is the same, Skip." );
                return false;
            }
            else {
                const std::string& serviceStr = toString( service );
                dbClient.update(
                    tableName, JSON( "_id" << deviceId ), JSON( "$set" << JSON( "service" << serviceStr ) ) );

                broadcaster.sendBroadcast( HOUSE_MODE_PROTECT_BUTTONS_UPDATED,
                                           JSON( "deviceId" << deviceId << "service" << serviceStr ) );

                CoreDHouseModes::ScriptEvents::ProtectSosButtonUpdated luaEvent( deviceId, toString( service ) );
                broadcaster.emit( luaEvent );
            }
        }
        else {
            const std::string& serviceStr = toString( service );
            dbClient.insert( tableName, JSON( "_id" << deviceId << "service" << serviceStr ) );

            broadcaster.sendBroadcast( HOUSE_MODE_PROTECT_BUTTONS_ADDED,
                                       JSON( "deviceId" << deviceId << "service" << serviceStr ) );
            CoreDHouseModes::ScriptEvents::ProtectSosButtonAdded luaEvent( deviceId, toString( service ) );
            broadcaster.emit( luaEvent );
        }
    }
    else {
        if( isFound ) {
            removeProtectButton( dbClient, deviceId );

            broadcaster.sendBroadcast( HOUSE_MODE_PROTECT_BUTTONS_REMOVED, JSON( "deviceId" << deviceId ) );

            CoreDHouseModes::ScriptEvents::ProtectSosButtonRemoved luaEvent( deviceId );
            broadcaster.emit( luaEvent );
        }
        else {
            LOGGER_TRACE( cc::trace( "No record to remove for deviceId=" ) + deviceId );
            return false;
        }
    }
    return true;
}

std::vector< ProtectButton > getProtectButtons( IDbClient& dbClient ) {
    auto fields = JSON( "_id" << 1 << "service" << 1 );
    auto records = dbClient.findAll( DBNames::HouseModesProtectButtonsDB, JSON_EMPTY_OBJ, &fields );
    std::vector< ProtectButton > result;
    result.reserve( records.size() );
    for( const auto& item : records ) {
        ProtectButton device;
        if( device.fromDbJson( item ) ) {
            result.push_back( std::move( device ) );
        }
    }
    return result;
}

std::pair< ProtectButton, bool > findProtectButtonById( IDbClient& dbClient, const std::string& deviceId ) {
    auto json = dbClient.findOne(
        DBNames::HouseModesProtectButtonsDB, JSON( HouseModeFields::IdFieldName << deviceId ), nullptr );
    std::pair< ProtectButton, bool > result;

    if( json.empty() ) {
        result.second = false;
        return result;
    }

    ProtectButton& device = result.first;
    device.fromDbJson( json );
    result.second = true;

    return result;
}

bool removeProtectButton( IDbClient& dbClient, const std::string& deviceId ) {
    const auto tableName = DBNames::HouseModesProtectButtonsDB;
    dbClient.remove( tableName, JSON( "_id" << deviceId ) );
    return true;
}

bool protectDevicesAdd( Services::Service& broadcaster, const DeviceIDs& ids ) {
    IDbClient& dbClient = broadcaster.getDBClient();
    auto storedIds = getProtectDevicesIds( dbClient, JSON_EMPTY_OBJ );
    using OrderedIds = std::set< std::string >;
    OrderedIds input( begin( ids ), end( ids ) ), filteredInput;

    std::set_difference( begin( input ),
                         end( input ),
                         begin( storedIds ),
                         end( storedIds ),
                         std::inserter( filteredInput, begin( filteredInput ) ) );

    if( !filteredInput.empty() ) {
        for( const auto& id : filteredInput ) {
            dbClient.insert( DBNames::HouseModesProtectDevicesDB, JSON( db::ID_FIELD_NAME << id ) );
        }

        broadcaster.sendBroadcast( HOUSE_MODES_PROTECT_DEVICES_ADDED, JSON( "deviceIds" << filteredInput ) );
        return true;
    }
    return false;
}

bool protectDevicesRemove( Services::Service& broadcaster, const DeviceIDs& ids ) {
    IDbClient& dbClient = broadcaster.getDBClient();
    auto storedIds = getProtectDevicesIds( dbClient, JSON_EMPTY_OBJ );
    using OrderedIds = std::set< std::string >;
    OrderedIds input( begin( ids ), end( ids ) ), filteredInput;

    std::set_intersection( begin( input ),
                           end( input ),
                           begin( storedIds ),
                           end( storedIds ),
                           std::inserter( filteredInput, begin( filteredInput ) ) );

    if( !filteredInput.empty() ) {
        for( const auto& id : filteredInput ) {
            removeProtectDevice( dbClient, id );
        }

        broadcaster.sendBroadcast( HOUSE_MODES_PROTECT_DEVICES_REMOVED, JSON( "deviceIds" << filteredInput ) );
        return true;
    }
    return false;
}

DeviceIDs getProtectDevicesIds( IDbClient& dbClient, const Json& dbQuery ) {
    auto fields = JSON( db::ID_FIELD_NAME << 1 );
    auto records = dbClient.findAll( DBNames::HouseModesProtectDevicesDB, dbQuery, &fields );
    DeviceIDs result;
    result.reserve( records.size() );
    for( const auto& item : records ) {
        result.push_back( json_get_str( item, db::ID_FIELD_NAME ) );
    }
    return result;
}

bool removeProtectDevice( IDbClient& dbClient, const std::string& deviceId ) {
    const auto tableName = DBNames::HouseModesProtectDevicesDB;
    dbClient.remove( tableName, JSON( "_id" << deviceId ) );
    return true;
}

std::tuple< bool, std::vector< std::string >, JsonObject > readDeviceIdsParams( const JsonObject& params ) {
    JsonObject result;
    std::vector< std::string > deviceIds;
    auto deviceIdsIt = params.find( "deviceIds" );
    if( deviceIdsIt == params.end() || deviceIdsIt->empty() ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getBadRequest( "deviceIds", error );
        setErrorObject( result, error );
        return std::make_tuple( false, deviceIds, result );
    }

    try {
        jsonCheckArray( params, "deviceIds", []( const JsonArray& arr ) {
            for( const auto& item : arr )
                jsonCheckString( item.get< std::string >(), JsonStringChecks::NON_EMPTY );
        } );

        fromJsonStringArray( *deviceIdsIt, [&deviceIds]( const std::string& item ) { deviceIds.push_back( item ); } );
        return std::make_tuple( true, deviceIds, result );
    }
    catch( const std::exception& ex ) {
        JsonObject error = JSON_EMPTY_OBJ;
        Errors::getWrongParams( "deviceIds", error );
        setErrorObject( result, error, "Format of a required field is wrong" );
        return std::make_tuple( false, deviceIds, result );
    }
}

bool updateDeviceIdList( IDbClient& dbClient,
                         const std::string& modeId,
                         const std::string& deviceId,
                         const char* const fieldName,
                         bool addDevice ) {
    if( deviceId.empty() )
        throw std::runtime_error( "call has not valid data with current houseModeId/deviceId" );

    const auto deviceIdsJson = findHouseModesArrayField( dbClient, modeId, fieldName );
    auto deviceIds = fromJsonArray( deviceIdsJson );
    if( addDevice ) {
        auto it = deviceIds.find( deviceId );
        if( it != deviceIds.end() )
            return false;

        deviceIds.insert( deviceId );
    }
    else {
        auto it = deviceIds.find( deviceId );
        if( it == deviceIds.end() )
            return false;

        deviceIds.erase( it );
    }

    dbClient.update( DBNames::HouseModesDB,
                     JSON( HouseModeFields::IdFieldName << modeId ),
                     JSON( "$set" << JSON( fieldName << deviceIds ) ) );

    return true;
}

bool addAlarmsOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId ) {
    return updateDeviceIdList( dbClient, modeId, deviceId, HouseModeFields::AlarmsOffDevicesFieldName, true );
}

bool removeAlarmsOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId ) {
    return updateDeviceIdList( dbClient, modeId, deviceId, HouseModeFields::AlarmsOffDevicesFieldName, false );
}

bool addCamerasOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId ) {
    return updateDeviceIdList( dbClient, modeId, deviceId, HouseModeFields::CamerasOffDevicesFieldName, true );
}

bool removeCamerasOffDevice( IDbClient& dbClient, const std::string& modeId, const std::string& deviceId ) {
    return updateDeviceIdList( dbClient, modeId, deviceId, HouseModeFields::CamerasOffDevicesFieldName, false );
}

void sendBypassDevicesBroadcast( Services::Service& broadcaster,
                                 const std::string& modeId,
                                 JsonArray deviceIds,
                                 bool added ) {
    using namespace CoreDHouseModes::Resources;
    const Json broadcastData = { { BroadcastFields::ModeIdFieldName, modeId },
                                 { BroadcastFields::DeviceIdsFieldName, std::move( deviceIds ) } };

    const auto subclass = added ? HOUSE_MODE_BYPASS_DEVICES_ADDED : HOUSE_MODE_BYPASS_DEVICES_REMOVED;
    broadcaster.sendBroadcast( subclass, broadcastData );
}

bool updateBypassDevices( Services::Service& broadcaster,
                          const std::string& modeId,
                          const DeviceIDs& deviceIds,
                          bool add ) {
    IDbClient& dbClient = broadcaster.getDBClient();
    const auto deviceIdsJson = findHouseModesArrayField( dbClient, modeId, HouseModeFields::BypassDevicesFieldName );
    auto storedIds = fromJsonArray( deviceIdsJson );

    JsonArray updatedIds = JSON_EMPTY_ARR;
    if( add ) {
        for( const std::string& deviceId : deviceIds ) {
            bool inserted = false;
            std::tie( std::ignore, inserted ) = storedIds.insert( deviceId );
            if( inserted )
                updatedIds.push_back( deviceId );
        }
    }
    else {
        for( const std::string& deviceId : deviceIds ) {
            if( storedIds.erase( deviceId ) > 0 )
                updatedIds.push_back( deviceId );
        }
    }

    const bool atLeaseOneDeviceUpdated = !updatedIds.empty();
    if( atLeaseOneDeviceUpdated ) {
        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << modeId ),
                         JSON( "$set" << JSON( HouseModeFields::BypassDevicesFieldName << storedIds ) ) );

        sendBypassDevicesBroadcast( broadcaster, modeId, std::move( updatedIds ), add );
    }

    return atLeaseOneDeviceUpdated;
}

bool addBypassDevices( Services::Service& broadcaster, const std::string& modeId, const DeviceIDs& deviceIds ) {
    const bool add = true;
    return updateBypassDevices( broadcaster, modeId, deviceIds, add );
}

bool removeBypassDevices( Services::Service& broadcaster, const std::string& modeId, const DeviceIDs& deviceIds ) {
    const bool add = false;
    return updateBypassDevices( broadcaster, modeId, deviceIds, add );
}

void removeAllBypassDevices( Services::Service& broadcaster, const std::string& modeId ) {
    IDbClient& dbClient = broadcaster.getDBClient();
    auto deviceIdsJson = findHouseModesArrayField( dbClient, modeId, HouseModeFields::BypassDevicesFieldName );
    if( !deviceIdsJson.empty() ) {
        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << modeId ),
                         JSON( "$set" << JSON( HouseModeFields::BypassDevicesFieldName << JSON_EMPTY_ARR ) ) );

        sendBypassDevicesBroadcast( broadcaster, modeId, std::move( deviceIdsJson ), false /* added */ );
    }
}

bool addCamera( IDbClient& dbClient, const std::string& deviceId, const std::string& makeRecordingItemId ) {
    const auto tableName = DBNames::HouseModesCamerasDB;
    dbClient.insert( tableName, JSON( db::ID_FIELD_NAME << deviceId << Items::MakeRecording << makeRecordingItemId ) );
    return true;
}

bool removeCamera( IDbClient& dbClient, const std::string& deviceId ) {
    const auto tableName = DBNames::HouseModesCamerasDB;
    dbClient.remove( tableName, JSON( db::ID_FIELD_NAME << deviceId ) );
    return true;
}

bool eraseDisarmedDevice( IDbClient& dbClient, const std::string& deviceId ) {
    auto houseModes = getHouseModesList( dbClient );
    for( auto& houseMode : houseModes ) {
        const auto fields = JSON( HouseModeFields::DisarmedDevicesFieldName << 1 );
        auto json =
            dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << houseMode.m_id ), &fields );
        if( json.empty() )
            return false;

        JsonArray deviceIdsJson;
        json_get_arr( json, HouseModeFields::DisarmedDevicesFieldName, deviceIdsJson );
        auto deviceIds = fromJsonArray( deviceIdsJson );
        auto it = deviceIds.find( deviceId );
        if( it == deviceIds.end() )
            continue;

        deviceIds.erase( it );
        dbClient.update( DBNames::HouseModesDB,
                         JSON( HouseModeFields::IdFieldName << houseMode.m_id ),
                         JSON( "$set" << JSON( HouseModeFields::DisarmedDevicesFieldName << deviceIds ) ) );

        houseMode.m_disarmedDeviceIds.erase( deviceId );
    }
    return true;
}

bool modifyDevice( Services::Service& broadcaster,
                   IDbModifier& dbModifier,
                   const std::string& deviceId,
                   const JsonObject& device ) {
    DeviceHouseModeSettingsModifier modifier( broadcaster, dbModifier, deviceId, device );
    return modifier.modifyDevice();
}

bool addSecurityDevice( IDbClient& dbClient, const std::string& deviceId, const std::string& securityThreatItemId ) {
    const auto tableName = DBNames::HouseModesSecurityThreatsDB;
    dbClient.insert( tableName,
                     JSON( db::ID_FIELD_NAME << deviceId << Items::SecurityThreat << securityThreatItemId ) );
    return true;
}

bool removeSecurityDevice( IDbClient& dbClient, const std::string& deviceId ) {
    const auto tableName = DBNames::HouseModesSecurityThreatsDB;
    dbClient.remove( tableName, JSON( db::ID_FIELD_NAME << deviceId ) );
    return true;
}

size_t getSecurityDeviceItemPairs( const IDbClient& dbClient,
                                   const Json& query,
                                   const ProcessDbResultsetItem& callback ) {
    //
    size_t counter = 0;
    auto fields = JSON( db::ID_FIELD_NAME << 1 << Items::SecurityThreat << 1 );
    auto records = dbClient.findAll( DBNames::HouseModesSecurityThreatsDB, query, &fields );
    for( const auto& record : records ) {
        callback( record );
        counter++;
    }
    return counter;
}

JsonArray findHouseModesArrayField( IDbClient& dbClient, const std::string& modeId, const char* fieldName ) {
    if( modeId.empty() || !fieldName || !*fieldName )
        throw std::runtime_error( "call has not valid data with current houseModeId/deviceId" );

    const auto fields = JSON( fieldName << 1 );
    auto json = dbClient.findOne( DBNames::HouseModesDB, JSON( HouseModeFields::IdFieldName << modeId ), &fields );

    JsonArray result = JSON_EMPTY_ARR;
    if( !json.empty() )
        json_get_arr( json, fieldName, result );

    return result;
}

JsonObject getHouseModesAlarmDevicesEvent( IDbClient& dbClient, const std::string& id ) {
    return dbClient.findOne( HaTempHouseModesAlarmDevicesDBName, JSON( db::ID_FIELD_NAME << id ), nullptr );
}

JsonObject getHouseModesAlarmDevicesEvents( IDbClient& dbClient ) {
    return dbClient.findAll( HaTempHouseModesAlarmDevicesDBName, JSON_EMPTY_OBJ, nullptr );
}

void deleteDeviceFromHouseModesAlarmedList( IDbClient& dbClient, const std::string& id ) {
    dbClient.remove( HaTempHouseModesAlarmDevicesDBName, JSON( db::ID_FIELD_NAME << id ) );
}

void addDeviceToHouseModesAlarmedList( IDbClient& dbClient,
                                       const std::string& id,
                                       unsigned long timestamp,
                                       int delay,
                                       bool silentAlarm ) {
    JsonObject query =
        JSON( db::ID_FIELD_NAME << id << "timestamp" << timestamp << "alarmDelay" << delay << "silent" << silentAlarm );
    dbClient.insert( HaTempHouseModesAlarmDevicesDBName, query );
}

void sendsAlarmPhase( Services::Service& broadcaster, const std::string& phase ) {
    broadcaster.sendBroadcast( HOUSE_MODE_PHASE_CHANGED, JSON( "phase" << phase ) );
}

void clearDeviceFromHouseModesAlarmedList( IDbClient& dbClient ) {
    dbClient.clear( HaTempHouseModesAlarmDevicesDBName );
}

int64_t now() {
    return std::chrono::system_clock::now().time_since_epoch().count();
};

void checkCommandTimestampDb( IDbClient& dbClient, const std::string& modeId, std::chrono::milliseconds timestamp ) {
    if( !dbClient.has( DBNames::HouseModesCommandTimestampsDBName,
                       JSON( HouseModeCommandFields::CommandFieldName
                             << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << modeId ) ) ) {
        dbClient.insert( DBNames::HouseModesCommandTimestampsDBName,
                         JSON( HouseModeCommandFields::CommandFieldName
                               << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << modeId
                               << HouseModeCommandFields::TimestampFieldName << timestamp.count() ) );
    }
}

bool checkCommandTimestampTempDb( IDbClient& dbClient,
                                  const std::string& modeId,
                                  std::chrono::milliseconds timestamp ) {
    if( !dbClient.has( HaTempHouseModesCommandTimestampsDBName,
                       JSON( HouseModeCommandFields::CommandFieldName
                             << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << modeId ) ) ) {
        dbClient.insert( HaTempHouseModesCommandTimestampsDBName,
                         JSON( HouseModeCommandFields::CommandFieldName
                               << "hub.modes.protect.set" << HouseModeCommandFields::ModeIdFieldName << modeId
                               << HouseModeCommandFields::TimestampFieldName << timestamp.count() ) );
        return true;
    }
    return false;
}

int fetchEntryDelayValue( IDbClient& dbClient, const std::string& deviceId ) {
    DeviceGroupFindHandler findHandler( dbClient, Model::Resources::DBNames::DeviceGroupsDB );
    const JsonObject deviceObject = dbClient.findOne( Model::Resources::DBNames::DevicesDB,
                                                      JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ),
                                                      nullptr );

    Json groupToFind = JSON(
        "category" << json_get_str( deviceObject, Model::Resources::DeviceFields::CategoryFieldName ) << "subcategory"
                   << json_get_str( deviceObject, Model::Resources::DeviceFields::SubcategoryFieldName ) );
    auto deviceGroups = findHandler.processFindRequest( groupToFind );

    if( deviceGroups.empty() ) {
        return -1;
    }
    JsonArray deviceGroupArray;

    json_get_arr( deviceGroups, CoreD::Resources::DeviceGroup::DeviceGroups, deviceGroupArray );
    for( size_t i = 0; i < deviceGroupArray.size(); ++i ) {
        Json deviceGroupIdJson = JSON_EMPTY_OBJ;
        if( json_get( deviceGroupArray, { i, CoreD::Resources::DeviceGroup::Id }, deviceGroupIdJson ) ) {
            auto deviceGroupId = deviceGroupIdJson.get< std::string >();
            if( !deviceGroupId.empty() ) {
                const JsonObject groupDbRecord =
                    dbClient.findOne( Model::Resources::DBNames::DeviceGroupsDB,
                                      JSON( Model::Resources::DeviceFields::IdFieldName << deviceGroupId ),
                                      nullptr );
                if( json_get_str( groupDbRecord, CoreD::Resources::DeviceGroup::Role ) == "house_modes" ) {
                    auto deviceGroupDelay = json_get_str( groupDbRecord, CoreDHouseModes::HOUSE_MODE_ENTRY_DELAY );
                    if( !deviceGroupDelay.empty() && deviceGroupDelay != "none" ) {
                        auto houseModeDelay = CoreDHouseModes::getEntryDelay( dbClient );
                        auto delay = json_get_int( houseModeDelay, deviceGroupDelay, -1 );
                        return delay;
                    }
                }
            }
        }
    }

    return -1;
}

std::chrono::seconds getTimestamp() {
    auto currentTime = std::chrono::system_clock::now();
    return std::chrono::duration_cast< std::chrono::seconds >( currentTime.time_since_epoch() );
}

Json getAbortWindow( IDbClient& dbClient ) {
    const JsonObject houseModeSettings =
        dbClient.findOne( HUBSettings, JSON( db::ID_FIELD_NAME << CoreDHouseModes::HOUSE_MODE_ENTRY ), nullptr );

    if( houseModeSettings.empty() ) {
        return JSON_EMPTY_OBJ;
    }

    Json abortWindow;
    json_get( houseModeSettings, CoreDHouseModes::HOUSE_MODE_ABORT_WINDOW, abortWindow );
    return abortWindow;
}

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
