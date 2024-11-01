#include "service_interfaces/daemon_service.h"
#include "backupd/backup_service.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< BackupUpdateManager::Service >().run( argc, argv );
}
