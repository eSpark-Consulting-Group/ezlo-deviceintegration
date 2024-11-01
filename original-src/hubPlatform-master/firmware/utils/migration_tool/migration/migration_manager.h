#pragma once

#include "migration_tool/migration/migration.h"
#include "migration_tool/migration/data_version.h"
#include "database_client/idbclient.h"
#include "provision_data_utils/provision_data.h"

#include <map>
#include <memory>

namespace HubPlatform {
namespace MigrationTool {

class MigrationManager {
public:
    MigrationManager( IDbClient& dbClient, const ProvisionDataUtils::ProvisionData& provisionData );

    bool migrateDatabase();

private:
    DataFormatVersion detectExistingDataFormatVersion() const;
    DataFormatVersion getCurrentDataFormatVersion() const;
    bool databaseIsEmpty() const;

    bool setCurrentDataFormatVersion( const DataFormatVersion& version );

    template < typename MigrationType, typename... MigrationArgs >
    void registerMigration( DataFormatVersion::ComponentType major,
                            DataFormatVersion::ComponentType minor,
                            MigrationArgs&&... migrationArgs );

private:
    IDbClient& m_dbClient;
    const ProvisionDataUtils::ProvisionData& m_provisionData;
    std::map< DataFormatVersion, std::unique_ptr< Migration > > m_migrations;
};

}  // namespace MigrationTool
}  // namespace HubPlatform
