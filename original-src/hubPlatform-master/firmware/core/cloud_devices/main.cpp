#include "service_interfaces/daemon_service.h"
#include "cloud_devices/cloud_devices_service.h"

int main( int argc, char* argv[] ) {
    using namespace HubPlatform;
    return Services::DaemonService< CloudDevices::CloudDevicesService >().run( argc, argv );
}
