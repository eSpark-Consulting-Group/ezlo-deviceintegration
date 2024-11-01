#include "siren_housemodes_migration.h"

#include "config_reader/config_reader.h"
#include "config_reader/config_keys.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_29_sirenHouseModes {

namespace {

const char* const devicesDb = "db.firmware.cored.devices";
const char* const houseModesDb = "db.firmware.cored.houseModes";
const char* const dbCoredSettings = "db.firmware.cored.settings";

}  // namespace

SirenHouseModesMigration::SirenHouseModesMigration( IDbClient& dbClient, ConfigReader* configReader )
: m_dbClient( dbClient )
, m_configReaderPtr( configReader ) {}

namespace Fields {
const char* alarmsOffDevices = "alarmsOffDevices";
const char* id = "_id";
const char* type = "type";
const char* alarmDelay = "alarmDelay";
}  // namespace Fields

namespace Types {
const char* siren = "siren";
}

bool SirenHouseModesMigration::migrate() {
    bool alarmsOff = migrateAlarmsOffDevices();
    bool alarmDelay = migrateCoredSettingsAlarmDelay();
    return alarmsOff && alarmDelay;
}

bool SirenHouseModesMigration::migrateAlarmsOffDevices() {
    LOGGER_TRACE_RAW( "SirenHouseModes alarmsOffDevices migration started" );
    std::vector< Json > sirenIds = fetchAllSirens();
    std::vector< Json > houseModes = fetchHouseModes();

    buildStorage( houseModes );
    storageMergeSirenIds( sirenIds, houseModes );
    saveToDb( houseModes );

    LOGGER_INFO( cc::note( "New alarmsOff devices added to houseModes entries: " ) + cc::num10( sirensAdded() ) );
    LOGGER_TRACE_RAW( "SirenHouseModes alarmsOffDevices migration finished" );
    return true;
}

void SirenHouseModesMigration::updateHouseModeEntry( const std::string& houseModeId,
                                                     const JsonArray& offDevicesArray ) {
    Json updateQuery = JSON( Fields::id << houseModeId );
    Json houseModeModified = JSON( "$set" << JSON( Fields::alarmsOffDevices << offDevicesArray ) );
    try {
        m_dbClient.update( houseModesDb, updateQuery, houseModeModified );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Exception caught: " + cc::error( ex.what() ) );
        throw ex;
    }
}

void SirenHouseModesMigration::storageFillDeviceIds( const std::string& houseModeId, const JsonArray& deviceIds ) {
    if( deviceIds.empty() ) {
        m_storage[ houseModeId ] = {};
        return;
    }
    for( const auto& deviceId : deviceIds ) {
        if( deviceId.empty() || !deviceId.is_string() )
            continue;

        std::string deviceIdStr = deviceId.get< std::string >();
        m_storage[ houseModeId ].insert( deviceIdStr );
    }
}

// extract houseModes from db to in-memory storage
void SirenHouseModesMigration::buildStorage( const std::vector< Json >& houseModes ) {
    for( const auto& houseModeEntry : houseModes ) {
        std::string houseModeId = json_get_str( houseModeEntry, Fields::id );
        try {
            jsonCheckArray(
                houseModeEntry, Fields::alarmsOffDevices, [this, &houseModeId]( const JsonArray& deviceIds ) {
                    storageFillDeviceIds( houseModeId, deviceIds );
                } );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Exception while iterating house modes: " ).append( cc::error( ex.what() ) ) );
            // skip entry
        }
    }
}

// merge in-memory storage with extracted houseModes; save houseModes back to database
void SirenHouseModesMigration::saveToDb( std::vector< Json >& houseModes ) {
    for( auto& houseModeEntry : houseModes ) {
        Json deviceIds = Json::array();
        std::string houseModeId = json_get_str( houseModeEntry, Fields::id );
        auto offDevices = m_storage[ houseModeId ];
        if( !offDevices.empty() ) {
            auto offDevicesArray = houseModeEntry[ Fields::alarmsOffDevices ];
            offDevicesArray = offDevices;
            LOGGER_TRACE( cc::trace( "alarmsOffDevices set to " ).append( cc::j( offDevicesArray ) ) );

            updateHouseModeEntry( houseModeId, offDevicesArray );
        }
    }
}

// update in-memory storage with all found siren identifiers
void SirenHouseModesMigration::storageMergeSirenIds( const std::vector< Json >& sirenIds,
                                                     std::vector< Json >& houseModes ) {
    for( const auto& sirenId : sirenIds ) {
        // append siren id to alarmsOffDevices[]
        auto sirenIdStr = json_get_str( sirenId, Fields::id );

        auto addAlarmOffDeviceToInMemoryStorage = [this, &sirenIdStr]( Json& houseMode ) {
            auto id = json_get_str( houseMode, Fields::id );

            auto& offDevices = m_storage[ id ];
            auto it = offDevices.find( sirenIdStr );
            if( it == offDevices.end() ) {
                auto isInserted = offDevices.insert( sirenIdStr ).second;
                if( isInserted )
                    m_sirensAdded++;
            }
        };
        std::for_each( houseModes.begin(), houseModes.end(), addAlarmOffDeviceToInMemoryStorage );
    }
}

int SirenHouseModesMigration::sirensAdded() const {
    return m_sirensAdded;
}

std::vector< Json > SirenHouseModesMigration::fetchAllSirens() const {
    Json queryAllSirens = JSON( Fields::type << Types::siren );
    Json fieldsToReturn = JSON( Fields::id << true );
    auto sirenIds = m_dbClient.findAll( devicesDb, queryAllSirens, &fieldsToReturn );
    for( const auto& returned : sirenIds ) {
        LOGGER_INFO( cc::j( returned ) );
    }
    return sirenIds;
}

std::vector< Json > SirenHouseModesMigration::fetchHouseModes() const {
    return m_dbClient.findAll( houseModesDb, JSON_EMPTY_OBJ, nullptr );
}

bool SirenHouseModesMigration::migrateCoredSettingsAlarmDelay() {
    auto houseModeSettings = m_dbClient.findOne( dbCoredSettings, JSON( Fields::id << "houseMode" ), nullptr );
    if( !houseModeSettings.empty() ) {
        const int prevDefaultAlarmDelay = 30;
        int actualAlarmDelay = json_get_int( houseModeSettings, Fields::alarmDelay, -1 );
        int alarmDelay = 0;

        try {
            static ConfigReader configReaderDefault;
            ConfigReader& configReader = m_configReaderPtr ? *m_configReaderPtr : configReaderDefault;

            if( configReader.hasValue( ConfigKeys::HouseModeAlarmDelay ) ) {
                alarmDelay = configReader.getInt( ConfigKeys::HouseModeAlarmDelay );
            }
        }
        catch( const std::exception& error ) {
            Log::warn( cc::warn( "Cannot get house mode switch delay and alarm delay from configs: " ) +
                       cc::note( error.what() ) );
        }

        if( alarmDelay != prevDefaultAlarmDelay && alarmDelay != actualAlarmDelay ) {
            LOGGER_DEBUG( "Updating alarmDelay to " + cc::num10( alarmDelay ) );
            m_dbClient.update( dbCoredSettings,
                               JSON( Fields::id << "houseMode" ),
                               JSON( "$set" << JSON( Fields::alarmDelay << alarmDelay ) ) );
        }
        else {
            LOGGER_DEBUG( "Updating alarmDelay is not needed" );
        }
    }
    return true;
}

}  // namespace v1_29_sirenHouseModes
}  // namespace MigrationTool
}  // namespace HubPlatform