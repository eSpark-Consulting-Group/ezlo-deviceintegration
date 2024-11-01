#pragma once

#include "migration_tool/migration/v1_37_securityDevices/resources.h"
#include "migration_tool/migration/migration.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

/*

CHANGELOG:
    - remove device id from houseMode.devices collection if the device doesn't have associated security_threat item;
    - add to house modes all of them which are not in house modes (devices, mode[i].disarmed), default state is
disarmed;
    - an armed device is a device with "security_threat" item getter functionality (hasGetter=true).

*/

class IDbClient;

namespace HubPlatform {
namespace MigrationTool {
namespace v1_37_securityDevices {

class SecurityDeviceMigration : public HubPlatform::MigrationTool::Migration {
public:
    explicit SecurityDeviceMigration( IDbClient& dbClient );
    bool migrate() override;
    size_t recordsInserted() const;
    size_t recordsDeleted() const;
    size_t securityDevicesAdded() const;

    size_t removedFromDisarmedDevices() const;
    size_t insertedInModes() const;
    size_t changedArmedTo( bool value ) const;

public:
    using Collection = std::map< std::string, Json >;
    using CategoriesIndex = std::map< std::string, std::set< std::string > >;

    Collection fetchSecurityDevices() const;
    Collection fetchHouseModes() const;
    std::string fetchCurrentModeId() const;
    Json fetchDevice( const std::string& deviceId ) const;
    Collection fetchSecurityItems() const;
    std::set< std::string > fetchArmedDevices();

private:
    bool tryMigrate();

    void addSecurityDevice( const std::string& deviceId, const std::string& securityThreatId );
    void removeFromDisarmedDevices( const std::vector< std::string >& deviceIds );
    static bool removeFromModeDisarmed( Json& mode, const std::vector< std::string >& deviceIds );
    static bool addToModeDisarmed( Json& mode, const std::vector< std::string >& deviceIds );
    void updateHouseModeRecord( const std::string& modeId, const Json& mode );
    void addToHouseModes( const std::string& deviceId, const Json& device );
    static CategoriesIndex makeCategoriesIndex( const Json& mode );
    void updateDeviceArmedField( const std::string& deviceId, bool armed );

private:
    IDbClient& m_dbClient;
    size_t m_recordsInserted{ 0 };
    size_t m_recordsDeleted{ 0 };
    size_t m_securityDevicesAdded{ 0 };
    size_t m_removedFromDisarmedDevices{ 0 };
    size_t m_insertedInModes{ 0 };
    size_t m_changedArmedToTrue{ 0 };
    size_t m_changedArmedToFalse{ 0 };
};

}  // namespace v1_37_securityDevices
}  // namespace MigrationTool
}  // namespace HubPlatform