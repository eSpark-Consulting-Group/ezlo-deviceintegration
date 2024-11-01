#include "networkd/communication_test/communication_manual_test.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Success case
DONE    2. Failed case

*/

namespace HubPlatform {
namespace Network {

namespace UT {
using namespace Network::CommunicationTest;
class CommunicationTestTestSuite : public ::testing::Test {
public:
    CommunicationTestTestSuite() = default;
};

TEST_F( CommunicationTestTestSuite, success ) {
    int status = 1;
    int complete = 1;
    bool result = ManualCommunicationTest::isCloudControllerSignalingGood( status, complete );
    EXPECT_TRUE( result );
}

TEST_F( CommunicationTestTestSuite, failed ) {
    int status = 0;
    int complete = 1;
    bool result = ManualCommunicationTest::isCloudControllerSignalingGood( status, complete );

    EXPECT_FALSE( result );
    status = 1;
    complete = 0;
    result = ManualCommunicationTest::isCloudControllerSignalingGood( status, complete );
    EXPECT_FALSE( result );

    status = 0;
    complete = 0;
    result = ManualCommunicationTest::isCloudControllerSignalingGood( status, complete );
    EXPECT_FALSE( result );
}

TEST_F( CommunicationTestTestSuite, testBroadcast ) {
    const std::string operationId = "operationId";
    const std::string status = "status";
    const int completed = 100;
    Json error = JSON( "error"
                       << "none" );

    Json result = ManualCommunicationTest::createBroadcastPayload( operationId, status, completed, error );
    EXPECT_EQ( json_get_str( result, "operationId" ), operationId );
    EXPECT_EQ( json_get_str( result, "status" ), status );
    EXPECT_EQ( json_get_int( result, "completed" ), completed );

    Json errorObject;

    EXPECT_TRUE( json_get( result, "error", errorObject ) );
    EXPECT_EQ( errorObject.dump(), error.dump() );
}

}  // namespace UT
}  // namespace Network
}  // namespace HubPlatform
