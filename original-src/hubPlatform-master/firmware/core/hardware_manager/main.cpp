#include "service_interfaces/daemon_service.h"
#include "hardware_manager/hm_service.h"
#include "base_utils/i2ccontrol.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< HardwareManager::Service >().run( argc, argv );
}
