
#include "service_interfaces/daemon_service.h"

#include "mqtt_gateway_tests/application.h"

#include <gtest/gtest.h>

int main( int argc, char** argv ) {
    using namespace HubPlatform;

    ::testing::InitGoogleTest( &argc, argv );

    return Services::DaemonService< Tests::MQTTGatewayTestsApplication >().run( argc, argv );
}
