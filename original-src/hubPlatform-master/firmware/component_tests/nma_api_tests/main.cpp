
#include "service_interfaces/daemon_service.h"

#include "nma_api_tests/application.h"

#include <gtest/gtest.h>

int main( int argc, char** argv ) {
    using namespace HubPlatform;

    ::testing::InitGoogleTest( &argc, argv );

    return Services::DaemonService< Tests::NmaApiTestsApplication >().run( argc, argv );
}
