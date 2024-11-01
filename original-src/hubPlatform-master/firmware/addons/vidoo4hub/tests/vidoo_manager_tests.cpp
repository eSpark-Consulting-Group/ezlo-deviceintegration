#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "vidoo4hub/vidoo_manager/vidoo_manager.h"
#include "vidoo4hub/tests/fakes/fake_dictionary_manager.h"
#include "vidoo4hub/tests/fakes/fake_vidoo_manager_resources.h"
#include "vidoo4hub/tests/utils/test_utils.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "cored/model/resources/cored_resources.h"

/*

TESTPLAN

DONE    1. Detectable objects
DONE        1.1 Update object list
DONE        1.2 Update object list with empty list
DONE        1.3 Update object list with invalid parameters
DONE    2. Detectable persons
DONE        2.1 Update person list
DONE        2.2 Update person list with empty list
DONE        2.3 Update person list with invalid parameters
DONE    3. Detectable persons
DONE        3.1 Update person list
DONE        3.2 Update person list with empty list
DONE        3.3 Update person list with invalid parameters
DONE    4. Hotzones
DONE        4.1 Update hotzones list
DONE    5. Motions
DONE        5.1 Motion
DONE        5.2 Multizone Motion
DONE    6. Syncing
DONE        6.1 Sync updated hotzones
DONE        6.2 Sync updated objects/persons

*/

namespace HubPlatform {
namespace UT {

using namespace Vidoo4Hub;

class VidooManagerTestSuite : public ::testing::Test {
public:
    using List = std::vector< std::string >;
    std::map< std::string, VidooCamera > createCameraList( const List& cameraIds ) {
        std::map< std::string, VidooCamera > cameraList;
        for( auto& id : cameraIds )
            cameraList.emplace( id, getDummyCamera( id ) );
        return cameraList;
    }

    static FakeVidooManagerResources createFakeVidooManagerResources() {
        using Type = Vidoo4Hub::DetectableTypes::Type;
        FakeVidooManagerResources resources;
        resources.m_detectableItemsMap = {
            { Type::Object, "objectsItemId" }, { Type::Person, "personsItemId" }, { Type::QRCode, "qrCodesItemId" }
        };
        return resources;
    }

public:
protected:
    const std::string m_defaultHotzoneId = HubPlatform::Vidoo4Hub::Resources::DefaultValues::defaultHotzoneId;
    const std::string m_defaultHotzoneName = HubPlatform::Vidoo4Hub::Resources::DefaultValues::defaultHotzoneName;
};

TEST_F( VidooManagerTestSuite, updateObjectList ) /* 1.1 */ {
    using namespace Vidoo4Hub::Resources;
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;
    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    DetectableObject object1( "id1" );
    DetectableObject object2( "id2" );
    DetectableObject object3( "id3" );
    DetectableObject object4( "id4" );

    VidooManager vidooManager( resources, dictionaryManager );

    const auto detectableObjectsItemId = resources.getDetectableItemId( DetectableTypes::Type::Object );

    {  // initial list
        auto objectList = MessageBuilder::objectList( { object1, object2, object3 } );
        vidooManager.updateObjectList( objectList );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

        const auto dictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        const auto expectedJson = DetectableBuilder::dictionary( { object1, object2, object3 } );
        EXPECT_EQ( dictionary.dump(), expectedJson.dump() );
    }

    const auto& removedObject = object1;
    const auto& commonObject = object2;
    const auto& newObject = object4;
    const DetectableObject newNameObject( object3.id, "objectNewName" );

    {  // updated list
        auto objectList = MessageBuilder::objectList( { commonObject, newNameObject, newObject } );
        vidooManager.updateObjectList( objectList );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

        const auto dictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        const auto expectedJson = DetectableBuilder::dictionary( { commonObject, newNameObject, newObject } );
        EXPECT_EQ( dictionary.dump(), expectedJson.dump() );
    }

    vidooManager.updateObjectList( {} );
    ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( VidooManagerTestSuite, updateObjectListWithEmptyList ) /* 1.2 */ {
    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    VidooManager vidooManager( resources, dictionaryManager );

    std::vector< Json > objectList;
    vidooManager.updateObjectList( objectList );

    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( VidooManagerTestSuite, updateObjectListWithInvalidParameters ) /* 1.3 */ {
    using namespace Vidoo4Hub::Resources;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    VidooManager vidooManager( resources, dictionaryManager );

    {
        std::vector< Json > list = { JSON( MessageFields::objectName << "objectName" ) };
        EXPECT_THROW( vidooManager.updateObjectList( list ), Vidoo4HubError );
        EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 ) << "Missing object id";
    }

    {
        auto list = MessageBuilder::objectList( { DetectableObject( "", "objectName" ) } );
        EXPECT_THROW( vidooManager.updateObjectList( list ), Vidoo4HubError );
        EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 ) << "Empty object id";
    }
};

TEST_F( VidooManagerTestSuite, updatePersonList ) /* 2.1 */ {
    using namespace Vidoo4Hub::Resources;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    DetectablePerson person1( "id1" );
    DetectablePerson person2( "id2" );
    DetectablePerson person3( "id3" );
    DetectablePerson person4( "id4" );

    VidooManager vidooManager( resources, dictionaryManager );

    const auto detectablePersonsItemId = resources.getDetectableItemId( DetectableTypes::Type::Person );

    {  // initial list

        auto personList = MessageBuilder::personList( { person1, person2, person3 } );
        vidooManager.updatePeopleList( personList );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

        const auto dictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        const auto expectedJson = DetectableBuilder::dictionary( { person1, person2, person3 } );
        EXPECT_EQ( dictionary.dump(), expectedJson.dump() );
    }

    const auto& removedPerson = person1;
    const auto& commonPerson = person2;
    const auto& newPerson = person4;
    const DetectablePerson newNamePerson( person3.id, "personNewName" );

    {  // updated list
        auto personList = MessageBuilder::personList( { commonPerson, newNamePerson, newPerson } );
        vidooManager.updatePeopleList( personList );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

        const auto dictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        const auto expectedJson = DetectableBuilder::dictionary( { commonPerson, newNamePerson, newPerson } );
        EXPECT_EQ( dictionary.dump(), expectedJson.dump() );
    }

    vidooManager.updatePeopleList( {} );
    ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( VidooManagerTestSuite, updatePersonListWithEmptyList ) /* 2.2 */ {
    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    VidooManager vidooManager( resources, dictionaryManager );

    std::vector< Json > objectList;
    vidooManager.updatePeopleList( objectList );

    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( VidooManagerTestSuite, updatePersonListWithInvalidParameters ) /* 2.3 */ {
    using namespace Vidoo4Hub::Resources;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    VidooManager vidooManager( resources, dictionaryManager );

    {
        std::vector< Json > list = { JSON( MessageFields::personName << "personName" ) };
        EXPECT_THROW( vidooManager.updatePeopleList( list ), Vidoo4HubError );
        EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 ) << "Missing person id";
    }

    {
        std::vector< Json > list = MessageBuilder::personList( { DetectablePerson( "", "personName" ) } );
        EXPECT_THROW( vidooManager.updatePeopleList( list ), Vidoo4HubError );
        EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 ) << "Empty person id";
    }
};

TEST_F( VidooManagerTestSuite, updateQRCodeList ) /* 3.1 */ {
    using namespace Vidoo4Hub::Resources;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    DetectableQRCode qrCode1( "id1" );
    DetectableQRCode qrCode2( "id2" );
    DetectableQRCode qrCode3( "id3" );
    DetectableQRCode qrCode4( "id4" );

    VidooManager vidooManager( resources, dictionaryManager );

    const auto detectableQRCodesItemId = resources.getDetectableItemId( DetectableTypes::Type::QRCode );

    {  // initial list

        auto qrCodeList = MessageBuilder::qrCodeList( { qrCode1, qrCode2, qrCode3 } );
        vidooManager.updateQRCodeList( qrCodeList );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

        const auto dictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        const auto expectedJson = DetectableBuilder::dictionary( { qrCode1, qrCode2, qrCode3 } );
        EXPECT_EQ( dictionary.dump(), expectedJson.dump() );
    }

    const auto& removedQRCode = qrCode1;
    const auto& commonQRCode = qrCode2;
    const auto& newQRCode = qrCode4;
    const DetectableQRCode newNameQRCode( qrCode3.id, "qrCodeNewName", "qrCodeNewValue" );

    {  // updated list
        auto qrCodeList = MessageBuilder::qrCodeList( { commonQRCode, newNameQRCode, newQRCode } );
        vidooManager.updateQRCodeList( qrCodeList );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 1 );

        const auto dictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        const auto expectedJson = DetectableBuilder::dictionary( { commonQRCode, newNameQRCode, newQRCode } );
        EXPECT_EQ( dictionary.dump(), expectedJson.dump() );
    }

    vidooManager.updateQRCodeList( {} );
    ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( VidooManagerTestSuite, updateQRCodeListWithEmptyList ) /* 3.2 */ {
    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    VidooManager vidooManager( resources, dictionaryManager );

    std::vector< Json > objectList;
    vidooManager.updateQRCodeList( objectList );

    EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
};

TEST_F( VidooManagerTestSuite, updateQRCodeListWithInvalidParameters ) /* 3.3 */ {
    using namespace Vidoo4Hub::Resources;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId" } );

    VidooManager vidooManager( resources, dictionaryManager );

    {
        std::vector< Json > list = { JSON( MessageFields::qrCodeName << "qrCodeName" << MessageFields::qrCodeValue
                                                                     << "qrCodeValue" ) };
        EXPECT_THROW( vidooManager.updateQRCodeList( list ), Vidoo4HubError ) << "Missing QR code id";
        EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
    }

    {
        std::vector< Json > list =
            MessageBuilder::qrCodeList( { DetectableQRCode( "", "qrCodeName", "qrCodeValue" ) } );
        EXPECT_THROW( vidooManager.updateQRCodeList( list ), Vidoo4HubError ) << "Empty QR code id";
        EXPECT_EQ( dictionaryManager.m_dictionaryMap.size(), 0 );
    }
};

TEST_F( VidooManagerTestSuite, updateHotzonesList ) /* 4.1 */ {
    using namespace Vidoo4Hub::Resources;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId1", "cameraId2" } );
    auto& camera1 = resources.m_cameras.at( "cameraId1" );
    auto& camera2 = resources.m_cameras.at( "cameraId2" );

    dictionaryManager.updateDictionary( camera1.hotzonesItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );
    dictionaryManager.updateDictionary( camera2.hotzonesItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );

    VidooManager vidooManager( resources, dictionaryManager );

    const std::string hotzoneId1 = "hotzoneId1";
    const std::string hotzoneName1 = "hotzoneName1";

    const std::string hotzoneId2 = "hotzoneId2";
    const std::string hotzoneName2 = "hotzoneName2";

    {  // set one hotzone for first camera
        std::vector< Json > list = { MessageBuilder::hotzone( camera1.cameraId, hotzoneId1, hotzoneName1 ) };
        vidooManager.updateHotzoneList( list );

        auto expectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        expectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 2 ) << "One timezone item for each camera";

        const auto dictionary1 = dictionaryManager.readDictionary( camera1.hotzonesItemId );
        EXPECT_EQ( dictionary1.dump(), expectedJson.dump() );

        const auto dictionary2 = dictionaryManager.readDictionary( camera2.hotzonesItemId );
        EXPECT_EQ( dictionary2.dump(), CameraUtils::getDefaultHotzoneDictionaryJson().dump() );
    }

    {  // set two hotzones for first camera
        std::vector< Json > list = { MessageBuilder::hotzone( camera1.cameraId, hotzoneId1, hotzoneName1 ),
                                     MessageBuilder::hotzone( camera1.cameraId, hotzoneId2, hotzoneName2 ) };

        vidooManager.updateHotzoneList( list );

        auto expectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        expectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        expectedJson[ hotzoneId2 ] = HotzoneBuilder::hotzone( hotzoneName2, {} );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 2 ) << "One timezone item for each camera";

        const auto dictionary1 = dictionaryManager.readDictionary( camera1.hotzonesItemId );
        EXPECT_EQ( dictionary1.dump(), expectedJson.dump() );

        const auto dictionary2 = dictionaryManager.readDictionary( camera2.hotzonesItemId );
        EXPECT_EQ( dictionary2.dump(), CameraUtils::getDefaultHotzoneDictionaryJson().dump() );
    }

    const std::string hotzoneId3 = "hotzoneId3";
    const std::string hotzoneName3 = "hotzoneName3";

    {  // set hotzone only for second camera
        std::vector< Json > list = { MessageBuilder::hotzone( camera2.cameraId, hotzoneId3, hotzoneName3 ) };
        vidooManager.updateHotzoneList( list );

        auto expectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        expectedJson[ hotzoneId3 ] = HotzoneBuilder::hotzone( hotzoneName3, {} );
        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 2 ) << "One timezone item for each camera";

        const auto dictionary1 = dictionaryManager.readDictionary( camera1.hotzonesItemId );
        EXPECT_EQ( dictionary1.dump(), CameraUtils::getDefaultHotzoneDictionaryJson().dump() );

        const auto dictionary2 = dictionaryManager.readDictionary( camera2.hotzonesItemId );
        EXPECT_EQ( dictionary2.dump(), expectedJson.dump() );
    }

    {  // clean hotzones
        std::vector< Json > list = {};
        vidooManager.updateHotzoneList( list );

        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 2 ) << "One timezone item for each camera";
        const auto dictionary1 = dictionaryManager.readDictionary( camera1.hotzonesItemId );
        EXPECT_EQ( dictionary1.dump(), CameraUtils::getDefaultHotzoneDictionaryJson().dump() );

        const auto dictionary2 = dictionaryManager.readDictionary( camera2.hotzonesItemId );
        EXPECT_EQ( dictionary2.dump(), CameraUtils::getDefaultHotzoneDictionaryJson().dump() );
    }
};

TEST_F( VidooManagerTestSuite, motion ) /* 5.1 */ {
    using namespace Vidoo4Hub::Resources;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace QRCodeFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId1", "cameraId2" } );
    auto& camera1 = resources.m_cameras.at( "cameraId1" );
    auto& camera2 = resources.m_cameras.at( "cameraId2" );

    dictionaryManager.updateDictionary( camera1.hotzonesItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );
    dictionaryManager.updateDictionary( camera2.hotzonesItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );

    VidooManager vidooManager( resources, dictionaryManager );

    DetectableObject object1( "objectId1" );
    DetectablePerson person1( "personId1" );
    DetectableQRCode qrCode1( "qrCodeId1" );

    const std::string hotzoneId1 = "hotzoneId1";
    const std::string hotzoneName1 = "hotzoneName1";

    const auto detectableObjectsItemId = resources.getDetectableItemId( DetectableTypes::Type::Object );
    const auto detectablePersonsItemId = resources.getDetectableItemId( DetectableTypes::Type::Person );
    const auto detectableQRCodesItemId = resources.getDetectableItemId( DetectableTypes::Type::QRCode );

    {  // initial state
        vidooManager.updateObjectList( MessageBuilder::objectList( { object1 } ) );
        vidooManager.updatePeopleList( MessageBuilder::personList( { person1 } ) );
        vidooManager.updateQRCodeList( MessageBuilder::qrCodeList( { qrCode1 } ) );
        vidooManager.updateHotzoneList( { MessageBuilder::hotzone( camera1.cameraId, hotzoneId1, hotzoneName1 ) } );
    }

    const auto defaultHotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

    {  // object detected in default hotzone
        dictionaryManager.updateDictionary( camera1.hotzonesItemId, defaultHotzonesDictionary );

        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } } };
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );

        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 5 )
            << "2x hotzones items + objects item + persons item + QR codes item";

        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );

        const auto objectDictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        const std::string objectDate =
            objectDictionary.at( object1.id ).at( ObjectFields::LastHotzone ).at( ObjectFields::Date );

        const auto objectExpectedJson = DetectableBuilder::dictionary(
            object1.id, object1.name, objectDate, camera1.hotzonesItemId, m_defaultHotzoneId );
        EXPECT_EQ( objectDictionary.dump(), objectExpectedJson.dump() );

        const auto personsDictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        const auto personsExpectedJson = JSON( person1.id << JSON( PersonFields::Name << person1.name ) );
        EXPECT_EQ( personsDictionary.dump(), personsExpectedJson.dump() );

        const auto qrCodesDictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        const auto qrCodesExpectedJson =
            JSON( qrCode1.id << JSON( QRCodeFields::Name << qrCode1.name << QRCodeFields::Value << qrCode1.value ) );
        EXPECT_EQ( qrCodesDictionary.dump(), qrCodesExpectedJson.dump() );
    }

    {  // person detected in default hotzone
        dictionaryManager.updateDictionary( camera1.hotzonesItemId, defaultHotzonesDictionary );

        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Person, { person1.id } } };
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );

        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 5 )
            << "2x hotzones items + objects item + persons item + QR codes item";

        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );

        const auto objectDictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        const std::string objectDate =
            objectDictionary.at( object1.id ).at( ObjectFields::LastHotzone ).at( ObjectFields::Date );

        const auto objectExpectedJson = DetectableBuilder::dictionary(
            object1.id, object1.name, objectDate, camera1.hotzonesItemId, m_defaultHotzoneId );
        EXPECT_EQ( objectDictionary.dump(), objectExpectedJson.dump() );

        const auto personsDictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        const std::string personDate =
            personsDictionary.at( person1.id ).at( PersonFields::LastHotzone ).at( PersonFields::Date );

        const auto personsExpectedJson = DetectableBuilder::dictionary(
            person1.id, person1.name, personDate, camera1.hotzonesItemId, m_defaultHotzoneId );
        EXPECT_EQ( personsDictionary.dump(), personsExpectedJson.dump() );

        const auto qrCodesDictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        const auto qrCodesExpectedJson =
            JSON( qrCode1.id << JSON( QRCodeFields::Name << qrCode1.name << QRCodeFields::Value << qrCode1.value ) );
        EXPECT_EQ( qrCodesDictionary.dump(), qrCodesExpectedJson.dump() );
    }

    {  // qr code detected in default hotzone
        dictionaryManager.updateDictionary( camera1.hotzonesItemId, defaultHotzonesDictionary );

        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::QRCode, { qrCode1.id } } };
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );

        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 5 )
            << "2x hotzones items + objects item + persons item + QR codes item";

        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );

        const auto objectDictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        const std::string objectDate =
            objectDictionary.at( object1.id ).at( ObjectFields::LastHotzone ).at( ObjectFields::Date );

        const auto objectExpectedJson = DetectableBuilder::dictionary(
            object1.id, object1.name, objectDate, camera1.hotzonesItemId, m_defaultHotzoneId );
        EXPECT_EQ( objectDictionary.dump(), objectExpectedJson.dump() );

        const auto personsDictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        const std::string personDate =
            personsDictionary.at( person1.id ).at( PersonFields::LastHotzone ).at( PersonFields::Date );

        const auto personsExpectedJson = DetectableBuilder::dictionary(
            person1.id, person1.name, personDate, camera1.hotzonesItemId, m_defaultHotzoneId );
        EXPECT_EQ( personsDictionary.dump(), personsExpectedJson.dump() );

        const auto qrCodesDictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        const std::string qrCodeDate =
            qrCodesDictionary.at( qrCode1.id ).at( QRCodeFields::LastHotzone ).at( QRCodeFields::Date );

        const auto qrCodesExpectedJson = DetectableBuilder::dictionary(
            qrCode1.id, qrCode1.name, qrCode1.value, qrCodeDate, camera1.hotzonesItemId, m_defaultHotzoneId );
        EXPECT_EQ( qrCodesDictionary.dump(), qrCodesExpectedJson.dump() );
    }

    {  // object, person and qrcode detected in hotzone 1
        dictionaryManager.updateDictionary( camera1.hotzonesItemId, defaultHotzonesDictionary );

        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } },
                                                           { DetectableTypes::Type::Person, { person1.id } },
                                                           { DetectableTypes::Type::QRCode, { qrCode1.id } } };
        vidooManager.motion( camera1.cameraId, hotzoneId1, detectedObjects );

        ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 5 )
            << "2x hotzones items + objects item + persons item + QR code item";

        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, {} );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, detectedObjects );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );

        const auto objectDictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        const std::string objectDate =
            objectDictionary.at( object1.id ).at( ObjectFields::LastHotzone ).at( ObjectFields::Date );
        const auto objectExpectedJson =
            DetectableBuilder::dictionary( object1.id, object1.name, objectDate, camera1.hotzonesItemId, hotzoneId1 );
        EXPECT_EQ( objectDictionary.dump(), objectExpectedJson.dump() );

        const auto personsDictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        const std::string personDate =
            personsDictionary.at( person1.id ).at( PersonFields::LastHotzone ).at( PersonFields::Date );
        const auto personsExpectedJson =
            DetectableBuilder::dictionary( person1.id, person1.name, personDate, camera1.hotzonesItemId, hotzoneId1 );
        EXPECT_EQ( personsDictionary.dump(), personsExpectedJson.dump() );

        const auto qrCodesDictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        const std::string qrCodeDate =
            qrCodesDictionary.at( qrCode1.id ).at( QRCodeFields::LastHotzone ).at( QRCodeFields::Date );
        const auto qrCodesExpectedJson = DetectableBuilder::dictionary(
            qrCode1.id, qrCode1.name, qrCode1.value, qrCodeDate, camera1.hotzonesItemId, hotzoneId1 );
        EXPECT_EQ( qrCodesDictionary.dump(), qrCodesExpectedJson.dump() );
    }
};

TEST_F( VidooManagerTestSuite, multizoneMotion ) /* 5.2 */ {
    using namespace Vidoo4Hub::Resources;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace QRCodeFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId1", "cameraId2" } );
    auto& camera1 = resources.m_cameras.at( "cameraId1" );
    auto& camera2 = resources.m_cameras.at( "cameraId2" );

    dictionaryManager.updateDictionary( camera1.hotzonesItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );
    dictionaryManager.updateDictionary( camera2.hotzonesItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );

    VidooManager vidooManager( resources, dictionaryManager );

    DetectableObject object1( "objectId1" );
    DetectablePerson person1( "personId1" );
    DetectableQRCode qrCode1( "qrCodeId1" );

    const std::string hotzoneId1 = "hotzoneId1";
    const std::string hotzoneName1 = "hotzoneName1";

    {  // initial state
        vidooManager.updateObjectList( MessageBuilder::objectList( { object1 } ) );
        vidooManager.updatePeopleList( MessageBuilder::personList( { person1 } ) );
        vidooManager.updateQRCodeList( MessageBuilder::qrCodeList( { qrCode1 } ) );
        vidooManager.updateHotzoneList( { MessageBuilder::hotzone( camera1.cameraId, hotzoneId1, hotzoneName1 ) } );
    }

    const auto defaultHotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

    {  // object detected in default hotzone
        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } } };
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );
        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );
    }

    {  // person and qr code appeared in default hotzone // object still in default hotzone
        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } },
                                                           { DetectableTypes::Type::Person, { person1.id } },
                                                           { DetectableTypes::Type::QRCode, { qrCode1.id } } };
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );
        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );
    }

    {  // person and qr code moved to first hotzone
        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } } };
        IVidooManager::DetectedObjects detectedPersonsAndQRCodes = {
            { DetectableTypes::Type::Person, { person1.id } }, { DetectableTypes::Type::QRCode, { qrCode1.id } }
        };

        vidooManager.motion( camera1.cameraId, hotzoneId1, detectedPersonsAndQRCodes );
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );
        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, detectedPersonsAndQRCodes );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );
    }

    {  // person and QR code leave
        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } } };

        vidooManager.motion( camera1.cameraId, hotzoneId1, {} );
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, detectedObjects );
        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );
    }

    {  // object leave
        vidooManager.motion( camera1.cameraId, m_defaultHotzoneId, {} );
        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );

        auto hotzonesExpectedJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesExpectedJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, {} );
        hotzonesExpectedJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        EXPECT_EQ( hotzonesDictionary.dump(), hotzonesExpectedJson.dump() );
    }
};

TEST_F( VidooManagerTestSuite, syncUpdatedHotzones ) /* 6.1 */ {
    using namespace Vidoo4Hub::Resources;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace QRCodeFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId1", "cameraId2" } );
    auto& camera1 = resources.m_cameras.at( "cameraId1" );
    auto& camera2 = resources.m_cameras.at( "cameraId2" );

    DetectableObject object1( "objectId1" );
    DetectablePerson person1( "personId1" );
    DetectableQRCode qrCode1( "qrCodeId1" );

    const std::string date = "1234";

    const std::string hotzoneId1 = "hotzoneId1";
    const std::string hotzoneName1 = "hotzoneName1";

    const std::string removedHotzone = "hotzoneId2";

    const auto detectableObjectsItemId = resources.getDetectableItemId( DetectableTypes::Type::Object );
    const auto detectablePersonsItemId = resources.getDetectableItemId( DetectableTypes::Type::Person );
    const auto detectableQRCodesItemId = resources.getDetectableItemId( DetectableTypes::Type::QRCode );

    {  // objects, persons and QR codes have reference to nonexistent second hotzone
        const auto objectJson =
            DetectableBuilder::dictionary( object1.id, object1.name, date, camera1.hotzonesItemId, removedHotzone );
        dictionaryManager.updateDictionary( detectableObjectsItemId, objectJson );

        const auto personJson =
            DetectableBuilder::dictionary( person1.id, person1.name, date, camera1.hotzonesItemId, removedHotzone );
        dictionaryManager.updateDictionary( detectablePersonsItemId, personJson );

        const auto qrCodeJson = DetectableBuilder::dictionary(
            qrCode1.id, qrCode1.name, qrCode1.value, date, camera1.hotzonesItemId, removedHotzone );
        dictionaryManager.updateDictionary( detectableQRCodesItemId, qrCodeJson );
    }

    {  // hotzones state after hotzone ("hotzoneId2") was removed
        auto hotzonesJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        hotzonesJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, {} );
        hotzonesJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, {} );
        dictionaryManager.updateDictionary( camera1.hotzonesItemId, hotzonesJson );
    }

    VidooManager vidooManager( resources, dictionaryManager );
    vidooManager.syncUpdatedHotzones();

    {  // objects, persons and QR codes have switched to default hotzone
        const auto expectedObjectJson =
            DetectableBuilder::dictionary( object1.id, object1.name, date, camera1.hotzonesItemId, m_defaultHotzoneId );
        const auto objectDictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        EXPECT_EQ( objectDictionary.dump(), expectedObjectJson.dump() );

        const auto expectedPersonJson =
            DetectableBuilder::dictionary( person1.id, person1.name, date, camera1.hotzonesItemId, m_defaultHotzoneId );
        const auto personDictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        EXPECT_EQ( personDictionary.dump(), expectedPersonJson.dump() );

        const auto expectedQRCodeJson = DetectableBuilder::dictionary(
            qrCode1.id, qrCode1.name, qrCode1.value, date, camera1.hotzonesItemId, m_defaultHotzoneId );
        const auto qrCodeDictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        EXPECT_EQ( qrCodeDictionary.dump(), expectedQRCodeJson.dump() );
    }

    resources.m_cameras.erase( resources.m_cameras.find( "cameraId1" ) );
    dictionaryManager.clearDictionary( camera1.hotzonesItemId );  // state after camera was removed
    ASSERT_EQ( dictionaryManager.m_dictionaryMap.size(), 3 ) << "objects item + persons item + QR codes item";

    vidooManager.syncUpdatedHotzones();

    {  // objects, persons and QR codes removed nonexistent hotzones item
        const auto expectedObjectJson = DetectableBuilder::dictionary( object1.id, object1.name );
        const auto objectDictionary = dictionaryManager.readDictionary( detectableObjectsItemId );
        EXPECT_EQ( objectDictionary.dump(), expectedObjectJson.dump() );

        const auto expectedPersonJson = DetectableBuilder::dictionary( person1.id, person1.name );
        const auto personDictionary = dictionaryManager.readDictionary( detectablePersonsItemId );
        EXPECT_EQ( personDictionary.dump(), expectedPersonJson.dump() );

        const auto expectedQRCodeJson = DetectableBuilder::dictionary( qrCode1.id, qrCode1.name, qrCode1.value );
        const auto qrCodeDictionary = dictionaryManager.readDictionary( detectableQRCodesItemId );
        EXPECT_EQ( qrCodeDictionary.dump(), expectedQRCodeJson.dump() );
    }
};

TEST_F( VidooManagerTestSuite, syncUpdatedObjectsList ) /* 6.2 */ {
    using namespace Vidoo4Hub::Resources;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;
    namespace ObjectFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace PersonFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;
    namespace QRCodeFields = HubPlatform::CoreD::Model::Resources::DetectableObjectBaseFields;

    FakeDictionaryManager dictionaryManager;

    auto resources = createFakeVidooManagerResources();
    resources.m_cameras = createCameraList( { "cameraId1", "cameraId2" } );
    auto& camera1 = resources.m_cameras.at( "cameraId1" );
    auto& camera2 = resources.m_cameras.at( "cameraId2" );

    DetectableObject object1( "objectId1" );
    DetectablePerson person1( "personId1" );
    DetectableQRCode qrCode1( "qrCodeId1" );

    DetectableObject removedObject2( "objectId2" );
    DetectablePerson removedPerson2( "personId2" );
    DetectableQRCode removedQRCode2( "qrCodeId2" );

    const std::string date = "1234";

    const std::string hotzoneId1 = "hotzoneId1";
    const std::string hotzoneName1 = "hotzoneName1";

    {  // objects, persons and QR codes
        const auto objectJson =
            DetectableBuilder::dictionary( object1.id, object1.name, date, camera1.hotzonesItemId, hotzoneId1 );
        dictionaryManager.updateDictionary( resources.getDetectableItemId( DetectableTypes::Type::Object ),
                                            objectJson );

        const auto personJson =
            DetectableBuilder::dictionary( person1.id, person1.name, date, camera1.hotzonesItemId, hotzoneId1 );
        dictionaryManager.updateDictionary( resources.getDetectableItemId( DetectableTypes::Type::Person ),
                                            personJson );

        const auto qrCodeJson = DetectableBuilder::dictionary(
            qrCode1.id, qrCode1.name, qrCode1.value, date, camera1.hotzonesItemId, hotzoneId1 );
        dictionaryManager.updateDictionary( resources.getDetectableItemId( DetectableTypes::Type::QRCode ),
                                            qrCodeJson );
    }

    {  // hotzones state after one hotzone was removed
        auto hotzonesJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        IVidooManager::DetectedObjects detectedObjects = {
            { DetectableTypes::Type::Object, { object1.id, removedObject2.id } },
            { DetectableTypes::Type::Person, { person1.id, removedPerson2.id } },
            { DetectableTypes::Type::QRCode, { qrCode1.id, removedQRCode2.id } }
        };

        hotzonesJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        hotzonesJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, detectedObjects );
        dictionaryManager.updateDictionary( camera1.hotzonesItemId, hotzonesJson );
    }

    VidooManager vidooManager( resources, dictionaryManager );
    vidooManager.syncUpdatedDetectableObjects();

    {  // hotzones after sync
        IVidooManager::DetectedObjects detectedObjects = { { DetectableTypes::Type::Object, { object1.id } },
                                                           { DetectableTypes::Type::Person, { person1.id } },
                                                           { DetectableTypes::Type::QRCode, { qrCode1.id } } };

        auto expectedHotzonesJson = CameraUtils::getDefaultHotzoneDictionaryJson();
        expectedHotzonesJson[ m_defaultHotzoneId ] = HotzoneBuilder::hotzone( m_defaultHotzoneName, detectedObjects );
        expectedHotzonesJson[ hotzoneId1 ] = HotzoneBuilder::hotzone( hotzoneName1, detectedObjects );

        const auto hotzonesDictionary = dictionaryManager.readDictionary( camera1.hotzonesItemId );
        EXPECT_EQ( hotzonesDictionary.dump(), expectedHotzonesJson.dump() );
    }
};

}  // namespace UT
}  // namespace HubPlatform
