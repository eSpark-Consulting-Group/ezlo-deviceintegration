#include "scenes_manager/scenes_processor/controller_interface/external_controller.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include "test_utils/temporary_root_folder_wrapper.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>

namespace HubPlatform {
namespace UT {

using ::testing::Return;

class MockExtControllerInterface : public ExternalControllers::AbstractInterface {
public:
    MOCK_METHOD2( subscribeOnItemChanges, void( const ControllerID& controllerId, const ItemsIDs& itemsIds ) );
    MOCK_METHOD2( unsubscribeFromItemChanges, void( const ControllerID& controllerId, const ItemsIDs& itemsIds ) );
    MOCK_METHOD2( subscribeOnDevicesChanges, void( const ControllerID& controllerId, const DevicesIDs& devicesIds ) );
    MOCK_METHOD2( unsubscribeFromDevicesChanges,
                  void( const ControllerID& controllerId, const DevicesIDs& devicesIds ) );
    MOCK_CONST_METHOD2( getItem, JsonObject( const ControllerID& controllerId, const ItemID& itemId ) );
    MOCK_CONST_METHOD2( getItems, RawItems( const ControllerID& controllerId, const ItemsIDs& itemsIds ) );
    MOCK_CONST_METHOD2( getItemsBy, RawItems( const ControllerID& controllerId, const JsonObject& condition ) );
    MOCK_CONST_METHOD2( getDevicesBy, RawDevices( const ControllerID& controllerId, const JsonObject& condition ) );
    MOCK_METHOD3( setItemValue,
                  void( const ControllerID& controllerId,
                        const JsonObject& data,
                        SendResponseCallback sendResponseCallback ) );
    MOCK_CONST_METHOD2( getSingleDevice, JsonObject( const ControllerID& controllerId, const DeviceID& deviceId ) );
    MOCK_METHOD3( setDeviceArmed,
                  JsonObject( const ControllerID& controllerId, const DeviceID& deviceId, bool armed ) );
};

class MockCallbacks {
public:
    MOCK_METHOD1( subscribeOnItemChanges, void( const SceneEngine::ControllerInterface::Item& ) );
};

class ExternalControllerTest : public testing::Test {
public:
    using ControllerId = SceneEngine::ControllerInterface::ControllerId;
    using ItemId = SceneEngine::ControllerInterface::ItemId;
    using ItemIds = SceneEngine::ControllerInterface::ItemIds;
    using DeviceIds = SceneEngine::ControllerInterface::DeviceIds;
    using DeviceId = SceneEngine::ControllerInterface::DeviceId;
    using ExternalControllerPtr = std::unique_ptr< ScenesManager::ExternalController >;

    void SetUp() override {}
    void TearDown() override {
        EXPECT_TRUE( isEmptyDatabase() );
    }
    ExternalControllerPtr getExternalController( const ControllerId& controllerId ) {
        return std::make_unique< ScenesManager::ExternalController >(
            controllerId, getExtControllerInterface(), m_service );
    }
    Services::Service& getService() {
        return m_service;
    }
    IDbClient& getDbClient() {
        return getService().getDBClient();
    }
    Services::Message getBusMessage() {
        return m_service.getFakeResponseBus().getMessage();
    }
    MockExtControllerInterface& getExtControllerInterface() {
        return m_extControllerInterface;
    }
    void itemAdded( ScenesManager::ExternalController& extController,
                    const SceneEngine::ControllerInterface::ItemId& itemId ) {
        extController.itemAdded( JSON( "_id" << itemId ) );
    }
    void deviceAdded( ScenesManager::ExternalController& extController,
                      const SceneEngine::ControllerInterface::DeviceId& deviceId ) {
        extController.deviceAdded( JSON( "_id" << deviceId ) );
    }
    bool isEmptyDatabase() {
        return getDbClient().findAll( HaScenesControllerSubscriptionDBName, JSON_EMPTY_OBJ, nullptr ).empty();
    }
    std::string getDatabaseItemState( const ControllerId& controllerId, const ItemId& itemId ) {
        auto fieldsToReturn = JSON( "state" << 1 );
        auto result = getDbClient().findOne( HaScenesControllerSubscriptionDBName,
                                             JSON( "controllerId" << controllerId << "itemId" << itemId ),
                                             &fieldsToReturn );
        return json_get_str( result, "state" );
    }
    std::string getDatabaseDeviceState( const ControllerId& controllerId, const DeviceId& deviceId ) {
        auto fieldsToReturn = JSON( "state" << 1 );
        auto result = getDbClient().findOne( HaScenesControllerSubscriptionDBName,
                                             JSON( "controllerId" << controllerId << "deviceId" << deviceId ),
                                             &fieldsToReturn );
        return json_get_str( result, "state" );
    }

private:
    Services::UT::FakeService m_service;
    MockExtControllerInterface m_extControllerInterface;
    Tests::TemporaryRootFolderWrapper m_folderWrapper;
};

TEST_F( ExternalControllerTest, itemSubscribeFullCircle ) {
    using namespace std::placeholders;
    MockCallbacks callbacks;
    auto extController = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_2" } ) ) )
        .Times( 1 );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_3" } ) ) )
        .Times( 1 );
    EXPECT_CALL( callbacks, subscribeOnItemChanges ).Times( 0 );
    extController->subscribeOnItemChanges( "itemId_1",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController->subscribeOnItemChanges( "itemId_2",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController->subscribeOnItemChanges( "itemId_1",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController->subscribeOnItemChanges( "itemId_3",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController->subscribeOnItemChanges( "itemId_1",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController->subscribeOnItemChanges( "itemId_2",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "wait" );
    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_2" ), "wait" );
    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_3" ), "wait" );

    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 3 );
    itemAdded( *extController, "itemId_1" );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_2" ) ) )
        .Times( 2 );
    itemAdded( *extController, "itemId_2" );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_3" ) ) )
        .Times( 1 );
    itemAdded( *extController, "itemId_3" );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "subscribed" );
    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_2" ), "subscribed" );
    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_3" ), "subscribed" );

    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_1", "itemId_1" ) )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_1" ) ) );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    extController->subscribeOnItemChanges( "itemId_1",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_1", "itemId_2" ) )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_2" ) ) );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_2" ) ) )
        .Times( 1 );
    extController->subscribeOnItemChanges( "itemId_2",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_1", "itemId_3" ) )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_3" ) ) );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_3" ) ) )
        .Times( 1 );
    extController->subscribeOnItemChanges( "itemId_3",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    auto& service = getService();
    service.addTask( [&service]() { service.stop(); } );
    service.run();

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 4; ++i ) {
        extController->unsubscribeFromItemChanges( "itemId_1" );
    }
    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_2" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 3; ++i ) {
        extController->unsubscribeFromItemChanges( "itemId_2" );
    }
    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_3" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 2; ++i ) {
        extController->unsubscribeFromItemChanges( "itemId_3" );
    }

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "" );
    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_2" ), "" );
    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_3" ), "" );
}

TEST_F( ExternalControllerTest, itemUnsubscribeOnWaitState ) {
    using namespace std::placeholders;
    MockCallbacks callbacks;
    auto extController = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( callbacks, subscribeOnItemChanges ).Times( 0 );
    extController->subscribeOnItemChanges( "itemId_1",
                                           std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "wait" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .WillOnce( Return() );
    extController->unsubscribeFromItemChanges( "itemId_1" );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "" );

    itemAdded( *extController, "itemId_1" );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "" );
}

TEST_F( ExternalControllerTest, itemSubscribeSeveralControllers ) {
    using namespace std::placeholders;
    MockCallbacks callbacks;
    auto extController1 = getExternalController( "extController_1" );
    auto extController2 = getExternalController( "extController_2" );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_2", ItemIds( { "itemId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( callbacks, subscribeOnItemChanges ).Times( 0 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController2->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "wait" );
    EXPECT_EQ( getDatabaseItemState( "extController_2", "itemId_1" ), "wait" );

    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    itemAdded( *extController1, "itemId_1" );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    itemAdded( *extController2, "itemId_1" );

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "subscribed" );
    EXPECT_EQ( getDatabaseItemState( "extController_2", "itemId_1" ), "subscribed" );

    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_1", "itemId_1" ) )
        .Times( 1 )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_1" ) ) );
    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_2", "itemId_1" ) )
        .Times( 1 )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_1" ) ) );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 2 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    extController2->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    auto& service = getService();
    service.addTask( [&service]() { service.stop(); } );
    service.run();

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 2; ++i ) {
        extController1->unsubscribeFromItemChanges( "itemId_1" );
    }
    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_2", ItemIds( { "itemId_1" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 2; ++i ) {
        extController2->unsubscribeFromItemChanges( "itemId_1" );
    }

    EXPECT_EQ( getDatabaseItemState( "extController_1", "itemId_1" ), "" );
    EXPECT_EQ( getDatabaseItemState( "extController_2", "itemId_1" ), "" );
}

TEST_F( ExternalControllerTest, itemSubscribeRebootOnWaitState ) {
    using namespace std::placeholders;
    MockCallbacks callbacks;
    auto extController1 = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( callbacks, subscribeOnItemChanges ).Times( 0 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    extController1 = getExternalController( "extController_1" );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );
    itemAdded( *extController1, "itemId_1" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .WillOnce( Return() );
    extController1->unsubscribeFromItemChanges( "itemId_1" );
}

TEST_F( ExternalControllerTest, itemSubscribeRebootOnSubscribedState ) {
    using namespace std::placeholders;
    MockCallbacks callbacks;
    auto extController1 = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(), subscribeOnItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( callbacks, subscribeOnItemChanges ).Times( 0 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    itemAdded( *extController1, "itemId_1" );

    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_1", "itemId_1" ) )
        .Times( 1 )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_1" ) ) );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    auto& service = getService();
    service.addTask( [&service]() { service.stop(); } );
    service.run();

    extController1 = getExternalController( "extController_1" );

    EXPECT_CALL( getExtControllerInterface(), getItem( "extController_1", "itemId_1" ) )
        .Times( 1 )
        .WillOnce( Return( JSON( "_id"
                                 << "itemId_1" ) ) );
    EXPECT_CALL( callbacks,
                 subscribeOnItemChanges( JSON( "_id"
                                               << "itemId_1" ) ) )
        .Times( 1 );
    extController1->subscribeOnItemChanges( "itemId_1",
                                            std::bind( &MockCallbacks::subscribeOnItemChanges, &callbacks, _1 ) );

    service.addTask( [&service]() { service.stop(); } );
    service.run();

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromItemChanges( "extController_1", ItemIds( { "itemId_1" } ) ) )
        .WillOnce( Return() );
    extController1->unsubscribeFromItemChanges( "itemId_1" );
}

TEST_F( ExternalControllerTest, deviceSubscribeFullCircle ) {
    auto extController = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_2" } ) ) )
        .Times( 1 );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_3" } ) ) )
        .Times( 1 );
    extController->subscribeOnDevicesChanges( "deviceId_1" );
    extController->subscribeOnDevicesChanges( "deviceId_2" );
    extController->subscribeOnDevicesChanges( "deviceId_1" );
    extController->subscribeOnDevicesChanges( "deviceId_3" );
    extController->subscribeOnDevicesChanges( "deviceId_1" );
    extController->subscribeOnDevicesChanges( "deviceId_2" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "wait" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_2" ), "wait" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_3" ), "wait" );

    deviceAdded( *extController, "deviceId_1" );
    deviceAdded( *extController, "deviceId_2" );
    deviceAdded( *extController, "deviceId_3" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "subscribed" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_2" ), "subscribed" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_3" ), "subscribed" );

    extController->subscribeOnDevicesChanges( "deviceId_1" );
    extController->subscribeOnDevicesChanges( "deviceId_2" );
    extController->subscribeOnDevicesChanges( "deviceId_3" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 4; ++i ) {
        extController->unsubscribeFromDevicesChanges( "deviceId_1" );
    }
    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_2" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 3; ++i ) {
        extController->unsubscribeFromDevicesChanges( "deviceId_2" );
    }
    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_3" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 2; ++i ) {
        extController->unsubscribeFromDevicesChanges( "deviceId_3" );
    }

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_2" ), "" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_3" ), "" );
}

TEST_F( ExternalControllerTest, deviceUnsubscribeOnWaitState ) {
    auto extController = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .Times( 1 );
    extController->subscribeOnDevicesChanges( "deviceId_1" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "wait" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .WillOnce( Return() );
    extController->unsubscribeFromDevicesChanges( "deviceId_1" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "" );

    deviceAdded( *extController, "deviceId_1" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "" );
}

TEST_F( ExternalControllerTest, deviceSubscribeSeveralControllers ) {
    auto extController1 = getExternalController( "extController_1" );
    auto extController2 = getExternalController( "extController_2" );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .Times( 1 );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_2", DeviceIds( { "deviceId_1" } ) ) )
        .Times( 1 );
    extController1->subscribeOnDevicesChanges( "deviceId_1" );
    extController2->subscribeOnDevicesChanges( "deviceId_1" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "wait" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_2", "deviceId_1" ), "wait" );

    deviceAdded( *extController1, "deviceId_1" );
    deviceAdded( *extController2, "deviceId_1" );

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "subscribed" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_2", "deviceId_1" ), "subscribed" );

    extController1->subscribeOnDevicesChanges( "deviceId_1" );
    extController2->subscribeOnDevicesChanges( "deviceId_1" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 2; ++i ) {
        extController1->unsubscribeFromDevicesChanges( "deviceId_1" );
    }
    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_2", DeviceIds( { "deviceId_1" } ) ) )
        .WillOnce( Return() );
    for( int i = 0; i < 2; ++i ) {
        extController2->unsubscribeFromDevicesChanges( "deviceId_1" );
    }

    EXPECT_EQ( getDatabaseDeviceState( "extController_1", "deviceId_1" ), "" );
    EXPECT_EQ( getDatabaseDeviceState( "extController_2", "deviceId_1" ), "" );
}

TEST_F( ExternalControllerTest, deviceSubscribeRebootOnWaitState ) {
    auto extController1 = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .Times( 1 );
    extController1->subscribeOnDevicesChanges( "deviceId_1" );

    extController1 = getExternalController( "extController_1" );
    extController1->subscribeOnDevicesChanges( "deviceId_1" );
    deviceAdded( *extController1, "deviceId_1" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .WillOnce( Return() );
    extController1->unsubscribeFromDevicesChanges( "deviceId_1" );
}

TEST_F( ExternalControllerTest, deviceSubscribeRebootOnSubscribedState ) {
    auto extController1 = getExternalController( "extController_1" );
    EXPECT_CALL( getExtControllerInterface(),
                 subscribeOnDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .Times( 1 );
    extController1->subscribeOnDevicesChanges( "deviceId_1" );

    deviceAdded( *extController1, "deviceId_1" );

    extController1->subscribeOnDevicesChanges( "deviceId_1" );

    extController1 = getExternalController( "extController_1" );

    extController1->subscribeOnDevicesChanges( "deviceId_1" );

    EXPECT_CALL( getExtControllerInterface(),
                 unsubscribeFromDevicesChanges( "extController_1", DeviceIds( { "deviceId_1" } ) ) )
        .WillOnce( Return() );
    extController1->unsubscribeFromDevicesChanges( "deviceId_1" );
}

}  // namespace UT
}  // namespace HubPlatform
