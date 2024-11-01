#include <gtest/gtest.h>
#include "json_utils/json_utils.h"
#include "cloud_devices/local_notifications/utils.h"
#include "cloud_devices/tests/fakes/fake_cloud_devices_service.h"
#include "cloud_devices/local_notifications/resources.h"

namespace HubPlatform {
namespace UT {

using namespace CloudDevices;

class LocalNotificationUtilsTest : public FakeCloudDevicesService {
protected:
    static void SetUpTestCase();
};

void LocalNotificationUtilsTest::SetUpTestCase() {
    FakeCloudDevicesService::SetUpTestCase();
    Log::setLocalLogLevel( Log::PriorityLevel::TRACE );
}

TEST_F( LocalNotificationUtilsTest, addAndGetCloudDataInfo ) {
    namespace Utils = CloudDevices::LocalNotification::Utils;
    using namespace LocalNotification::Resources::DB;

    const int tcdsDataVersionA = 1;
    const int tcdsFileVersionA = 2;
    const std::string tcdsUrlA = "tcds-url-value-A";
    const std::string tcdsS3IdA = "tcds-s3id-A";

    const int tcdsDataVersionB = 3;
    const int tcdsFileVersionB = 4;
    const std::string tcdsUrlB = "tcds-url-value-B";
    const std::string tcdsS3IdB = "tcds-s3id-B";

    Utils::addCloudDataInfo( getDBClient(), tcdsDataVersionA, tcdsFileVersionA, tcdsUrlA, tcdsS3IdA );
    Utils::addCloudDataInfo( getDBClient(), tcdsDataVersionB, tcdsFileVersionB, tcdsUrlB, tcdsS3IdB );

    {
        auto record = Utils::getCloudDataInfo( getDBClient(), tcdsDataVersionA, tcdsFileVersionA );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), tcdsDataVersionA );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), tcdsFileVersionA );
        EXPECT_EQ( json_get_str( record, "url" ), tcdsUrlA );
        EXPECT_EQ( json_get_str( record, "s3_id" ), tcdsS3IdA );
    }

    {
        auto record = Utils::getCloudDataInfo( getDBClient(), tcdsDataVersionB, tcdsFileVersionB );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), tcdsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), tcdsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), tcdsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), tcdsS3IdB );
    }
}

TEST_F( LocalNotificationUtilsTest, addAndGetMeshbotsDataInfo ) {
    namespace Utils = CloudDevices::LocalNotification::Utils;
    using namespace LocalNotification::Resources::DB;

    const int meshbotsDataVersionA = 10;
    const int meshbotsFileVersionA = 20;
    const std::string meshbotsUrlA = "meshbots-url-value-A";
    const std::string meshbotsS3IdA = "meshbots-s3id-A";

    const int meshbotsDataVersionB = 11;
    const int meshbotsFileVersionB = 21;
    const std::string meshbotsUrlB = "meshbots-url-value-B";
    const std::string meshbotsS3IdB = "meshbots-s3id-B";

    Utils::addMeshbotsDataInfo(
        getDBClient(), meshbotsDataVersionA, meshbotsFileVersionA, meshbotsUrlA, meshbotsS3IdA );
    Utils::addMeshbotsDataInfo(
        getDBClient(), meshbotsDataVersionB, meshbotsFileVersionB, meshbotsUrlB, meshbotsS3IdB );

    {
        auto record = Utils::getMeshbotsDataInfo( getDBClient(), meshbotsDataVersionA, meshbotsFileVersionA );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), meshbotsDataVersionA );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), meshbotsFileVersionA );
        EXPECT_EQ( json_get_str( record, "url" ), meshbotsUrlA );
        EXPECT_EQ( json_get_str( record, "s3_id" ), meshbotsS3IdA );
    }

    {
        auto record = Utils::getMeshbotsDataInfo( getDBClient(), meshbotsDataVersionB, meshbotsFileVersionB );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), meshbotsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), meshbotsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), meshbotsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), meshbotsS3IdB );
    }
}

TEST_F( LocalNotificationUtilsTest, maintainActiveCloudDataInfo ) {
    namespace Utils = CloudDevices::LocalNotification::Utils;
    using namespace LocalNotification::Resources::DB;

    const int tcdsDataVersionA = 1;
    const int tcdsFileVersionA = 2;
    const std::string tcdsUrlA = "tcds-url-value-A";
    const std::string tcdsS3IdA = "tcds-s3id-A";

    const int tcdsDataVersionB = 3;
    const int tcdsFileVersionB = 4;
    const std::string tcdsUrlB = "tcds-url-value-B";
    const std::string tcdsS3IdB = "tcds-s3id-B";

    Utils::addCloudDataInfo( getDBClient(), tcdsDataVersionA, tcdsFileVersionA, tcdsUrlA, tcdsS3IdA );
    Utils::addCloudDataInfo( getDBClient(), tcdsDataVersionB, tcdsFileVersionB, tcdsUrlB, tcdsS3IdB );

    EXPECT_TRUE( Utils::setActiveCloudDataVersion( getDBClient(), tcdsDataVersionA, tcdsFileVersionA ) );

    {
        auto record = Utils::getActiveCloudDataInfo( getDBClient() );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), tcdsDataVersionA );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), tcdsFileVersionA );
        EXPECT_EQ( json_get_str( record, "url" ), tcdsUrlA );
        EXPECT_EQ( json_get_str( record, "s3_id" ), tcdsS3IdA );
    }

    EXPECT_TRUE( Utils::setActiveCloudDataVersion( getDBClient(), tcdsDataVersionB, tcdsFileVersionB ) );

    {
        auto record = Utils::getActiveCloudDataInfo( getDBClient() );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), tcdsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), tcdsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), tcdsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), tcdsS3IdB );
    }

    Utils::removeUnusedCloudDataVersions( getDBClient() );

    {
        auto record = Utils::getCloudDataInfo( getDBClient(), tcdsDataVersionA, tcdsFileVersionA );
        EXPECT_TRUE( record.empty() );
    }

    {
        auto record = Utils::getActiveCloudDataInfo( getDBClient() );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), tcdsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), tcdsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), tcdsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), tcdsS3IdB );
    }

    {
        auto record = Utils::getCloudDataInfo( getDBClient(), tcdsDataVersionB, tcdsFileVersionB );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), tcdsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), tcdsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), tcdsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), tcdsS3IdB );
    }
}

TEST_F( LocalNotificationUtilsTest, maintainActiveMeshbotsDataInfo ) {
    namespace Utils = CloudDevices::LocalNotification::Utils;
    using namespace LocalNotification::Resources::DB;

    const int meshbotsDataVersionA = 1;
    const int meshbotsFileVersionA = 2;
    const std::string meshbotsUrlA = "meshbots-url-value-A";
    const std::string meshbotsS3IdA = "meshbots-s3id-A";

    const int meshbotsDataVersionB = 3;
    const int meshbotsFileVersionB = 4;
    const std::string meshbotsUrlB = "meshbots-url-value-B";
    const std::string meshbotsS3IdB = "meshbots-s3id-B";

    Utils::addMeshbotsDataInfo(
        getDBClient(), meshbotsDataVersionA, meshbotsFileVersionA, meshbotsUrlA, meshbotsS3IdA );
    Utils::addMeshbotsDataInfo(
        getDBClient(), meshbotsDataVersionB, meshbotsFileVersionB, meshbotsUrlB, meshbotsS3IdB );

    EXPECT_TRUE( Utils::setActiveMeshbotsDataVersion( getDBClient(), meshbotsDataVersionA, meshbotsFileVersionA ) );

    {
        auto record = Utils::getActiveMeshbotsDataInfo( getDBClient() );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), meshbotsDataVersionA );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), meshbotsFileVersionA );
        EXPECT_EQ( json_get_str( record, "url" ), meshbotsUrlA );
        EXPECT_EQ( json_get_str( record, "s3_id" ), meshbotsS3IdA );
    }

    EXPECT_TRUE( Utils::setActiveMeshbotsDataVersion( getDBClient(), meshbotsDataVersionB, meshbotsFileVersionB ) );

    {
        auto record = Utils::getActiveMeshbotsDataInfo( getDBClient() );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), meshbotsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), meshbotsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), meshbotsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), meshbotsS3IdB );
    }

    Utils::removeUnusedMeshbotsVersions( getDBClient() );

    {
        auto record = Utils::getMeshbotsDataInfo( getDBClient(), meshbotsDataVersionA, meshbotsFileVersionA );
        EXPECT_TRUE( record.empty() );
    }

    {
        auto record = Utils::getActiveMeshbotsDataInfo( getDBClient() );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), meshbotsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), meshbotsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), meshbotsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), meshbotsS3IdB );
    }

    {
        auto record = Utils::getMeshbotsDataInfo( getDBClient(), meshbotsDataVersionB, meshbotsFileVersionB );
        EXPECT_EQ( json_get_int( record, Fields::CurrentVersion ), meshbotsDataVersionB );
        EXPECT_EQ( json_get_int( record, Fields::FileVersion ), meshbotsFileVersionB );
        EXPECT_EQ( json_get_str( record, "url" ), meshbotsUrlB );
        EXPECT_EQ( json_get_str( record, "s3_id" ), meshbotsS3IdB );
    }
}

}  // namespace UT
}  // namespace HubPlatform
