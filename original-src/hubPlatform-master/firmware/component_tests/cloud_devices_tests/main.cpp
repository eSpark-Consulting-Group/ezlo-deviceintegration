
#include "service_interfaces/daemon_service.h"

#include "cloud_devices_tests/application.h"

#include <gtest/gtest.h>

int main( int argc, char** argv ) {
    using namespace HubPlatform;

    ::testing::InitGoogleTest( &argc, argv );

    return Services::DaemonService< Tests::CloudDevicesTestsApplication >().run( argc, argv );
}
