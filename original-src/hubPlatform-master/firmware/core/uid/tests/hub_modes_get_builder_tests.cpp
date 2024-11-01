#include "uid/response_builders/hub_modes_get/response_builder_v3.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "database_client/idbclient.h"
#include <gtest/gtest.h>
#include "base_utils/timeutils.h"

namespace HubPlatform {
namespace Uid {
namespace UT {

/*

TESTPLAN

DONE    1. Get system test info builder check output

*/

using namespace ::testing;

class GetSystemTestInfoBuilderTestSuite : public Test {
public:
    Services::UT::FakeService m_fakeService;
};

TEST_F( GetSystemTestInfoBuilderTestSuite, checkOutput ) {
    const std::string Type = "walk";
    const std::string OperationId = "operationId";
    const int StartedAt = TimeUtils::currentSystemTime().count();
    const int Duration = 900;

    auto& db = m_fakeService.getDBClient();
    Json info =
        JSON( "type" << Type << "operationId" << OperationId << "duration" << Duration << "startedAt" << StartedAt );
    db.insert( TempSystemWalkTestProgress, info );
    auto systemTestInfo = Uid::ResponseBuilder::HubModesGet::V3::ResponseBuilder::getSystemTestInfo( db );

    EXPECT_EQ( json_get_int( systemTestInfo, "duration" ), 900 );
    EXPECT_EQ( json_get_int( systemTestInfo, "startedAt" ), StartedAt );
    EXPECT_EQ( json_get_int( systemTestInfo, "timesLeft" ), 900 );
    EXPECT_EQ( json_get_str( systemTestInfo, "type" ), Type );
    EXPECT_EQ( json_get_str( systemTestInfo, "operationId" ), OperationId );
}

}  // namespace UT
}  // namespace Uid
}  // namespace HubPlatform
