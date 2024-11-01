#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "vidoo4hub/camera/vidoo_camera.h"
#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/stream_manager/stream_manager.h"
#include "vidoo4hub/camera_manager/camera_manager.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "cored/model/resources/cored_resources.h"

#include "vidoo4hub/tests/fakes/fake_vidoo4hub_service.h"
#include "vidoo4hub/tests/fakes/fake_cored_remote_interface.h"
#include "vidoo4hub/tests/fakes/fake_core_manager.h"
#include "vidoo4hub/tests/fakes/fake_stream_manager.h"
#include "vidoo4hub/tests/mocks/stream_manager_mock.h"
#include "vidoo4hub/tests/utils/test_utils.h"

#include <chrono>

/*

TESTPLAN

DONE    1. CameraManager tests
DONE        1.1 Load cameras on creation
DONE        1.2 Add camera
DONE        1.3 Delete camera
DONE        1.4 Update camera list
DONE        1.5 Delete all cameras
DONE        1.6 Clean camera item values
DONE        1.7 Handle camera stream events
DONE    2. Item cache tests
DONE        2.1 Items cache

*/

namespace HubPlatform {
namespace UT {

using namespace Vidoo4Hub;

class CameraManagerTestSuite : public FakeVidoo4HubService {
protected:
    std::map< std::string, VidooCamera >& getCameraList( CameraManager& manager ) {
        return manager.m_cameraList;
    }

    std::set< std::string > getCameraIdList( CameraManager& manager ) {
        std::set< std::string > cameraIdList;
        for( auto& camera : manager.m_cameraList )
            cameraIdList.emplace( camera.first );
        return cameraIdList;
    }

    void checkCameraCache( const CameraItemsCache& cache, const VidooCamera& camera, int line ) {
        using ItemType = CameraItemsCache::ItemType;

        const auto cameraId = camera.cameraId;
        std::string lineStr = std::to_string( line );

        auto checkItem = [&]( const CameraItemsCache& cache, std::string itemId, CameraItemsCache::ItemType itemType ) {
            const std::string message = "CameraId: " + cameraId + ", ItemId: " + itemId + ", line: " + lineStr;

            auto itemCache = cache.findItem( itemId );
            ASSERT_TRUE( itemCache != nullptr ) << message;
            EXPECT_EQ( itemCache->first, itemId ) << message;

            auto& data = itemCache->second;
            EXPECT_EQ( data.m_cameraId, cameraId ) << message;
            EXPECT_EQ( data.m_itemType, itemType ) << message;
        };

        checkItem( cache, camera.securityThreatItemId, ItemType::SecurityThreat );
        checkItem( cache, camera.motionItemId, ItemType::Motion );
        checkItem( cache, camera.hotzonesItemId, ItemType::Hotzones );
        checkItem( cache, camera.snapshotItemId, ItemType::Snapshot );
        checkItem( cache, camera.makeRecordingItemId, ItemType::MakeRecording );
        checkItem( cache, camera.stopRecordingItemId, ItemType::StopRecording );
        checkItem( cache, camera.streamId, ItemType::CameraStream );
    }

    void checkCameraCache( CameraManager& manager, const VidooCamera& camera, int line ) {
        auto& cache = manager.getCameraItemsCache();
        checkCameraCache( cache, camera, line );
    }

    void checkCameraCache( CameraManager& manager, const std::string& cameraId, int line ) {
        auto& cache = manager.getCameraItemsCache();
        const auto& camera = manager.m_cameraList.at( cameraId );
        checkCameraCache( cache, camera, line );
    }

    const size_t m_itemsPerCamera = 7;
};

TEST_F( CameraManagerTestSuite, LoadCamerasOnCreation ) /* 1.1 */ {
    auto& dbClient = getHostService().getDBClient();
    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    FakeStreamManager streamManager;
    const std::string vidooDeviceId = "vidooDeviceId";
    const std::string gatewayId = "gatewayId";

    const std::string cameraId1 = "12345";
    const std::string cameraId2 = "ABCDE";
    VidooCamera camera1 = getDummyCamera( cameraId1 );
    VidooCamera camera2 = getDummyCamera( cameraId2 );

    CameraUtils::storeCameraToDB( dbClient, camera1 );
    CameraUtils::storeCameraToDB( dbClient, camera2 );

    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );
    auto cameraIdList = getCameraIdList( manager );

    EXPECT_EQ( cameraIdList, std::set< std::string >( { cameraId1, cameraId2 } ) );
    checkCameraCache( manager, camera1, __LINE__ );
    checkCameraCache( manager, camera2, __LINE__ );
};

TEST_F( CameraManagerTestSuite, AddCamera ) /* 1.2 */ {
    auto dummyCamera1 = getDummyCamera( "id_1" );
    auto dummyCamera2 = getDummyCamera( "id_2" );

    StreamManagerMock streamManager;
    EXPECT_CALL( streamManager, addCameraStream( testing::_, testing::_, dummyCamera1.rtspURL ) ).Times( 1 );
    EXPECT_CALL( streamManager, addCameraStream( testing::_, testing::_, dummyCamera2.rtspURL ) ).Times( 1 );
    EXPECT_CALL( streamManager, createLiveRTSPStream( testing::_ ) ).Times( 2 );

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";
    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    manager.addCamera(
        dummyCamera1.cameraId, dummyCamera1.cameraSubcategory, dummyCamera1.cameraName, dummyCamera1.rtspURL );
    manager.addCamera(
        dummyCamera2.cameraId, dummyCamera2.cameraSubcategory, dummyCamera2.cameraName, dummyCamera2.rtspURL );

    const auto cameraList = getCameraList( manager );
    ASSERT_EQ( cameraList.size(), 2 );
    EXPECT_EQ( CameraUtils::readAllCamerasFromDB( getDBClient() ).size(), 2 );

    auto camera1 = cameraList.at( "id_1" );
    auto camera2 = cameraList.at( "id_2" );

    checkCameraCache( manager, camera1, __LINE__ );
    checkCameraCache( manager, camera2, __LINE__ );

    auto checkCameraResources = [&]( const VidooCamera& camera, const VidooCamera& dummyCamera ) {
        std::string cameraIdString = "camera_id: " + camera.cameraId;

        EXPECT_EQ( camera.cameraId, dummyCamera.cameraId ) << cameraIdString;
        EXPECT_EQ( camera.cameraSubcategory, dummyCamera.cameraSubcategory ) << cameraIdString;
        EXPECT_EQ( camera.cameraName, dummyCamera.cameraName ) << cameraIdString;
        EXPECT_EQ( camera.rtspURL, dummyCamera.rtspURL ) << cameraIdString;

        EXPECT_TRUE( !camera.cameraDeviceId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_devices.count( camera.cameraDeviceId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.sensorDeviceId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_devices.count( camera.sensorDeviceId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.securityThreatItemId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.securityThreatItemId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.motionItemId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.motionItemId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.hotzonesItemId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.hotzonesItemId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.snapshotItemId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.snapshotItemId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.makeRecordingItemId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.makeRecordingItemId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.stopRecordingItemId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.stopRecordingItemId ) ) << cameraIdString;

        EXPECT_TRUE( !camera.streamId.empty() ) << cameraIdString;
        EXPECT_TRUE( core.m_items.count( camera.streamId ) ) << cameraIdString;
    };

    checkCameraResources( camera1, dummyCamera1 );
    checkCameraResources( camera2, dummyCamera2 );
};

TEST_F( CameraManagerTestSuite, DeleteCamera ) /* 1.3 */ {
    auto dummyCamera1 = getDummyCamera( "id_1" );
    auto dummyCamera2 = getDummyCamera( "id_2" );

    StreamManagerMock streamManager;
    EXPECT_CALL( streamManager, addCameraStream( testing::_, testing::_, testing::_ ) ).Times( 2 );
    EXPECT_CALL( streamManager, createLiveRTSPStream( testing::_ ) ).Times( 2 );
    EXPECT_CALL( streamManager, removeCameraStream( testing::_ ) ).Times( 1 );
    EXPECT_CALL( streamManager, deleteLiveRTSPStream( testing::_ ) ).Times( 1 );

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";
    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    manager.addCamera(
        dummyCamera1.cameraId, dummyCamera1.cameraSubcategory, dummyCamera1.cameraName, dummyCamera1.rtspURL );
    manager.addCamera(
        dummyCamera2.cameraId, dummyCamera2.cameraSubcategory, dummyCamera2.cameraName, dummyCamera2.rtspURL );
    ASSERT_EQ( getCameraList( manager ).size(), 2 );

    checkCameraCache( manager, dummyCamera1.cameraId, __LINE__ );
    checkCameraCache( manager, dummyCamera2.cameraId, __LINE__ );

    manager.deleteCamera( "id_2" );

    auto dbCameras = CameraUtils::readAllCamerasFromDB( getDBClient() );
    ASSERT_EQ( dbCameras.size(), 1 );

    const auto cameraList = getCameraList( manager );
    ASSERT_EQ( cameraList.size(), 1 );

    auto camera1 = cameraList.begin()->second;
    EXPECT_EQ( camera1.cameraId, "id_1" );

    checkCameraCache( manager, camera1, __LINE__ );
    EXPECT_EQ( manager.getCameraItemsCache().size(), m_itemsPerCamera ) << "Cache for 1 camera";

    std::set< std::string > devices = { camera1.cameraDeviceId, camera1.sensorDeviceId };
    std::set< std::string > items = {
        camera1.securityThreatItemId, camera1.motionItemId,        camera1.hotzonesItemId, camera1.snapshotItemId,
        camera1.makeRecordingItemId,  camera1.stopRecordingItemId, camera1.streamId
    };

    EXPECT_EQ( core.m_devices.size(), 2 );
    EXPECT_TRUE( core.m_devices.count( camera1.cameraDeviceId ) );
    EXPECT_TRUE( core.m_devices.count( camera1.sensorDeviceId ) );
    EXPECT_EQ( items.size(), core.m_items.size() );
    for( auto& item : items )
        EXPECT_TRUE( core.m_items.count( item ) ) << "ItemId: " << item;
};

TEST_F( CameraManagerTestSuite, UpdateCameraList ) /* 1.4 */ {
    using namespace Vidoo4Hub::Resources::DB;

    auto cameraA = getDummyCamera( "id_A" );
    auto cameraB = getDummyCamera( "id_B" );
    auto cameraC = getDummyCamera( "id_C" );
    auto cameraD = getDummyCamera( "id_D" );

    FakeStreamManager streamManager;
    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";
    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    manager.addCamera( cameraA.cameraId, cameraA.cameraSubcategory, cameraA.cameraName, cameraA.rtspURL );
    manager.addCamera( cameraB.cameraId, cameraB.cameraSubcategory, cameraB.cameraName, cameraB.rtspURL );
    manager.addCamera( cameraC.cameraId, cameraC.cameraSubcategory, cameraC.cameraName, cameraC.rtspURL );
    ASSERT_EQ( getCameraList( manager ).size(), 3 );

    std::vector< Json > updateList;
    updateList.reserve( 4 );

    const std::string changedName = "changed_name";
    const std::string changedRTSP = "changed rtsp";
    updateList.emplace_back( JSON( FieldNames::cameraId << cameraA.cameraId << FieldNames::cameraName
                                                        << cameraA.cameraName << FieldNames::cameraSubcategory
                                                        << cameraA.cameraSubcategory << FieldNames::rtspURL
                                                        << cameraA.rtspURL ) );
    updateList.emplace_back( JSON( FieldNames::cameraId << cameraB.cameraId << FieldNames::cameraName << changedName
                                                        << FieldNames::cameraSubcategory << cameraB.cameraSubcategory
                                                        << FieldNames::rtspURL << changedRTSP ) );
    updateList.emplace_back( JSON( FieldNames::cameraId << cameraD.cameraId << FieldNames::cameraName
                                                        << cameraD.cameraName << FieldNames::cameraSubcategory
                                                        << cameraD.cameraSubcategory << FieldNames::rtspURL
                                                        << cameraD.rtspURL ) );

    auto cameraListBeforeUpdate = getCameraList( manager );

    manager.updateCameraList( updateList );

    auto cameraList = getCameraList( manager );
    ASSERT_EQ( getCameraList( manager ).size(), 3 );

    EXPECT_TRUE( cameraList.count( cameraA.cameraId ) );
    checkCameraCache( manager, cameraA.cameraId, __LINE__ );

    auto modifiedCamera = cameraList.at( cameraB.cameraId );
    EXPECT_EQ( modifiedCamera.rtspURL, changedRTSP );
    EXPECT_EQ( modifiedCamera.cameraName, changedName );
    EXPECT_EQ( json_get_str( core.m_devices.at( modifiedCamera.cameraDeviceId ), "name" ), changedName );
    EXPECT_TRUE( json_get_str( core.m_devices.at( modifiedCamera.sensorDeviceId ), "name" ).find( changedName ) !=
                 std::string::npos );
    EXPECT_EQ( cameraListBeforeUpdate.at( cameraB.cameraId ).cameraDeviceId, modifiedCamera.cameraDeviceId )
        << "Update should not re-create a new device";
    checkCameraCache( manager, modifiedCamera.cameraId, __LINE__ );

    EXPECT_FALSE( cameraList.count( cameraC.cameraId ) );
    EXPECT_FALSE( manager.getCameraItemsCache().findItem( cameraC.hotzonesItemId ) ) << "Item from deleted camera";

    EXPECT_TRUE( cameraList.count( cameraD.cameraId ) );
    checkCameraCache( manager, cameraD.cameraId, __LINE__ );
};

TEST_F( CameraManagerTestSuite, DeleteAllCameras ) /* 1.5 */ {
    using namespace Vidoo4Hub::Resources::DB;

    auto cameraA = getDummyCamera( "id_A" );
    auto cameraB = getDummyCamera( "id_B" );

    FakeStreamManager streamManager;
    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";
    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    manager.addCamera( cameraA.cameraId, cameraA.cameraSubcategory, cameraA.cameraName, cameraA.rtspURL );
    manager.addCamera( cameraB.cameraId, cameraB.cameraSubcategory, cameraB.cameraName, cameraB.rtspURL );
    ASSERT_EQ( getCameraList( manager ).size(), 2 );
    checkCameraCache( manager, cameraA.cameraId, __LINE__ );
    checkCameraCache( manager, cameraB.cameraId, __LINE__ );

    std::vector< Json > emptyList;
    manager.updateCameraList( emptyList );

    auto cameraList = getCameraList( manager );
    ASSERT_EQ( getCameraList( manager ).size(), 0 );
    EXPECT_EQ( manager.getCameraItemsCache().size(), 0 );
};

TEST_F( CameraManagerTestSuite, CleanCameraItemValues ) /* 1.6 */ {
    using namespace Vidoo4Hub::Resources::DB;

    auto dummyCamera = getDummyCamera( "id_A" );

    FakeStreamManager streamManager;
    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";
    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    manager.addCamera(
        dummyCamera.cameraId, dummyCamera.cameraSubcategory, dummyCamera.cameraName, dummyCamera.rtspURL );
    ASSERT_EQ( getCameraList( manager ).size(), 1 );

    auto camera = manager.getCameras().begin()->second;

    auto updateItemValue = [&]( const std::string& itemId, const Json& value ) {
        core.getItemValue( itemId ) = value;
        EXPECT_EQ( core.getItemValue( itemId ), value );
    };

    updateItemValue( camera.motionItemId, Json( true ) );
    updateItemValue( camera.securityThreatItemId, Json( true ) );
    updateItemValue( camera.snapshotItemId, Json( 1 ) );
    updateItemValue( camera.makeRecordingItemId, Json( 30 ) );
    updateItemValue( camera.stopRecordingItemId, Json( 1 ) );

    const std::string defaultHotzoneId = HubPlatform::Vidoo4Hub::Resources::DefaultValues::defaultHotzoneId;
    const std::string hotzoneId = "hotzone1";

    auto hotzones = CameraUtils::getDefaultHotzoneDictionaryJson();
    HotzoneBuilder::DetectedObjects detectedObjects1 = { { DetectableTypes::Type::Object, { "object1" } },
                                                         { DetectableTypes::Type::Person, { "person1" } },
                                                         { DetectableTypes::Type::QRCode, { "qrcode1" } } };
    HotzoneBuilder::DetectedObjects detectedObjects2 = { { DetectableTypes::Type::Object, { "object2" } },
                                                         { DetectableTypes::Type::Person, { "person2" } },
                                                         { DetectableTypes::Type::QRCode, { "qrcode2" } } };
    hotzones[ defaultHotzoneId ] = HotzoneBuilder::hotzone( "hotzoneName1", detectedObjects1 );
    hotzones[ hotzoneId ] = HotzoneBuilder::hotzone( "hotzoneName2", detectedObjects2 );

    updateItemValue( camera.hotzonesItemId, hotzones );

    manager.cleanItemValues();

    EXPECT_EQ( core.getItemValue( camera.motionItemId ), Json( false ) );
    EXPECT_EQ( core.getItemValue( camera.securityThreatItemId ), Json( false ) );
    EXPECT_EQ( core.getItemValue( camera.snapshotItemId ), Json( 0 ) );
    EXPECT_EQ( core.getItemValue( camera.makeRecordingItemId ), Json( 0 ) );
    EXPECT_EQ( core.getItemValue( camera.stopRecordingItemId ), Json( 0 ) );

    auto expectedHotzones = CameraUtils::getDefaultHotzoneDictionaryJson();
    expectedHotzones[ defaultHotzoneId ] = HotzoneBuilder::hotzone( "hotzoneName1", {} );
    expectedHotzones[ hotzoneId ] = HotzoneBuilder::hotzone( "hotzoneName2", {} );

    EXPECT_EQ( core.getItemValue( camera.hotzonesItemId ), expectedHotzones );
};

TEST_F( CameraManagerTestSuite, HandleStreamEvents ) /* 1.7 */ {
    namespace StreamFields = HubPlatform::CoreD::Model::Resources::CameraStreamValueFields;

    FakeStreamManager streamManager;
    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";
    CameraManager manager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    auto dummyCamera = getDummyCamera( "id_1" );
    manager.addCamera(
        dummyCamera.cameraId, dummyCamera.cameraSubcategory, dummyCamera.cameraName, dummyCamera.rtspURL );

    const std::string streamUrl = "rtsp://<ip>:<port>/<stream-id>";
    const auto defaultItemValue = JSON( StreamFields::localRtspUrl << "" << StreamFields::cloudLiveStreamUrl << "" );
    const auto updatedItemValue =
        JSON( StreamFields::localRtspUrl << streamUrl << StreamFields::cloudLiveStreamUrl << "" );

    const auto cameraList = getCameraList( manager );
    ASSERT_EQ( cameraList.size(), 1 );
    const auto& camera = cameraList.begin()->second;

    ASSERT_EQ( core.getItemValue( camera.streamId ), defaultItemValue );

    manager.handleStreamStarted( camera.streamId, streamUrl );
    EXPECT_EQ( core.getItemValue( camera.streamId ), updatedItemValue );

    manager.handleStreamStopped( camera.streamId );
    EXPECT_EQ( core.getItemValue( camera.streamId ), defaultItemValue );
};

TEST_F( CameraManagerTestSuite, ItemsCache ) /* 2.1 */ {
    using namespace Vidoo4Hub::Resources::DB;

    auto cameraA = getDummyCamera( "id_A" );
    auto cameraB = getDummyCamera( "id_B" );

    CameraItemsCache cache;
    EXPECT_EQ( cache.size(), 0 );

    cache.updateCache( { { cameraA.cameraId, cameraA } } );
    checkCameraCache( cache, cameraA, __LINE__ );
    EXPECT_EQ( cache.size(), m_itemsPerCamera );
    EXPECT_TRUE( cache.findItem( cameraA.hotzonesItemId ) );

    cache.updateCache( { { cameraB.cameraId, cameraB } } );
    checkCameraCache( cache, cameraB, __LINE__ );
    EXPECT_TRUE( cache.size() == m_itemsPerCamera );
    EXPECT_TRUE( cache.findItem( cameraB.hotzonesItemId ) );
    EXPECT_FALSE( cache.findItem( cameraA.hotzonesItemId ) ) << "Deleted camera";

    cache.updateCache( { { cameraA.cameraId, cameraA }, { cameraB.cameraId, cameraB } } );
    EXPECT_TRUE( cache.size() == 2 * m_itemsPerCamera );
    checkCameraCache( cache, cameraA, __LINE__ );
    checkCameraCache( cache, cameraB, __LINE__ );

    cache.updateCache( {} );
    EXPECT_TRUE( cache.size() == 0 );
};

}  // namespace UT
}  // namespace HubPlatform
