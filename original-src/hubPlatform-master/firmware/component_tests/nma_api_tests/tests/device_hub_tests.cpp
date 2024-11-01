#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. hub.device.room.set
DONE    2. hub.device.armed.set
DONE    3. hub.device.name.set
DONE    4. hub.device.check
DONE        4.1 No device id
DONE        4.2 Wrong check value
DONE        4.3 Wrong itemIds value
DONE        4.4 No device
DONE        4.5 Device check not supported
DONE        4.6 Successful hub.device.checked broadcast
DONE        4.7 Failed hub.device.checked broadcast

*/

class DeviceHubTest : public CoreNmaTest {
protected:
    using GatewayID = std::string;
    using DeviceID = std::string;

    void SetUp() override;
    GatewayID addGateway() {
        CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
        return getMainGateway( coreRemoteInterface, __LINE__ );
    }

    DeviceID addDevice( const GatewayID& gatewayId ) {
        CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
        JsonObject broadcast;
        DeviceID id;
        CoreD::Model::Device device1(
            gatewayId, "testDevice1", "smoke", "134_3_24", "security_sensor", false /*batteryPowered*/ );
        device1.setSubcategory( "smoke" );
        auto deviceAddingResult = coreRemoteInterface.addDevice( device1 );
        broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "add device" ) );

        if( json_get_str( broadcast, "msg_subclass" ) == "hub.device.added" )
            if( deviceAddingResult.second )
                id = deviceAddingResult.first;

        return id;
    }

    static std::string getGatewayId( const std::string& name = "model_factory" ) {
        CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
        auto gateways = coreRemoteInterface.getGateways();
        for( const auto& gateway : gateways ) {
            if( json_get_str( gateway, "name" ) == name ) {
                return json_get_str( gateway, "_id" );
            }
        }
        return std::string();
    }

    static void checkBroadcastDeviceUpdatedRoomChanged( const Json& broadcast, const std::string& message ) {
        EXPECT_TRUE( broadcast.count( "result" ) > 0 ) << message;
        EXPECT_TRUE( broadcast[ "result" ].count( "roomId" ) > 0 ) << message;
        EXPECT_EQ( json_get_bool( broadcast[ "result" ], "syncNotification", false ), true )
            << ( message.empty() ? message : std::string{ "Should be true for roomId change" } );
    }
};

void DeviceHubTest::SetUp() {
    CommonNmaTests::SetUp();
    callingFile( __FILE__ );
}

TEST_F( DeviceHubTest, setRoomDeviceHub ) {
    auto gatewayId = addGateway();
    ASSERT_NE( gatewayId, GatewayID() );
    auto device1Id = addDevice( gatewayId );
    ASSERT_NE( device1Id, DeviceID() );

    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    JsonObject broadcast;
    // add new subdevice
    CoreD::Model::Device device2(
        gatewayId, "testDevice2", "smoke", "134_3_24", "security_sensor", false /*batteryPowered*/ );
    device2.setSubcategory( "smoke" );
    device2.setParentDeviceId( device1Id );
    auto deviceAddingResult = coreRemoteInterface.addDevice( device2 );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "device added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto device2Id = deviceAddingResult.first;

    // add new room to core
    JsonObject room = coreRemoteInterface.createRoom( "newRoom" );
    broadcast = waitForBroadcast( "hub.room.created", NMA_TEST_MAKE_MESSAGE( "room created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.room.created" );
    std::string roomId = json_get_str( room, "_id" );
    EXPECT_FALSE( roomId.empty() );

    JsonObject room2 = coreRemoteInterface.createRoom( "newRoom2" );
    broadcast = waitForBroadcast( "hub.room.created", NMA_TEST_MAKE_MESSAGE( "room created" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.room.created" );
    std::string room2Id = json_get_str( room, "_id" );
    EXPECT_FALSE( room2Id.empty() );

    // place device with subdevice into room
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.room.set" }, { "params", JSON( "_id" << device1Id << "roomId" << roomId ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );
        broadcast =
            waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "ignoring hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        broadcast =
            waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "ignoring hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        auto deviceInfo1 = coreRemoteInterface.getSingleDevice( device1Id );
        std::string roomIdNew;
        EXPECT_TRUE( json_get( deviceInfo1, "roomId", roomIdNew ) );
        EXPECT_EQ( roomId, roomIdNew );

        auto deviceInfo2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get( deviceInfo2, "roomId", roomIdNew ) );
        EXPECT_EQ( roomId, roomIdNew );
    }

    // unplace device with subdevice from room ( roomid "" or "0" )
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.device.room.set" },
                                                           { "params",
                                                             JSON( "_id" << device1Id << "roomId"
                                                                         << "" ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );
        broadcast =
            waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "ignoring hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        broadcast =
            waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "ignoring hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        auto deviceInfo1 = coreRemoteInterface.getSingleDevice( device1Id );
        std::string roomIdNew;
        EXPECT_TRUE( json_get( deviceInfo1, "roomId", roomIdNew ) );
        EXPECT_EQ( "", roomIdNew );

        auto deviceInfo2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get( deviceInfo2, "roomId", roomIdNew ) );
        EXPECT_EQ( "", roomIdNew );
    }

    // place device into absent room
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.device.room.set" },
                                                           { "params",
                                                             JSON( "_id" << device1Id << "roomId"
                                                                         << "testRoom" ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        auto error = answer[ "error" ];
        EXPECT_EQ( error[ "data" ].dump(), "\"nma.room.notfound\"" );
        auto deviceInfo = coreRemoteInterface.getSingleDevice( device1Id );
        std::string roomIdNew;
        EXPECT_TRUE( json_get( deviceInfo, "roomId", roomIdNew ) );
        EXPECT_EQ( "", roomIdNew );
    }

    // place alone subdevice into room
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.room.set" }, { "params", JSON( "_id" << device2Id << "roomId" << room2Id ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        auto error = answer[ "error" ];
        EXPECT_FALSE( error.empty() );
        EXPECT_EQ( error[ "data" ].dump(), "\"ezlo.device.forbidden.place.subdevice\"" );
        auto deviceInfo = coreRemoteInterface.getSingleDevice( device2Id );
        std::string roomIdNew;
        EXPECT_TRUE( json_get( deviceInfo, "roomId", roomIdNew ) );
        EXPECT_EQ( "", roomIdNew );
    }

    // place device with subdevice into room again
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.room.set" }, { "params", JSON( "_id" << device1Id << "roomId" << roomId ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );
        broadcast =
            waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "ignoring hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        broadcast =
            waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "ignoring hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        auto deviceInfo1 = coreRemoteInterface.getSingleDevice( device1Id );
        std::string roomIdNew;
        EXPECT_TRUE( json_get( deviceInfo1, "roomId", roomIdNew ) );
        EXPECT_EQ( roomId, roomIdNew );

        auto deviceInfo2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get( deviceInfo2, "roomId", roomIdNew ) );
        EXPECT_EQ( roomId, roomIdNew );
    }

    // delete room (unplace all devices from room)
    {
        const auto answer =
            nma().sendtoHubAndGetAnswer( { { "method", "hub.room.delete" }, { "params", JSON( "_id" << roomId ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );
        broadcast = waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        broadcast = waitForBroadcast( "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "hub.device.updated broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        checkBroadcastDeviceUpdatedRoomChanged( broadcast,
                                                NMA_TEST_MAKE_MESSAGE( "Should be true for roomId change" ) );

        broadcast = waitForBroadcast( "hub.room.deleted", NMA_TEST_MAKE_MESSAGE( "room deleted broadcast" ) );

        auto deviceInfo1 = coreRemoteInterface.getSingleDevice( device1Id );
        std::string roomIdNew;
        EXPECT_TRUE( json_get( deviceInfo1, "roomId", roomIdNew ) );
        EXPECT_EQ( roomIdNew, "" );

        auto deviceInfo2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get( deviceInfo2, "roomId", roomIdNew ) );
        EXPECT_EQ( roomIdNew, "" );
    }

    // remove devices
    coreRemoteInterface.removeDevice( device1Id );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "hub.device.removed broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );

    coreRemoteInterface.removeDevice( device2Id );
    broadcast = waitForBroadcast( "hub.device.removed", NMA_TEST_MAKE_MESSAGE( "hub.device.removed broadcast" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( DeviceHubTest, setDeviceArmed ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    auto gatewayId = getMainGateway( coreRemoteInterface, __LINE__ );
    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeId != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }
    auto houseMode = coreRemoteInterface.getHouseModeById( "1" );
    houseModeId = "1";

    // add armed device1 and disarmed device2 in HouseMode 1;
    CoreD::Model::Device device01(
        gatewayId, "testDevice1", "smoke", "134_3_24", "security_sensor", false /*batteryPowered*/ );
    device01.setSubcategory( "smoke" );
    auto deviceAddingResult = coreRemoteInterface.addDevice( device01 );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "device added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto device1Id = deviceAddingResult.first;

    using namespace CoreD::Model::Resources;

    auto securityItemId1 = addItem(
        coreRemoteInterface, device1Id, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( device1Id, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    CoreD::Model::Device device02(
        gatewayId, "testDevice2", "motion", "134_3_24", "security_sensor", false /*batteryPowered*/ );
    device02.setSubcategory( "motion" );
    deviceAddingResult = coreRemoteInterface.addDevice( device02 );
    broadcast = waitForBroadcast( "hub.device.added", NMA_TEST_MAKE_MESSAGE( "device added" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    ASSERT_TRUE( deviceAddingResult.second );
    auto device2Id = deviceAddingResult.first;

    auto securityItemId2 = addItem(
        coreRemoteInterface, device2Id, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( device2Id, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    // check status devices in HouseMode 1;
    {
        auto device1 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_TRUE( json_get_bool( device1, "armed" ) );
        auto device2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_FALSE( json_get_bool( device2, "armed" ) );
        auto houseMode = coreRemoteInterface.getHouseModeById( "1" );
    }
    // set armed device1 and then disarmed and check status in HouseMode 1;
    {
        const auto answer1 = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.armed.set" }, { "params", JSON( "_id" << device1Id << "armed" << true ) } } );
        EXPECT_TRUE( answer1[ "result" ].empty() );
        EXPECT_TRUE( answer1[ "error" ].empty() );
        NMA_TEST_WAIT_NO_BROADCASTS();
        auto device10 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_TRUE( json_get_bool( device10, "armed" ) );

        const auto answer2 = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.armed.set" }, { "params", JSON( "_id" << device1Id << "armed" << false ) } } );
        EXPECT_TRUE( answer2[ "result" ].empty() );
        EXPECT_TRUE( answer2[ "error" ].empty() );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        auto device11 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_FALSE( json_get_bool( device11, "armed" ) );

        const auto answer3 = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.armed.set" }, { "params", JSON( "_id" << device2Id << "armed" << true ) } } );
        EXPECT_TRUE( answer3[ "result" ].empty() );
        EXPECT_TRUE( answer3[ "error" ].empty() );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        auto device20 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get_bool( device20, "armed" ) );

        const auto answer4 = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.armed.set" }, { "params", JSON( "_id" << device2Id << "armed" << false ) } } );
        EXPECT_TRUE( answer4[ "result" ].empty() );
        EXPECT_TRUE( answer4[ "error" ].empty() );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        auto device21 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_FALSE( json_get_bool( device21, "armed" ) );
    }
    // set HouseMode to 2 (disarmed devices update to armed during switch houseMode );
    {
        // set switch_to_delay to 0 before running the test
        auto switchDelayPeriod = coreRemoteInterface.getSwitchDelay();
        coreRemoteInterface.setSwitchDelay( 0 );

        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.switch" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "2" ) } } );
        coreRemoteInterface.setSwitchDelay( switchDelayPeriod );

        testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
        broadcast = waitForBroadcast(
            "hub.device.updated", NMA_TEST_MAKE_MESSAGE( "device updated" ), std::chrono::seconds( 35 ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );

        auto device10 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_TRUE( json_get_bool( device10, "armed" ) );
        auto device20 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get_bool( device20, "armed" ) );

        const auto answer1 = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.armed.set" }, { "params", JSON( "_id" << device1Id << "armed" << false ) } } );
        EXPECT_TRUE( answer1[ "result" ].empty() );
        EXPECT_TRUE( answer1[ "error" ].empty() );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        auto device11 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_FALSE( json_get_bool( device11, "armed" ) );

        const auto answer2 = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.device.armed.set" }, { "params", JSON( "_id" << device2Id << "armed" << false ) } } );
        EXPECT_TRUE( answer2[ "result" ].empty() );
        EXPECT_TRUE( answer2[ "error" ].empty() );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        auto device21 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_FALSE( json_get_bool( device21, "armed" ) );
    }

    // remove devices
    auto itemsNumber = coreRemoteInterface.getItemsByDeviceId( device1Id ).size();
    removeDevice( coreRemoteInterface, device1Id, itemsNumber, 0, __LINE__ );

    itemsNumber = coreRemoteInterface.getItemsByDeviceId( device2Id ).size();
    removeDevice( coreRemoteInterface, device2Id, itemsNumber, 0, __LINE__ );
}

TEST_F( DeviceHubTest, setDeviceEmptyName ) {
    auto gatewayId = addGateway();
    ASSERT_NE( gatewayId, GatewayID() );
    auto deviceId = addDevice( gatewayId );
    ASSERT_NE( deviceId, DeviceID() );

    // Set device empty name
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.device.name.set" },
                                                 { "params",
                                                   JSON( "_id" << deviceId << "name"
                                                               << "" ) } } );
    Json errorJson;
    auto assertAnswerHasErrorAndGetError = [&] {
        ASSERT_TRUE( json_get_obj( answer, "error", errorJson ) )
            << "Setting empty name to device or name that contains only spaces returns no error!";
    };
    auto assertErrorCode = [&] {
        ASSERT_EQ( json_get_int( errorJson, "code" ), static_cast< int >( Errors::Codes::INVALID_REQUEST ) )
            << "Setting empty name to device or name that contains only spaces returns incorrect error code!";
    };
    assertAnswerHasErrorAndGetError();
    assertErrorCode();

    // Set device name that consists of whitespaces
    answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.device.name.set" },
                                            { "params",
                                              JSON( "_id" << deviceId << "name"
                                                          << "  " ) } } );
    assertAnswerHasErrorAndGetError();
    assertErrorCode();

    // remove device
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;
    coreRemoteInterface.removeDevice( deviceId );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.removed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.removed" );
}

TEST_F( DeviceHubTest, hubDeviceCheckNoDeviceId /* 4.1 */ ) {
    auto deviceId = addDevice( getGatewayId() );
    ASSERT_NE( deviceId, DeviceID() );

    auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                           "   \"method\": \"hub.device.check\","
                                                           "   \"params\": {}"
                                                           "}" ) );

    EXPECT_EQ( "{\n"
               "    \"code\": -32600,\n"
               "    \"data\": \"rpc.params.notfound._id\",\n"
               "    \"message\": \"Bad request, _id does not exist\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
    EXPECT_EQ( "{}", answer[ "result" ].dump( 4 ) );
}

TEST_F( DeviceHubTest, hubDeviceCheckWrongCheckValue /* 4.2 */ ) {
    auto deviceId = addDevice( getGatewayId() );
    ASSERT_NE( deviceId, DeviceID() );

    auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                           "   \"method\": \"hub.device.check\","
                                                           "   \"params\": {"
                                                           "       \"_id\": \"" +
                                                           deviceId +
                                                           "\","
                                                           "       \"check\": \"nothing\""
                                                           "   }"
                                                           "}" ) );
    EXPECT_EQ( "{\n"
               "    \"code\": -32602,\n"
               "    \"data\": \"rpc.params.range.invalid.check\",\n"
               "    \"message\": \"Value is not in valid range\",\n"
               "    \"reason\": \"Possible values: value, status\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
    EXPECT_EQ( "{}", answer[ "result" ].dump( 4 ) );
}

TEST_F( DeviceHubTest, hubDeviceCheckWrongItemIdsValue /* 4.3 */ ) {
    auto deviceId = addDevice( getGatewayId() );
    ASSERT_NE( deviceId, DeviceID() );

    {
        auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                               "   \"method\": \"hub.device.check\","
                                                               "   \"params\": {"
                                                               "       \"_id\": \"" +
                                                               deviceId +
                                                               "\","
                                                               "       \"items\": \"string\""
                                                               "   }"
                                                               "}" ) );

        EXPECT_EQ( "{\n"
                   "    \"code\": -32600,\n"
                   "    \"data\": \"rpc.params.notfound.items\",\n"
                   "    \"message\": \"Bad request, items does not exist\"\n"
                   "}",
                   answer[ "error" ].dump( 4 ) );
        EXPECT_EQ( "{}", answer[ "result" ].dump( 4 ) );
    }

    {
        auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                               "   \"method\": \"hub.device.check\","
                                                               "   \"params\": {"
                                                               "       \"_id\": \"" +
                                                               deviceId +
                                                               "\","
                                                               "       \"items\": [ 4, 2 ]"
                                                               "   }"
                                                               "}" ) );

        EXPECT_EQ( "{\n"
                   "    \"code\": -32602,\n"
                   "    \"data\": \"rpc.params.invalid.items\",\n"
                   "    \"message\": \"Wrong params, items\"\n"
                   "}",
                   answer[ "error" ].dump( 4 ) );
        EXPECT_EQ( "{}", answer[ "result" ].dump( 4 ) );
    }
}

TEST_F( DeviceHubTest, hubDeviceCheckNoDevice /* 4.4 */ ) {
    auto deviceId = addDevice( getGatewayId() );
    ASSERT_NE( deviceId, DeviceID() );

    auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                           "   \"method\": \"hub.device.check\","
                                                           "   \"params\": {"
                                                           "       \"_id\": \"missingDeviceId\""
                                                           "   }"
                                                           "}" ) );

    EXPECT_EQ( "{\n"
               "    \"code\": -32500,\n"
               "    \"data\": \"ezlo.device.notfound\",\n"
               "    \"message\": \"No device found!\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
    EXPECT_EQ( "{}", answer[ "result" ].dump( 4 ) );
}

TEST_F( DeviceHubTest, hubDeviceCheckDeviceCheckNotSupported /* 4.5 */ ) {
    auto gatewayId = getGatewayId( "item_control" );
    ASSERT_NE( gatewayId, GatewayID() );
    auto deviceId = addDevice( gatewayId );
    ASSERT_NE( deviceId, DeviceID() );

    auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                           "   \"method\": \"hub.device.check\","
                                                           "   \"params\": {"
                                                           "       \"_id\": \"" +
                                                           deviceId +
                                                           "\""
                                                           "   }"
                                                           "}" ) );

    EXPECT_EQ( "{\n"
               "    \"code\": -32500,\n"
               "    \"data\": \"hub.device.check.unsupported\",\n"
               "    \"message\": \"The device doesn't support checking values or status.\"\n"
               "}",
               answer[ "error" ].dump( 4 ) );
    EXPECT_EQ( "{}", answer[ "result" ].dump( 4 ) );
}

TEST_F( DeviceHubTest, hubDeviceCheckSuccessfulBroadcast /* 4.6 */ ) {
    auto deviceId = addDevice( getGatewayId() );
    ASSERT_NE( deviceId, DeviceID() );

    auto answer = nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                           "   \"method\": \"hub.device.check\","
                                                           "   \"params\": {"
                                                           "       \"_id\": \"" +
                                                           deviceId +
                                                           "\""
                                                           "   }"
                                                           "}" ) );

    ASSERT_EQ( "null", answer[ "error" ].dump( 4 ) );
    ASSERT_EQ( "{}", answer[ "result" ].dump( 4 ) );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( "hub.device.checked" );
    EXPECT_EQ( "{\n"
               "    \"id\": \"ui_broadcast\",\n"
               "    \"msg_subclass\": \"hub.device.checked\",\n"
               "    \"result\": {\n"
               "        \"completed\": 100,\n"
               "        \"deviceId\": \"" +
                   deviceId +
                   "\",\n"
                   "        \"status\": \"finished\"\n"
                   "    }\n"
                   "}",
               broadcast.dump( 4 ) );
}

TEST_F( DeviceHubTest, hubDeviceCheckFailedBroadcast /* 4.6 */ ) {
    auto deviceId = addDevice( getGatewayId() );
    ASSERT_NE( deviceId, DeviceID() );

    auto answer =
        nma().sendtoHubAndGetAnswer( PARSE_JSON( "{"
                                                 "   \"method\": \"hub.device.check\","
                                                 "   \"params\": {"
                                                 "       \"_id\": \"" +
                                                 deviceId +
                                                 "\","
                                                 "       \"items\": [ \"-42\", \"error.data\", \"error message\" ]"
                                                 "   }"
                                                 "}" ) );

    ASSERT_EQ( "null", answer[ "error" ].dump( 4 ) );
    ASSERT_EQ( "{}", answer[ "result" ].dump( 4 ) );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST_WITHOUT_MSG_ID( "hub.device.checked" );
    EXPECT_EQ( "{\n"
               "    \"id\": \"ui_broadcast\",\n"
               "    \"msg_subclass\": \"hub.device.checked\",\n"
               "    \"result\": {\n"
               "        \"deviceId\": \"" +
                   deviceId +
                   "\",\n"
                   "        \"error\": {\n"
                   "            \"code\": -42,\n"
                   "            \"data\": \"error.data\",\n"
                   "            \"message\": \"error message\"\n"
                   "        },\n"
                   "        \"status\": \"failed\"\n"
                   "    }\n"
                   "}",
               broadcast.dump( 4 ) );
}

}  // namespace Tests
}  // namespace HubPlatform
