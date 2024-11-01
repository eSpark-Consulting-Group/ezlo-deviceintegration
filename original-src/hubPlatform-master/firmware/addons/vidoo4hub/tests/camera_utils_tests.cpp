#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "vidoo4hub/camera/vidoo_camera.h"
#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/stream_manager/stream_manager.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"

#include "vidoo4hub/tests/fakes/fake_vidoo4hub_service.h"
#include "vidoo4hub/tests/fakes/fake_cored_remote_interface.h"
#include "vidoo4hub/tests/utils/test_utils.h"

#include "database_client/dbqueryparser.h"

#include <chrono>

/*

TESTPLAN

DONE    1. Camera tests
DONE        1.1 Camera operator ==
DONE        1.2 Camera subcategories

DONE    2. Json tests
DONE        2.1 Camera to json
DONE        2.2 Camera from json

DONE    3. Database tests
DONE        3.1 Store camera to DB
DONE        3.2 Store multiple cameras to DB
DONE        3.3 Read camera from DB
DONE        3.4 Read nonexising camera from DB
DONE        3.5 Read all cameras from DB
DONE        3.6 Delete multiple cameras from DB

DONE    4. CoreD tests
DONE        4.1 Add camera device
DONE        4.2 Add motion sensor device
DONE        4.3 Add security threat item
DONE        4.4 Add motion item
DONE        4.5 Add hotzones item
DONE        4.6 Add snapshot item
DONE        4.7 Add make recording item
DONE        4.8 Add stop recording item
DONE        4.9 Add stream item
DONE        4.10 Remove device
DONE        4.11 Remove item

DONE    5. Stream tests
DONE        5.1 Add camera stream
DONE        5.2 Remove camera stream
DONE        5.3 Create live RTSP stream
DONE        5.4 Delete live RTSP stream

*/

namespace HubPlatform {
namespace UT {

using namespace Vidoo4Hub;

class CameraUtilsTestSuite : public FakeVidoo4HubService {};

TEST_F( CameraUtilsTestSuite, cameraEquality ) /* 1.1 */ {
    auto camera1 = getDummyCamera( "12345" );

    VidooCamera camera2;
    EXPECT_NE( camera2, camera1 );

    camera2.cameraId = camera1.cameraId;
    EXPECT_NE( camera2, camera1 );

    camera2.cameraSubcategory = camera1.cameraSubcategory;
    EXPECT_NE( camera2, camera1 );

    camera2.cameraName = camera1.cameraName;
    EXPECT_NE( camera2, camera1 );

    camera2.cameraDeviceId = camera1.cameraDeviceId;
    EXPECT_NE( camera2, camera1 );

    camera2.sensorDeviceId = camera1.sensorDeviceId;
    EXPECT_NE( camera2, camera1 );

    camera2.securityThreatItemId = camera1.securityThreatItemId;
    EXPECT_NE( camera2, camera1 );

    camera2.motionItemId = camera1.motionItemId;
    EXPECT_NE( camera2, camera1 );

    camera2.hotzonesItemId = camera1.hotzonesItemId;
    EXPECT_NE( camera2, camera1 );

    camera2.snapshotItemId = camera1.snapshotItemId;
    EXPECT_NE( camera2, camera1 );

    camera2.makeRecordingItemId = camera1.makeRecordingItemId;
    EXPECT_NE( camera2, camera1 );

    camera2.stopRecordingItemId = camera1.stopRecordingItemId;
    EXPECT_NE( camera2, camera1 );

    camera2.rtspURL = camera1.rtspURL;
    EXPECT_NE( camera2, camera1 );

    camera2.streamId = camera1.streamId;
    EXPECT_EQ( camera2, camera1 );
}

TEST_F( CameraUtilsTestSuite, cameraSubcategories ) /* 1.2 */ {
    using namespace Vidoo4Hub::Resources::CameraSubcategory;
    EXPECT_TRUE( CameraUtils::isKnownCameraSubcategory( IndoorCam ) );
    EXPECT_TRUE( CameraUtils::isKnownCameraSubcategory( OutdoorCam ) );
    EXPECT_TRUE( CameraUtils::isKnownCameraSubcategory( DoorbellCam ) );

    EXPECT_FALSE( CameraUtils::isKnownCameraSubcategory( "unknown" ) );
    EXPECT_FALSE( CameraUtils::isKnownCameraSubcategory( "" ) );
}

TEST_F( CameraUtilsTestSuite, cameraToJson ) /* 2.1 */ {
    using namespace Vidoo4Hub::Resources::DB;

    auto camera = getDummyCamera( "12345" );
    auto j = CameraUtils::cameraToJson( camera );

    EXPECT_EQ( camera.cameraId, json_get_str( j, FieldNames::cameraId ) );
    EXPECT_EQ( camera.cameraSubcategory, json_get_str( j, FieldNames::cameraSubcategory ) );
    EXPECT_EQ( camera.cameraName, json_get_str( j, FieldNames::cameraName ) );
    EXPECT_EQ( camera.cameraDeviceId, json_get_str( j, FieldNames::cameraDeviceId ) );
    EXPECT_EQ( camera.sensorDeviceId, json_get_str( j, FieldNames::sensorDeviceId ) );
    EXPECT_EQ( camera.securityThreatItemId, json_get_str( j, FieldNames::securityThreatItemId ) );
    EXPECT_EQ( camera.motionItemId, json_get_str( j, FieldNames::motionItemId ) );
    EXPECT_EQ( camera.hotzonesItemId, json_get_str( j, FieldNames::hotzonesItemId ) );
    EXPECT_EQ( camera.snapshotItemId, json_get_str( j, FieldNames::snapshotItemId ) );
    EXPECT_EQ( camera.makeRecordingItemId, json_get_str( j, FieldNames::makeRecordingItemId ) );
    EXPECT_EQ( camera.stopRecordingItemId, json_get_str( j, FieldNames::stopRecordingItemId ) );
    EXPECT_EQ( camera.rtspURL, json_get_str( j, FieldNames::rtspURL ) );
    EXPECT_EQ( camera.streamId, json_get_str( j, FieldNames::streamId ) );
}

TEST_F( CameraUtilsTestSuite, cameraFromJson ) /* 2.2 */ {
    VidooCamera camera = getDummyCamera( "12345" );
    auto cameraJson = CameraUtils::cameraToJson( camera );

    auto newCamera = CameraUtils::cameraFromJson( cameraJson );
    EXPECT_EQ( newCamera, camera );

    auto newJson = CameraUtils::cameraToJson( newCamera );
    EXPECT_EQ( newJson.dump(), cameraJson.dump() );
}

TEST_F( CameraUtilsTestSuite, StoreCameraToDb ) /* 3.1 */ {
    using namespace Vidoo4Hub::Resources::DB;
    auto& dbClient = getHostService().getDBClient();

    const std::string cameraId = "12345";
    VidooCamera camera = getDummyCamera( cameraId );

    CameraUtils::storeCameraToDB( dbClient, camera );

    auto results = dbClient.findAll( TableNames::VidooCamerasDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( results.size(), 1 );
    EXPECT_EQ( json_get_str( results.front(), db::ID_FIELD_NAME ), cameraId );
};

TEST_F( CameraUtilsTestSuite, StoreMultipleCamerasToDb ) /* 3.2 */ {
    using namespace Vidoo4Hub::Resources::DB;
    auto& dbClient = getHostService().getDBClient();

    const std::string cameraId1 = "12345";
    const std::string cameraId2 = "ABCDE";
    VidooCamera camera1 = getDummyCamera( cameraId1 );
    VidooCamera camera2 = getDummyCamera( cameraId2 );

    CameraUtils::storeCameraToDB( dbClient, camera1 );
    CameraUtils::storeCameraToDB( dbClient, camera2 );

    auto results = dbClient.findAll( TableNames::VidooCamerasDBName, JSON_EMPTY_OBJ, nullptr );
    ASSERT_EQ( results.size(), 2 );

    std::set< std::string > storedIds;
    storedIds.emplace( json_get_str( results.front(), db::ID_FIELD_NAME ) );
    storedIds.emplace( json_get_str( results.back(), db::ID_FIELD_NAME ) );

    EXPECT_EQ( storedIds, std::set< std::string >( { cameraId1, cameraId2 } ) );
};

TEST_F( CameraUtilsTestSuite, ReadCameraFromDb ) /* 3.3 */ {
    auto& dbClient = getHostService().getDBClient();

    const std::string cameraId = "12345";
    VidooCamera camera = getDummyCamera( cameraId );

    CameraUtils::storeCameraToDB( dbClient, camera );
    auto cameraPtr = CameraUtils::readCameraFromDB( dbClient, cameraId );

    ASSERT_TRUE( cameraPtr );
    EXPECT_EQ( *cameraPtr, camera );
};

TEST_F( CameraUtilsTestSuite, ReadNonexisingCameraFromDb ) /* 3.4 */ {
    auto& dbClient = getHostService().getDBClient();
    auto cameraPtr = CameraUtils::readCameraFromDB( dbClient, "nonExistingCameraId" );
    ASSERT_FALSE( cameraPtr );
};

TEST_F( CameraUtilsTestSuite, ReadAllCamerasFromDb ) /* 3.5 */ {
    auto& dbClient = getHostService().getDBClient();

    const std::string cameraId1 = "12345";
    const std::string cameraId2 = "ABCDE";

    std::map< std::string, VidooCamera > cameraList;
    cameraList.emplace( cameraId1, getDummyCamera( cameraId1 ) );
    cameraList.emplace( cameraId2, getDummyCamera( cameraId2 ) );

    for( auto& camera : cameraList )
        CameraUtils::storeCameraToDB( dbClient, camera.second );

    auto camerasFromDb = CameraUtils::readAllCamerasFromDB( dbClient );
    ASSERT_EQ( camerasFromDb, cameraList );
};

TEST_F( CameraUtilsTestSuite, DeleteMultipleCamerasFromDb ) /* 3.6 */ {
    auto& dbClient = getHostService().getDBClient();

    const std::string cameraId1 = "12345";
    const std::string cameraId2 = "ABCDE";
    VidooCamera camera1 = getDummyCamera( cameraId1 );
    VidooCamera camera2 = getDummyCamera( cameraId2 );
    CameraUtils::storeCameraToDB( dbClient, camera1 );
    CameraUtils::storeCameraToDB( dbClient, camera2 );
    ASSERT_EQ( CameraUtils::readAllCamerasFromDB( dbClient ).size(), 2 );

    CameraUtils::deleteCameraFromDB( dbClient, cameraId1 );
    ASSERT_EQ( CameraUtils::readAllCamerasFromDB( dbClient ).size(), 1 );
    ASSERT_TRUE( CameraUtils::readCameraFromDB( dbClient, cameraId2 ) ) << "camera 2 should remain";

    EXPECT_NO_THROW( CameraUtils::deleteCameraFromDB( dbClient, cameraId1 ) ) << "delete same camera";
    ASSERT_EQ( CameraUtils::readAllCamerasFromDB( dbClient ).size(), 1 );

    CameraUtils::deleteCameraFromDB( dbClient, cameraId2 );
    ASSERT_EQ( CameraUtils::readAllCamerasFromDB( dbClient ).size(), 0 );

    EXPECT_NO_THROW( CameraUtils::deleteCameraFromDB( dbClient, "unknownCameraId" ) );
};

TEST_F( CameraUtilsTestSuite, AddCameraDevice ) /* 4.1 */ {
    FakeCoreDRemoteInterface core( getHostService() );

    const std::string cameraId = "12345";
    const std::string cameraSubcategory = Vidoo4Hub::Resources::CameraSubcategory::DoorbellCam;
    const std::string cameraName = "camera";
    const std::string gatewayId = "gateway";

    auto deviceId = CameraUtils::createCameraDevice( core, gatewayId, cameraSubcategory, cameraName );
    EXPECT_TRUE( !deviceId.empty() );
    EXPECT_EQ( core.m_devices.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddMotionSensorDevice ) /* 4.2 */ {
    FakeCoreDRemoteInterface core( getHostService() );

    const std::string cameraId = "12345";
    const std::string cameraDeviceId = getUniqueID();
    const std::string cameraName = "camera";
    const std::string gatewayId = "gateway";

    auto deviceId = CameraUtils::createMotionSensorDevice( core, gatewayId, cameraDeviceId, cameraName );
    EXPECT_TRUE( !deviceId.empty() );
    EXPECT_EQ( core.m_devices.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddSecurityThreatItem ) /* 4.3 */ {
    FakeCoreDRemoteInterface core( getHostService() );
    const std::string cameraDeviceId = getUniqueID();

    auto itemId = CameraUtils::createSecurityThreatItem( core, cameraDeviceId );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddMotionItem ) /* 4.4 */ {
    FakeCoreDRemoteInterface core( getHostService() );
    const std::string cameraDeviceId = getUniqueID();

    auto itemId = CameraUtils::createMotionItem( core, cameraDeviceId );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddHotzonesItem ) /* 4.5 */ {
    FakeCoreDRemoteInterface core( getHostService() );
    const std::string cameraDeviceId = getUniqueID();

    auto itemId = CameraUtils::createHotzonesItem( core, cameraDeviceId );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddSnapshotItem ) /* 4.6 */ {
    FakeCoreDRemoteInterface core( getHostService() );
    const std::string cameraDeviceId = getUniqueID();

    auto itemId = CameraUtils::createSnapshotItem( core, cameraDeviceId );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddMakeRecordingItem ) /* 4.7 */ {
    FakeCoreDRemoteInterface core( getHostService() );
    const std::string cameraDeviceId = getUniqueID();

    auto itemId = CameraUtils::createMakeRecordingItem( core, cameraDeviceId );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddStopRecordingItem ) /* 4.8 */ {
    FakeCoreDRemoteInterface core( getHostService() );
    const std::string cameraDeviceId = getUniqueID();

    auto itemId = CameraUtils::createStopRecordingItem( core, cameraDeviceId );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, AddStreamItem ) /* 4.9 */ {
    FakeCoreDRemoteInterface core( getHostService() );

    const std::string deviceId = "deviceId";
    const std::string streamName = "stream_name";

    auto itemId = CameraUtils::createStreamItem( core, deviceId, streamName );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );
};

TEST_F( CameraUtilsTestSuite, RemoveDevice ) /* 4.10 */ {
    FakeCoreDRemoteInterface core( getHostService() );

    auto deviceId = CameraUtils::createCameraDevice( core, "gatewayId", "subcategory", "deviceName" );
    ASSERT_EQ( core.m_devices.size(), 1 );

    CameraUtils::removeDevice( core, deviceId );
    EXPECT_TRUE( core.m_devices.empty() );
};

TEST_F( CameraUtilsTestSuite, RemoveItem ) /* 4.11 */ {
    FakeCoreDRemoteInterface core( getHostService() );

    auto itemId = CameraUtils::createMotionItem( core, "deviceId" );
    EXPECT_TRUE( !itemId.empty() );
    EXPECT_EQ( core.m_items.size(), 1 );

    CameraUtils::removeItem( core, itemId );
    EXPECT_TRUE( core.m_items.empty() );
};

TEST_F( CameraUtilsTestSuite, AddCameraStream ) /* 5.1 */ {
    StreamManager streamManager( getHostService(), std::chrono::seconds( 0 ) );

    const std::string streamItemId = "itemId";
    const std::string streamName = "streamName";
    const std::string rtspURL = "rtspURL";

    EXPECT_NO_THROW( streamManager.addCameraStream( streamItemId, streamName, rtspURL ) );
};

TEST_F( CameraUtilsTestSuite, RemoveCameraStream ) /* 5.2 */ {
    StreamManager streamManager( getHostService(), std::chrono::seconds( 0 ) );
    EXPECT_NO_THROW( streamManager.removeCameraStream( "streamItemId" ) );
};

TEST_F( CameraUtilsTestSuite, CreateLiveRTSPStream ) /* 5.3 */ {
    StreamManager streamManager( getHostService(), std::chrono::seconds( 0 ) );
    EXPECT_NO_THROW( streamManager.createLiveRTSPStream( "streamItemId" ) );
};

TEST_F( CameraUtilsTestSuite, DeleteLiveRTSPStream ) /* 5.4 */ {
    StreamManager streamManager( getHostService(), std::chrono::seconds( 0 ) );
    EXPECT_NO_THROW( streamManager.deleteLiveRTSPStream( "streamItemId" ) );
};

}  // namespace UT
}  // namespace HubPlatform
