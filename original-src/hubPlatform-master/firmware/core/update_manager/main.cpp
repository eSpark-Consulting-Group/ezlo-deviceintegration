#include "service_interfaces/daemon_service.h"
#include "update_manager/um_service.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< UpdateManager::Service >().run( argc, argv );
}
