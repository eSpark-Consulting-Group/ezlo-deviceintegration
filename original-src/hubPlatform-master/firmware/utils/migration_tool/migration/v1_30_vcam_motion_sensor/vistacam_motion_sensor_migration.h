#pragma once

#include "migration_tool/migration/migration.h"
#include "json_utils/json_utils.h"
#include <set>
#include <map>

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_30_houseModesOptions {

class VistaCamMotionSensorMigration {
public:
    explicit VistaCamMotionSensorMigration( IDbClient& dbClient );
    bool migrateHouseModesDevices( const std::string& collectionName,
                                   const std::string& message,
                                   const Json& dbDeviceFilter,
                                   const Json& dbHouseModesFilter );
    bool migrateHouseModesDevices( const std::string& collectionName,
                                   const std::string& message,
                                   const std::vector< Json >& devices,
                                   const std::vector< Json >& houseModes );

    int devicesAdded() const;
    static bool hasEzvizMotionSensorParts( const std::string& deviceTypeId );

private:
    using DisarmedDeviceIds = std::set< Json >;
    using HouseModeId = std::string;
    using HouseModeDisarmedDevices = std::map< HouseModeId, DisarmedDeviceIds >;

private:
    using FillDevices = std::function< void( Json ) >;
    bool processDevices( const FillDevices& fillDevices, const Json& dbQuery, const Json& dbFieldsFilter ) const;
    std::vector< Json > fetchHouseModes( const Json& dbQuery ) const;

    void buildStorage( const std::vector< Json >& houseModes, const std::string& collectionField );
    void storageMergeIds( const std::vector< Json >& deviceIds, const std::vector< Json >& houseModes );
    void saveToDb( const std::vector< Json >& houseModes, const std::string& collectionField );

    void updateHouseModeEntry( const std::string& houseModeId,
                               const JsonArray& devicesArray,
                               const std::string& collectionField );
    void storageFillDeviceIds( const std::string& houseModeId, const JsonArray& deviceIds );

private:
    IDbClient& m_dbClient;
    HouseModeDisarmedDevices m_storage;
    int m_devicesAdded{ 0 };
};

}  // namespace v1_30_houseModesOptions
}  // namespace MigrationTool
}  // namespace HubPlatform
