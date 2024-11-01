#include "vistacam_motion_sensor_migration.h"

#include "config_reader/config_reader.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "resources.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_30_houseModesOptions {

namespace {

const char* const devicesDb = "db.firmware.cored.devices";
const char* const houseModesDb = "db.firmware.cored.houseModes";

}  // namespace

VistaCamMotionSensorMigration::VistaCamMotionSensorMigration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

bool VistaCamMotionSensorMigration::hasEzvizMotionSensorParts( const std::string& deviceTypeId ) {
    return deviceTypeId.find( Types::ezvizVistaCam_MotionSensorPrefix ) != std::string::npos &&
           deviceTypeId.find( Types::ezvizVistaCam_MotionSensorSuffix ) != std::string::npos;
}

bool VistaCamMotionSensorMigration::migrateHouseModesDevices( const std::string& collectionName,
                                                              const std::string& message,
                                                              const Json& dbDeviceFilter,
                                                              const Json& dbHouseModesFilter ) {
    std::vector< Json > deviceIds;

    std::function< void( const Json& ) > inserter;
    auto simpleInserter = [&deviceIds]( const Json& deviceDbRecord ) { deviceIds.push_back( deviceDbRecord ); };
    auto motionSensorVistaCamFilteringInserter = [&deviceIds]( const Json& deviceDbRecord ) {
        auto deviceTypeId = json_get_str( deviceDbRecord, Fields::deviceTypeId );

        if( hasEzvizMotionSensorParts( deviceTypeId ) )
            deviceIds.push_back( deviceDbRecord );
    };

    bool isDevices = collectionName == Fields::disarmedDevices;
    if( isDevices ) {
        inserter = motionSensorVistaCamFilteringInserter;
    }
    else {
        inserter = simpleInserter;
    }

    processDevices( inserter, dbDeviceFilter, Json{} );

    std::vector< Json > houseModes = fetchHouseModes( dbHouseModesFilter );

    return migrateHouseModesDevices( collectionName, message, deviceIds, houseModes );
}

bool VistaCamMotionSensorMigration::migrateHouseModesDevices( const std::string& collectionName,
                                                              const std::string& message,
                                                              const std::vector< Json >& deviceIds,
                                                              const std::vector< Json >& houseModes ) {
    LOGGER_TRACE_RAW(
        cc::trace( "VistaCamMotionSensorMigration migration started for collection: " + collectionName ) );

    try {
        buildStorage( houseModes, collectionName );
        storageMergeIds( deviceIds, houseModes );
        saveToDb( houseModes, collectionName );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Exception in house modes migration: " ) + cc::note( ex.what() ) +
                      cc::error( ", exception type: " ) + cc::note( typeid( ex ).name() ) );
        LOGGER_ERROR( "Migration has failed!" );
        return false;
    }

    LOGGER_TRACE( cc::note( "Sum number of device ids added to different collections: " ) + message + ": " +
                  cc::num10( devicesAdded() ) );
    LOGGER_TRACE_RAW( cc::trace( "VistaCamMotionSensorMigration finished" ) );

    return true;
}

std::vector< Json > VistaCamMotionSensorMigration::fetchHouseModes( const Json& dbQuery ) const {
    return m_dbClient.findAll( houseModesDb, dbQuery, nullptr );
}

int VistaCamMotionSensorMigration::devicesAdded() const {
    return m_devicesAdded;
}

bool VistaCamMotionSensorMigration::processDevices( const FillDevices& fillDevices,
                                                    const Json& dbQuery,
                                                    const Json& dbFieldsFilter ) const {
    const Json* fieldsToReturn = dbFieldsFilter.empty() ? nullptr : &dbFieldsFilter;
    auto ids = m_dbClient.findAll( devicesDb, dbQuery, fieldsToReturn );
    for( const auto& returned : ids ) {
        LOGGER_INFO( cc::j( returned ) );
        fillDevices( returned );
    }
    return !ids.empty();
}

/**
 *  @param collectionName: disarmed | alarmsOff
 */
void VistaCamMotionSensorMigration::buildStorage( const std::vector< Json >& houseModes,
                                                  const std::string& collectionField ) {
    for( const auto& houseModeEntry : houseModes ) {
        std::string houseModeId = json_get_str( houseModeEntry, Fields::id );
        try {
            jsonCheckArray( houseModeEntry, collectionField, [this, &houseModeId]( const JsonArray& deviceIds ) {
                storageFillDeviceIds( houseModeId, deviceIds );
            } );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Exception while iterating house modes: " ).append( cc::error( ex.what() ) ) );
            throw ex;
        }
    }
}

void VistaCamMotionSensorMigration::storageMergeIds( const std::vector< Json >& deviceIds,
                                                     const std::vector< Json >& houseModes ) {
    for( const auto& deviceId : deviceIds ) {
        auto idStr = json_get_str( deviceId, Fields::id );

        auto addDeviceToInMemoryStorage = [this, &idStr]( const Json& houseMode ) {
            auto id = json_get_str( houseMode, Fields::id );

            auto& devices = m_storage[ id ];
            auto it = devices.find( idStr );
            if( it == devices.end() ) {
                auto isInserted = devices.insert( idStr ).second;
                if( isInserted )
                    m_devicesAdded++;
            }
        };
        std::for_each( houseModes.cbegin(), houseModes.cend(), addDeviceToInMemoryStorage );
    }
}

void VistaCamMotionSensorMigration::saveToDb( const std::vector< Json >& houseModes,
                                              const std::string& collectionField ) {
    for( const auto& houseModeEntry : houseModes ) {
        Json deviceIds = Json::array();
        std::string houseModeId = json_get_str( houseModeEntry, Fields::id );
        auto devices = m_storage[ houseModeId ];
        if( !devices.empty() ) {
            LOGGER_TRACE( cc::trace( "disarmedDevices set to " ).append( cc::j( devices ) ) );

            updateHouseModeEntry( houseModeId, devices, collectionField );
        }
    }
}

void VistaCamMotionSensorMigration::storageFillDeviceIds( const std::string& houseModeId, const JsonArray& deviceIds ) {
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

void VistaCamMotionSensorMigration::updateHouseModeEntry( const std::string& houseModeId,
                                                          const JsonArray& devicesArray,
                                                          const std::string& collectionField ) {
    Json updateQuery = JSON( Fields::id << houseModeId );
    Json houseModeModified = JSON( "$set" << JSON( collectionField << devicesArray ) );
    try {
        m_dbClient.update( houseModesDb, updateQuery, houseModeModified );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Exception caught: " + cc::error( ex.what() ) );
        throw ex;
    }
}

}  // namespace v1_30_houseModesOptions
}  // namespace MigrationTool
}  // namespace HubPlatform