#include "service_interfaces/daemon_service.h"
#include "cored/core_service.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< CoreD::CoreService >().run( argc, argv );
}
