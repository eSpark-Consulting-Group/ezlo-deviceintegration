#include "extensions_manager/em_service.h"

#include "service_interfaces/daemon_service.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< ExtensionsManager::Service >().run( argc, argv );
}
