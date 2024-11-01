#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "vidoo4hub/tests/fakes/fake_vidoo4hub_service.h"
#include "vidoo4hub/vidoo_manager/vidoo_manager.h"
#include "vidoo4hub/vidoo_manager/vidoo_manager_resources.h"
#include "vidoo4hub/camera_manager/camera_manager.h"
#include "vidoo4hub/vidoo_mediator/vidoo_mediator.h"
#include "vidoo4hub/dictionary_manager/dictionary_manager.h"
#include "vidoo4hub/vidoo_device/vidoo_device.h"
#include "vidoo4hub/tests/fakes/fake_dictionary_manager.h"
#include "vidoo4hub/tests/fakes/fake_stream_manager.h"
#include "vidoo4hub/tests/fakes/fake_core_manager.h"
#include "vidoo4hub/tests/fakes/fake_cored_remote_interface.h"
#include "vidoo4hub/tests/fakes/fake_vidoo_manager_resources.h"
#include "vidoo4hub/tests/fakes/fake_vidoo_client.h"
#include "vidoo4hub/tests/mocks/stream_manager_mock.h"
#include "vidoo4hub/tests/utils/test_utils.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "cored/model/resources/cored_resources.h"

/*

TESTPLAN

DONE    1. VidooMediator tests
DONE        1.1 Test generic flow
DONE        1.2 Handle ItemUpdate
DONE        1.3 Handle stream events
DONE        1.4 Handle request camera streams

*/

namespace HubPlatform {
namespace UT {

using namespace Vidoo4Hub;

class VidooMediatorTestSuite : public FakeVidoo4HubService {
public:
    const std::string m_defaultHotzoneId = Vidoo4Hub::Resources::DefaultValues::defaultHotzoneId;
};

TEST_F( VidooMediatorTestSuite, TestFlow ) /* 1.1 */ {
    using namespace Vidoo4Hub::Resources;

    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace QRCodeFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    FakeVidooClient vidooClient;

    VidooDevice vidooDevice( getHostService(), coreManager, gatewayId );

    FakeStreamManager streamManager;
    CameraManager cameraManager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    DictionaryManager dictionaryManager( coreManager );
    VidooManagerResources vidooManagerResources( vidooDevice, cameraManager );
    VidooManager vidooManager( vidooManagerResources, dictionaryManager );

    VidooMediator mediator( coreManager, vidooClient, cameraManager, streamManager, vidooManager );

    const std::string cameraId1 = "camera1";
    const std::string hotzoneId = "hotzone1";
    const std::string objectId = "object1";
    const std::string personId = "person1";
    const std::string qrCodeId = "qrcode1";

    {  // add camera
        Json addCamera =
            R"({"id":"id","method":"update_camera_list","params":{"cameras":[{"camera_id":"camera1","camera_subcategory":"doorbell_cam","camera_name":"cameraName1","rtsp_url":"rtspURL1"}]}})"_json;
        mediator.handleVidooMessage( addCamera );
        ASSERT_EQ( cameraManager.getCameras().size(), 1 );
    }

    const auto camera = cameraManager.getCameras().begin()->second;

    {  // register object/person/qrCode
        Json registerObject =
            R"({"id":"id","method":"update_object_list","params":{"objects":[{"object_id":"object1","object_name":"objectName1"}]}})"_json;
        mediator.handleVidooMessage( registerObject );

        Json registerPerson =
            R"({"id":"id","method":"update_people_list","params":{"people":[{"person_id":"person1","person_name":"PersonName1"}]}})"_json;
        mediator.handleVidooMessage( registerPerson );

        Json registerQRCode =
            R"({"id":"id","method":"update_qrcode_list","params":{"qrcodes":[{"qrcode_id":"qrcode1","qrcode_name":"QRCodeName1", "qrcode_value":"QRCodeValue1"}]}})"_json;
        mediator.handleVidooMessage( registerQRCode );

        auto& detectableObjects = core.getItemValue( vidooDevice.getDetectableItemId( DetectableTypes::Type::Object ) );
        auto& detectablePersons = core.getItemValue( vidooDevice.getDetectableItemId( DetectableTypes::Type::Person ) );
        auto& detectableQRCodes = core.getItemValue( vidooDevice.getDetectableItemId( DetectableTypes::Type::QRCode ) );

        EXPECT_TRUE( detectableObjects.count( objectId ) );
        EXPECT_TRUE( detectablePersons.count( personId ) );
        EXPECT_TRUE( detectableQRCodes.count( qrCodeId ) );
    }

    {  // update hotzones list
        Json updateHotzoneList =
            R"({"id":"id","method":"update_hotzone_list","params":{"hotzones":[{"camera_id":"camera1","hotzone_id":"hotzone1","hotzone_name":"hotzoneName1"}]}})"_json;
        mediator.handleVidooMessage( updateHotzoneList );

        auto& hotzones = core.getItemValue( camera.hotzonesItemId );
        EXPECT_EQ( hotzones.size(), 2 );
        EXPECT_TRUE( hotzones.count( m_defaultHotzoneId ) );
        EXPECT_TRUE( hotzones.count( hotzoneId ) );
    }

    {  // object and QR code motion started
        Json objectMotionStarted =
            R"({"id":"id","method":"motion","params":{"camera_id":"camera1","object_ids":["object1"],"people_ids":[],"qrcode_ids":["qrcode1"]}})"_json;

        EXPECT_EQ( core.getItemValue( camera.motionItemId ), Json( false ) );
        EXPECT_EQ( core.getItemValue( camera.securityThreatItemId ), Json( false ) );

        mediator.handleVidooMessage( objectMotionStarted );

        EXPECT_EQ( core.getItemValue( camera.motionItemId ), Json( true ) );
        EXPECT_EQ( core.getItemValue( camera.securityThreatItemId ), Json( true ) );

        auto& detectableObjects = core.getItemValue( vidooDevice.getDetectableItemId( DetectableTypes::Type::Object ) );
        auto& detectableQRCodes = core.getItemValue( vidooDevice.getDetectableItemId( DetectableTypes::Type::QRCode ) );
        auto& hotzones = core.getItemValue( camera.hotzonesItemId );

        EXPECT_EQ( detectableObjects.at( objectId ).at( ObjectFields::LastHotzone ).at( ObjectFields::Id ),
                   m_defaultHotzoneId );
        EXPECT_EQ( detectableQRCodes.at( qrCodeId ).at( QRCodeFields::LastHotzone ).at( QRCodeFields::Id ),
                   m_defaultHotzoneId );

        EXPECT_EQ( hotzones.at( m_defaultHotzoneId ).at( HotzoneFields::Detected ).at( HotzoneFields::Objects ),
                   JSON_ARRAY( objectId ) );
        EXPECT_EQ( hotzones.at( m_defaultHotzoneId ).at( HotzoneFields::Detected ).at( HotzoneFields::QRCodes ),
                   JSON_ARRAY( qrCodeId ) );
        EXPECT_EQ( hotzones.at( hotzoneId ).at( HotzoneFields::Detected ).count( HotzoneFields::Persons ), 0 );
    }

    {  // person motion started in hotzone
        Json detectPersonMotion =
            R"({"id":"id","method":"hotzone_motion","params":{"camera_id":"camera1","hotzone_id":"hotzone1","object_ids":[],"people_ids":["person1"],"qrcode_ids":[]}})"_json;
        mediator.handleVidooMessage( detectPersonMotion );

        auto& detectablePersons = core.getItemValue( vidooDevice.getDetectableItemId( DetectableTypes::Type::Person ) );
        auto& hotzones = core.getItemValue( camera.hotzonesItemId );

        EXPECT_EQ( detectablePersons.at( personId ).at( ObjectFields::LastHotzone ).at( ObjectFields::Id ), hotzoneId );
        EXPECT_EQ( hotzones.at( hotzoneId ).at( HotzoneFields::Detected ).at( HotzoneFields::Persons ),
                   JSON_ARRAY( personId ) );
        EXPECT_EQ( hotzones.at( m_defaultHotzoneId ).at( HotzoneFields::Detected ).at( HotzoneFields::Objects ),
                   JSON_ARRAY( objectId ) );
    }

    {  // person motion ended in hotzone
        Json detectPersonMotion =
            R"({"id":"id","method":"hotzone_motion","params":{"camera_id":"camera1","hotzone_id":"hotzone1","object_ids":[],"people_ids":[],"qrcode_ids":[]}})"_json;
        mediator.handleVidooMessage( detectPersonMotion );

        auto& hotzones = core.getItemValue( camera.hotzonesItemId );
        EXPECT_EQ( hotzones.at( hotzoneId ).at( HotzoneFields::Detected ).count( HotzoneFields::Persons ), 0 );
        EXPECT_EQ( hotzones.at( hotzoneId ).at( HotzoneFields::Detected ).count( HotzoneFields::QRCodes ), 0 );
        EXPECT_EQ( hotzones.at( m_defaultHotzoneId ).at( HotzoneFields::Detected ).at( HotzoneFields::Objects ),
                   JSON_ARRAY( objectId ) );
    }

    {  // object and QR code motion ended
        Json objectMotionEnded =
            R"({"id":"id","method":"motion","params":{"camera_id":"camera1","object_ids":[],"people_ids":[],"qrcode_ids":[]}})"_json;

        EXPECT_EQ( core.getItemValue( camera.motionItemId ), Json( true ) );
        EXPECT_EQ( core.getItemValue( camera.securityThreatItemId ), Json( true ) );

        mediator.handleVidooMessage( objectMotionEnded );

        EXPECT_EQ( core.getItemValue( camera.motionItemId ), Json( false ) );
        EXPECT_EQ( core.getItemValue( camera.securityThreatItemId ), Json( false ) );

        auto& hotzones = core.getItemValue( camera.hotzonesItemId );

        EXPECT_EQ( hotzones.at( m_defaultHotzoneId ).at( HotzoneFields::Detected ).count( HotzoneFields::Objects ), 0 );
        EXPECT_EQ( hotzones.at( hotzoneId ).at( HotzoneFields::Detected ).count( HotzoneFields::Persons ), 0 );
        EXPECT_EQ( hotzones.at( hotzoneId ).at( HotzoneFields::Detected ).count( HotzoneFields::QRCodes ), 0 );
    }
};

TEST_F( VidooMediatorTestSuite, handleItemUpdate ) /* 1.2 */ {
    using namespace Vidoo4Hub::Resources;

    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    FakeVidooClient vidooClient;

    VidooDevice vidooDevice( getHostService(), coreManager, gatewayId );

    FakeStreamManager streamManager;
    CameraManager cameraManager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    DictionaryManager dictionaryManager( coreManager );
    VidooManagerResources vidooManagerResources( vidooDevice, cameraManager );
    VidooManager vidooManager( vidooManagerResources, dictionaryManager );

    VidooMediator mediator( coreManager, vidooClient, cameraManager, streamManager, vidooManager );

    const std::string cameraId1 = "camera1";
    const std::string cameraId2 = "camera2";

    Json cameraList = R"({"cameras":[
    {"camera_id":"camera1","camera_subcategory":"doorbell_cam","camera_name":"cameraName1","rtsp_url":"rtspURL1"},
    {"camera_id":"camera2","camera_subcategory":"doorbell_cam","camera_name":"cameraName2","rtsp_url":"rtspURL2"}
    ]})"_json;
    mediator.handleUpdateCameraList( cameraList );

    auto cameras = cameraManager.getCameras();
    ASSERT_EQ( cameras.size(), 2 );

    const auto camera1 = cameras.at( cameraId1 );
    const auto camera2 = cameras.at( cameraId2 );

    auto snapshotItemValue = [&]( const std::string& cameraId ) -> Json& {
        auto& camera = cameraManager.getCameras().at( cameraId );
        auto& itemJson = core.m_items.at( camera.snapshotItemId );
        return itemJson.at( CoreD::Model::Resources::ItemFields::ValueFieldName );
    };

    auto makeRecordingItemValue = [&]( const std::string& cameraId ) -> Json& {
        auto& camera = cameraManager.getCameras().at( cameraId );
        auto& itemJson = core.m_items.at( camera.makeRecordingItemId );
        return itemJson.at( CoreD::Model::Resources::ItemFields::ValueFieldName );
    };

    auto stopRecordingItemValue = [&]( const std::string& cameraId ) -> Json& {
        auto& camera = cameraManager.getCameras().at( cameraId );
        auto& itemJson = core.m_items.at( camera.stopRecordingItemId );
        return itemJson.at( CoreD::Model::Resources::ItemFields::ValueFieldName );
    };

    {  // update snapshot item
        snapshotItemValue( cameraId1 ) = Json( 1 );
        EXPECT_EQ( snapshotItemValue( cameraId1 ), Json( 1 ) );

        auto updateMessage = JSON( "_id" << camera1.snapshotItemId << "value" << 1 );
        mediator.handleItemUpdate( updateMessage );

        EXPECT_EQ( snapshotItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( snapshotItemValue( cameraId2 ), Json( 0 ) );

        EXPECT_EQ( makeRecordingItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( makeRecordingItemValue( cameraId2 ), Json( 0 ) );

        EXPECT_EQ( stopRecordingItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( stopRecordingItemValue( cameraId2 ), Json( 0 ) );

        ASSERT_EQ( vidooClient.m_messages.size(), 1 );
        auto vidooMessage = vidooClient.m_messages.front();
        vidooClient.m_messages.clear();

        EXPECT_EQ( json_get_str( vidooMessage, "method" ), RPC::takeSnapshot );
        EXPECT_EQ( vidooMessage.at( "params" ), JSON( MessageFields::cameraId << cameraId1 ) );
    }

    {  // update stopRecording item
        stopRecordingItemValue( cameraId1 ) = Json( 1 );
        EXPECT_EQ( stopRecordingItemValue( cameraId1 ), Json( 1 ) );

        auto updateMessage = JSON( "_id" << camera1.stopRecordingItemId << "value" << 1 );
        mediator.handleItemUpdate( updateMessage );

        EXPECT_EQ( snapshotItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( snapshotItemValue( cameraId2 ), Json( 0 ) );

        EXPECT_EQ( makeRecordingItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( makeRecordingItemValue( cameraId2 ), Json( 0 ) );

        EXPECT_EQ( stopRecordingItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( stopRecordingItemValue( cameraId2 ), Json( 0 ) );

        ASSERT_EQ( vidooClient.m_messages.size(), 1 );
        auto vidooMessage = vidooClient.m_messages.front();
        vidooClient.m_messages.clear();

        EXPECT_EQ( json_get_str( vidooMessage, "method" ), RPC::stopRecording );
        EXPECT_EQ( vidooMessage.at( "params" ), JSON( MessageFields::cameraId << cameraId1 ) );
    }

    {  // update makeRecording item
        makeRecordingItemValue( cameraId1 ) = Json( 30 );
        EXPECT_EQ( makeRecordingItemValue( cameraId1 ), Json( 30 ) );

        auto updateMessage = JSON( "_id" << camera1.makeRecordingItemId << "value" << 30 );
        mediator.handleItemUpdate( updateMessage );

        EXPECT_EQ( snapshotItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( snapshotItemValue( cameraId2 ), Json( 0 ) );

        EXPECT_EQ( makeRecordingItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( makeRecordingItemValue( cameraId2 ), Json( 0 ) );

        EXPECT_EQ( stopRecordingItemValue( cameraId1 ), Json( 0 ) );
        EXPECT_EQ( stopRecordingItemValue( cameraId2 ), Json( 0 ) );

        ASSERT_EQ( vidooClient.m_messages.size(), 1 );
        auto vidooMessage = vidooClient.m_messages.front();
        vidooClient.m_messages.clear();

        EXPECT_EQ( json_get_str( vidooMessage, "method" ), RPC::makeRecording );
        EXPECT_EQ( vidooMessage.at( "params" ),
                   JSON( MessageFields::cameraId << cameraId1 << MessageFields::duration << 30 ) );
    }
};

TEST_F( VidooMediatorTestSuite, HandleStreamEvents ) /* 1.3 */ {
    using namespace Vidoo4Hub::Resources;

    namespace StreamFields = HubPlatform::CoreD::Model::Resources::CameraStreamValueFields;

    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    FakeVidooClient vidooClient;
    VidooDevice vidooDevice( getHostService(), coreManager, gatewayId );
    FakeStreamManager streamManager;
    CameraManager cameraManager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );
    DictionaryManager dictionaryManager( coreManager );
    VidooManagerResources vidooManagerResources( vidooDevice, cameraManager );
    VidooManager vidooManager( vidooManagerResources, dictionaryManager );

    VidooMediator mediator( coreManager, vidooClient, cameraManager, streamManager, vidooManager );

    {  // add camera
        Json addCamera =
            R"({"id":"id","method":"update_camera_list","params":{"cameras":[{"camera_id":"camera1","camera_subcategory":"doorbell_cam","camera_name":"cameraName1","rtsp_url":"rtspURL1"}]}})"_json;
        mediator.handleVidooMessage( addCamera );
        ASSERT_EQ( cameraManager.getCameras().size(), 1 );
    }

    const auto camera = cameraManager.getCameras().begin()->second;

    const std::string streamUrl = "rtsp://<ip>:<port>/<stream-id>";
    const auto defaultItemValue = JSON( StreamFields::localRtspUrl << "" << StreamFields::cloudLiveStreamUrl << "" );
    const auto updatedItemValue =
        JSON( StreamFields::localRtspUrl << streamUrl << StreamFields::cloudLiveStreamUrl << "" );

    ASSERT_EQ( core.getItemValue( camera.streamId ), defaultItemValue );

    mediator.handleCamerasStartRtspStreaming(
        JSON( BusMessageFields::streamId << camera.streamId << BusMessageFields::localRtspUrl << streamUrl ) );
    EXPECT_EQ( core.getItemValue( camera.streamId ), updatedItemValue );

    mediator.handleCamerasStopRtspStreaming( JSON( BusMessageFields::streamId << camera.streamId ) );
    EXPECT_EQ( core.getItemValue( camera.streamId ), defaultItemValue );
};

TEST_F( VidooMediatorTestSuite, TestRequestCameraStreams ) /* 1.4 */ {
    using namespace Vidoo4Hub::Resources;

    const std::string gatewayId = "gatewayId";
    const std::string vidooDeviceId = "vidooDeviceId";

    FakeCoreDRemoteInterface core( getHostService() );
    FakeCoreManager coreManager( core );
    FakeVidooClient vidooClient;

    VidooDevice vidooDevice( getHostService(), coreManager, gatewayId );

    const Json cameraStreams = R"([{"rtspUrl":"URL"}])"_json;
    StreamManagerMock streamManager;
    EXPECT_CALL( streamManager, getCameraStreams() ).WillOnce( testing::Return( cameraStreams ) );

    CameraManager cameraManager( getHostService(), coreManager, streamManager, vidooDeviceId, gatewayId );

    DictionaryManager dictionaryManager( coreManager );
    VidooManagerResources vidooManagerResources( vidooDevice, cameraManager );
    VidooManager vidooManager( vidooManagerResources, dictionaryManager );

    VidooMediator mediator( coreManager, vidooClient, cameraManager, streamManager, vidooManager );

    Json requestCameraList = R"({"id":"id","method":"request_camera_streams_list","params":{}})"_json;
    mediator.handleVidooMessage( requestCameraList );

    ASSERT_EQ( vidooClient.m_messages.size(), 1 );
    auto vidooMessage = vidooClient.m_messages.front();
    vidooClient.m_messages.clear();

    EXPECT_EQ( json_get_str( vidooMessage, "method" ), RPC::updateCameraStreamsList );
    EXPECT_EQ( vidooMessage.at( "params" ), JSON( MessageFields::streamsList << cameraStreams ) );
};

}  // namespace UT
}  // namespace HubPlatform
