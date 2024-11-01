#include "external_controllers/ec_service.h"

#include "service_interfaces/daemon_service.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< ExternalControllers::Service >().run( argc, argv );
}
