#include <gtest/gtest.h>
#include "json_utils/json_utils.h"
#include "cloud_devices/tests/fakes/fake_cloud_devices_service.h"
#include "cloud_devices/local_notifications/multiple_data_handler.h"

/*
TESTPLAN

DONE    1. Adding Record
DONE    2. Fetching record
DONE    3. Clearing record
*/

namespace HubPlatform {
namespace UT {

using namespace CloudDevices;

class MultipleDataHandlerTests : public FakeCloudDevicesService {
protected:
    static void SetUpTestCase();
};

void MultipleDataHandlerTests::SetUpTestCase() {
    FakeCloudDevicesService::SetUpTestCase();
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

TEST_F( MultipleDataHandlerTests, addingRecord ) {
    auto data = JSON( "version" << 1 << "url"
                                << "https://testurl.com"
                                << "s3_id"
                                << "testing_id_123" );

    CloudDataHandler cloudDataHandler;
    ASSERT_TRUE( cloudDataHandler.isQueueEmpty() );
    cloudDataHandler.addRecordToQueue( data );
    ASSERT_FALSE( cloudDataHandler.isQueueEmpty() );
}

TEST_F( MultipleDataHandlerTests, fetchLatestRecord ) {
    auto data1 = JSON( "version" << 1 << "url"
                                 << "https://testurl.com"
                                 << "s3_id"
                                 << "testing_id_123" );
    auto data2 = JSON( "version" << 2 << "url"
                                 << "https://testurl.com"
                                 << "s3_id"
                                 << "testing_id_123" );

    CloudDataHandler cloudDataHandler;
    cloudDataHandler.addRecordToQueue( data1 );
    cloudDataHandler.addRecordToQueue( data2 );
    ASSERT_FALSE( cloudDataHandler.isQueueEmpty() );
    EXPECT_EQ( cloudDataHandler.fetchLatestRecordFromQueue(), data2 );
}

TEST_F( MultipleDataHandlerTests, clearQueue ) {
    auto data1 = JSON( "version" << 1 << "url"
                                 << "https://testurl.com"
                                 << "s3_id"
                                 << "testing_id_123" );
    auto data2 = JSON( "version" << 2 << "url"
                                 << "https://testurl.com"
                                 << "s3_id"
                                 << "testing_id_123" );

    CloudDataHandler cloudDataHandler;
    cloudDataHandler.addRecordToQueue( data1 );
    cloudDataHandler.addRecordToQueue( data2 );
    ASSERT_FALSE( cloudDataHandler.isQueueEmpty() );
    cloudDataHandler.clearQueue();
    ASSERT_TRUE( cloudDataHandler.isQueueEmpty() );
}

}  // namespace UT
}  // namespace HubPlatform