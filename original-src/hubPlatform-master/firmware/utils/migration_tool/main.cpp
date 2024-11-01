#include "migration_tool/provisioning.h"
#include "migration_tool/system_repair/system_repair_manager.h"
#include "migration_tool/migration/migration_manager.h"
#include "migration_tool/data_resetter.h"

#include "log_utils/ezlo_log.h"
#include "provision_data_utils/provision_data_manager.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "tiny/tiny_command_line_parser.h"

#include <unistd.h>
#include <cstring>

namespace {
enum class Action { ResetData, ProvisionAndMigrate };
}

bool repairSystem( IDbClient& dbClient ) {
    HubPlatform::MigrationTool::SystemRepairManager repairManager( dbClient );
    return repairManager.repair();
}

bool provision() {
    using namespace HubPlatform::Provisioning;
    Provisioning provisioning;
    if( !provisioning.hasProvisionInformation() ) {
        Log::warn( "Need to update controller information" );
        return false;
    }
    HubPlatform::filesystem::createFolder( HubPlatform::Environment::getCertificateFolder() );

    bool ok = true;
    ok &= provisioning.setCloudCertificate();
    ok &= provisioning.setHubSslKeyPrivate();
    ok &= provisioning.setHubSslKeyPublic();
    ok &= provisioning.setZwaveRegion();
    ok &= provisioning.setRootPassword();
    ok &= provisioning.setOpkgTrustedKey();
    return ok;
}

bool migrate( IDbClient& dbClient ) {
    HubPlatform::MigrationTool::MigrationManager migrationManager(
        dbClient, HubPlatform::ProvisionDataUtils::getManager().getProvisionData() );
    return migrationManager.migrateDatabase();
}

bool resetData( IDbClient& dbClient ) {
    HubPlatform::MigrationTool::DataResetter dataResetter( dbClient );
    return dataResetter.resetData();
}

int main( int argc, char* argv[] ) {
    Action action = Action::ProvisionAndMigrate;
    tiny::command_line::parser parser;
    parser.on( "root", "Define firmware root folder", []( const std::string& rootPath ) {
        HubPlatform::Environment::setFirmwareRootFolder( rootPath );
        std::cout << "Root folder was changed: " << rootPath << std::endl;
    } );
    parser.on( "reset_data", "Reset data", [&action]() {
        action = Action::ResetData;
        ;
    } );
    parser.on( "help", "Show help info", [&parser]() {
        std::cout << parser.help_text() << "\n";
        ::_exit( 0 );
    } );

    parser.parse( argc, argv );

    bool ok = true;

    auto dbClient = createDbClient();

    switch( action ) {
    case Action::ProvisionAndMigrate:
        ok &= repairSystem( *dbClient );
        ok &= provision();
        ok &= migrate( *dbClient );
        break;
    case Action::ResetData:
        ok &= resetData( *dbClient );
        break;
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
