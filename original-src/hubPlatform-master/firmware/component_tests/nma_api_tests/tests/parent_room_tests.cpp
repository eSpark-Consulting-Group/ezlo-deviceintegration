#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN

DONE    1. test add parentRoom to device info
DONE       1.1 property in hub.devices.list answer (parent device)
DONE       1.2 property in hub.devices.list answer (child device)
DONE       1.3 check set parentRoom parameter to parent device
DONE       1.4 check deviceAsJson with set parenRoom for parent device
DONE       1.5 check device.added and device.updated broadcasts after addDevice and modifyDevice commands
DONE    2. hub.device.room.set method
DONE       2.1 check hub.device.room.set all cases
DONE       2.2 check hub.device.room.set error param 'separateChildDevices'
DONE    3. hub.room.delete method
DONE       3.1 check hub.room.delete with separate parentRoom parameter value
DONE       3.2 check hub.room.delete in different rooms
*/

static const char* const HubDeviceUpdatedBroadcast = "hub.device.updated";
static const char* const RoomIdKey = "roomId";
static const bool ParentRoom = true;
static const bool SeparateRoom = false;

class ParentRoomTest : public CoreNmaTest {
protected:
    void SetUp() override;
    void TearDown() override;

public:
    static void checkDeviceAsJsonWithDevice( const JsonObject& device );
    static JsonObject createDeviceJson();
    void removeDevice( const std::string& deviceId, int codeLine );
    static void setParentRoomCommand( const std::string& deviceId,
                                      const std::string& roomId,
                                      const std::vector< std::string >& separateChildDevices,
                                      bool isError,
                                      int codeLine );
    static void checkRoomInDevice( const std::string& deviceId, const std::string& roomId, int codeLin );
    void deleteRoomCommand( const std::string& roomId, int codeLine );
    std::string setRoomCreateCommand( const std::string& name, int codeLine );

private:
    using Device = HubPlatform::CoreD::Model::Device;
};

void ParentRoomTest::SetUp() {
    CommonNmaTests::SetUp();
    callingFile( __FILE__ );
}

void ParentRoomTest::TearDown() {
    clearExistingDevices( __LINE__ );
}

void ParentRoomTest::checkDeviceAsJsonWithDevice( const JsonObject& device ) {
    Device::checkDeviceAsJson( device );
}

void ParentRoomTest::removeDevice( const std::string& deviceId, int codeLine ) {
    auto& remoteInterface = makeRemote( nma() );
    auto itemsNumber = remoteInterface.getItemsByDeviceId( deviceId ).size();
    CoreNmaTest::removeDevice( remoteInterface, deviceId, itemsNumber, 0, codeLine );
}

void ParentRoomTest::setParentRoomCommand( const std::string& deviceId,
                                           const std::string& roomId,
                                           const std::vector< std::string >& separateChildDevices,
                                           bool isError,
                                           int codeLine ) {
    auto& remoteInterface = makeRemote( nma() );

    JsonArrayBuilder arr;

    auto codeLineStr = std::to_string( codeLine );

    for( const auto& device : separateChildDevices ) {
        arr << device;
    }

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.device.room.set" },
          { "params", JSON( "_id" << deviceId << "roomId" << roomId << "separateChildDevices" << arr ) } } );
    const auto& error = answer[ "error" ];

    if( !isError ) {
        EXPECT_TRUE( error.empty() ) << codeLineStr;
    }
    else {
        EXPECT_FALSE( error.empty() ) << codeLineStr;
        EXPECT_EQ( json_get_str( answer[ "error" ], "message" ), "Placing subdevice is forbidden" ) << codeLineStr;
    }
}

std::string ParentRoomTest::setRoomCreateCommand( const std::string& name, int codeLine ) {
    auto& remoteInterface = makeRemote( nma() );

    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.room.create" }, { "params", JSON( "name" << name ) } } );
    const auto& error = answer[ "error" ];
    EXPECT_TRUE( error.empty() );
    std::string codeLineStr = getCodeLineString( codeLine );
    auto broadcast = waitForBroadcast( "hub.room.created", codeLineStr );

    const Json& result = answer[ "result" ];
    std::string id;
    id = json_get_str( result, "_id" );
    return id;
}

void ParentRoomTest::checkRoomInDevice( const std::string& deviceId, const std::string& roomId, int codeLine ) {
    auto& remoteInterface = makeRemote( nma() );
    auto deviceInfo = remoteInterface.getSingleDevice( deviceId );
    std::string roomIdNew;
    auto codeLineStr = std::to_string( codeLine );
    EXPECT_TRUE( json_get( deviceInfo, "roomId", roomIdNew ) ) << codeLineStr;
    EXPECT_EQ( roomId, roomIdNew ) << codeLineStr;
}

void ParentRoomTest::deleteRoomCommand( const std::string& roomId, int codeLine ) {
    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.room.delete" }, { "params", JSON( "_id" << roomId ) } } );
    std::string codeLineStr = getCodeLineString( codeLine );
    auto broadcast = waitForBroadcast( "hub.room.deleted", codeLineStr );
}

JsonObject ParentRoomTest::createDeviceJson() {
    using namespace CoreD::Model::Resources;
    return JSON( DeviceFields::IdFieldName
                 << "5cf0dc8c7f000068d223e889" << DeviceFields::BatteryPoweredFieldName << false
                 << DeviceFields::CategoryFieldName << "switch" << DeviceFields::GatewayIdFieldName
                 << "5ca480227f00004c03de3f1f" << DeviceFields::InfoFieldName << JSON_EMPTY_OBJ
                 << DeviceFields::NameFieldName << "Smart Plug" << DeviceFields::StatusFieldName << "idle"
                 << DeviceFields::ReachableFieldName << true << DeviceFields::ArmedFieldName << false
                 << DeviceFields::ServiceNotificationFieldName << false << DeviceFields::RoomIdFieldName << ""
                 << DeviceFields::SubcategoryFieldName << "interior_plugin" << DeviceFields::TypeFieldName << "device"
                 << DeviceFields::ParentRoomFieldName << true );
}

TEST_F( ParentRoomTest, checkParentRoomPropertyInHubDevicesListForChild ) {  // 1.1
    using namespace CoreD::Model::Resources;

    auto parentDeviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto deviceInfo =
        addChildDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.devices.list" }, { "params", JSON_EMPTY_OBJ } } );
    Json devices;
    std::vector< Json > keys{ "result", "devices" };
    ASSERT_TRUE( json_get( answer, keys, devices ) );
    ASSERT_TRUE( devices.is_array() );
    ASSERT_FALSE( devices.empty() );
    for( const auto& device : devices ) {
        ASSERT_TRUE( device.is_object() );
        if( json_get_str( device, "parentDeviceId" ).empty() ) {
            ASSERT_EQ( device.count( DeviceFields::ParentRoomFieldName ), 0 );
        }
        else {
            ASSERT_EQ( device[ DeviceFields::ParentRoomFieldName ], ParentRoom );
        }
    }
    removeDevice( deviceInfo.m_deviceId, __LINE__ );
    removeDevice( parentDeviceInfo.m_deviceId, __LINE__ );
}

TEST_F( ParentRoomTest, checkParentRoomPropertyInHubDevicesList ) {  // 1.2
    using namespace CoreD::Model::Resources;

    auto deviceInfo = addDeviceWithOneItem( CoreD::Model::ValueType::Enum::Int, __LINE__ );
    auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.devices.list" }, { "params", JSON_EMPTY_OBJ } } );
    Json devices;
    std::vector< Json > keys{ "result", "devices" };
    ASSERT_TRUE( json_get( answer, keys, devices ) );
    ASSERT_TRUE( devices.is_array() );
    ASSERT_FALSE( devices.empty() );

    for( auto device : devices ) {
        std::string id;
        ASSERT_TRUE( json_get( device, "_id", id ) );
        if( id == deviceInfo.m_deviceId ) {
            ASSERT_FALSE( json_get( device, DeviceFields::ParentRoomFieldName, device ) );
        }
    }

    removeDevice( deviceInfo.m_deviceId, __LINE__ );
}

TEST_F( ParentRoomTest, checkDeviceAsJsonWrongParentRoomParameter ) {  // 1.3
    using namespace CoreD::Model::Resources;
    JsonObject device = createDeviceJson();
    device[ DeviceFields::ParentRoomFieldName ] = "room";

    EXPECT_THROW( checkDeviceAsJsonWithDevice( device ), std::exception );
}

TEST_F( ParentRoomTest, checkDeviceAsJsonWrongIfParentRoomForParent ) {  // 1.4
    using namespace CoreD::Model::Resources;
    JsonObject device = createDeviceJson();
    device[ DeviceFields::ParentDeviceIdFieldName ] = "parent";

    EXPECT_THROW( checkDeviceAsJsonWithDevice( device ), std::exception );
}

TEST_F( ParentRoomTest, checkBroadcasts ) {  // 1.5
    using namespace CoreD::Model::Resources;

    JsonObject broadcast, result;
    std::string codeLineStr = getCodeLineString( __LINE__ );

    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );
    CoreD::Model::Device parentDevice = createDevice(
        coreRemoteInterface, "testParentDevice", "smoke", "134_3_24", "security_sensor", "smoke", false, __LINE__ );

    std::string parentDeviceId;
    bool isCreated = false;
    std::tie( parentDeviceId, isCreated ) = coreRemoteInterface.addDevice( parentDevice );
    assertTrue( isCreated, NMA_TEST_MAKE_MESSAGE( "" ) );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_bool( result, DeviceFields::ParentRoomFieldName ), false );

    CoreD::Model::Device childDevice = createDevice(
        coreRemoteInterface, "testChildDevice", "smoke", "134_3_24", "security_sensor", "smoke", false, __LINE__ );
    const bool useParentRoom = true;
    childDevice.setParentRoom( useParentRoom );
    childDevice.setParentDeviceId( parentDeviceId );

    std::string childDeviceId;
    isCreated = false;
    std::tie( childDeviceId, isCreated ) = coreRemoteInterface.addDevice( childDevice );
    assertTrue( isCreated, NMA_TEST_MAKE_MESSAGE( "Child device create" ) );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.added" );

    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.added" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) );
    EXPECT_EQ( json_get_str( result, DeviceFields::ParentDeviceIdFieldName ), parentDeviceId );
    EXPECT_EQ( json_get_bool( result, DeviceFields::ParentRoomFieldName ), useParentRoom );

    auto updatedDevice = createDevice( coreRemoteInterface, "modify", "siren", "siren", "siren", "", false, __LINE__ );
    updatedDevice.setParentDeviceId( parentDeviceId );
    updatedDevice.setParentRoom( useParentRoom );

    coreRemoteInterface.modifyDevice( childDeviceId, updatedDevice );
    checkBroadcast( childDeviceId,
                    "hub.device.updated",
                    { { Resources::CheckBroadcast::HasNot, DeviceFields::ParentRoomFieldName } },
                    __LINE__ );

    updatedDevice.setParentRoom( !useParentRoom );

    coreRemoteInterface.modifyDevice( childDeviceId, updatedDevice );
    checkBroadcast( childDeviceId,
                    "hub.device.updated",
                    { { Resources::CheckBroadcast::Has, DeviceFields::ParentRoomFieldName },
                      { DeviceFields::ParentRoomFieldName, !useParentRoom } },
                    __LINE__ );

    removeDevice( childDeviceId, __LINE__ );
    removeDevice( parentDeviceId, __LINE__ );
}

TEST_F( ParentRoomTest, checkHubDeviceRoomSet ) {  // 2.2
    using namespace CoreD::Model::Resources;
    const auto Room = "RoomID";
    auto roomId = setRoomCreateCommand( Room, __LINE__ );

    auto parentDeviceInfo = addDeviceWithOneItemAndRoom( CoreD::Model::ValueType::Enum::Int, roomId, __LINE__ );
    auto childDeviceInfo1 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, ParentRoom, roomId, __LINE__ );
    auto childDeviceInfo2 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, ParentRoom, roomId, __LINE__ );
    auto childDeviceInfo3 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, SeparateRoom, roomId, __LINE__ );
    auto childDeviceInfo4 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, SeparateRoom, roomId, __LINE__ );
    const auto NewRoom = "NewRoomID";
    auto newRoomId = setRoomCreateCommand( NewRoom, __LINE__ );
    setParentRoomCommand( parentDeviceInfo.m_deviceId, newRoomId, {}, false, __LINE__ );
    checkBroadcast( parentDeviceInfo.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkBroadcast( childDeviceInfo1.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkBroadcast( childDeviceInfo2.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );

    checkBroadcast( childDeviceInfo3.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );

    checkBroadcast( childDeviceInfo4.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );

    checkRoomInDevice( parentDeviceInfo.m_deviceId, newRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo1.m_deviceId, newRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo2.m_deviceId, newRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo3.m_deviceId, newRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo4.m_deviceId, newRoomId, __LINE__ );

    const auto NewParentRoom = "NewParentRoom";
    auto NewParentRoomId = setRoomCreateCommand( NewParentRoom, __LINE__ );

    setParentRoomCommand(
        parentDeviceInfo.m_deviceId, NewParentRoomId, { childDeviceInfo3.m_deviceId }, false, __LINE__ );
    checkBroadcast( parentDeviceInfo.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );

    checkBroadcast( childDeviceInfo1.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkBroadcast( childDeviceInfo2.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkBroadcast( childDeviceInfo4.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );

    checkRoomInDevice( parentDeviceInfo.m_deviceId, NewParentRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo1.m_deviceId, NewParentRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo2.m_deviceId, NewParentRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo3.m_deviceId, newRoomId, __LINE__ );
    checkRoomInDevice( childDeviceInfo4.m_deviceId, NewParentRoomId, __LINE__ );

    setParentRoomCommand( childDeviceInfo1.m_deviceId, roomId, {}, true, __LINE__ );
    checkRoomInDevice( childDeviceInfo1.m_deviceId, NewParentRoomId, __LINE__ );

    setParentRoomCommand( childDeviceInfo3.m_deviceId, NewParentRoomId, {}, false, __LINE__ );
    checkBroadcast( childDeviceInfo3.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkRoomInDevice( childDeviceInfo3.m_deviceId, NewParentRoomId, __LINE__ );

    removeDevice( childDeviceInfo4.m_deviceId, __LINE__ );
    removeDevice( childDeviceInfo3.m_deviceId, __LINE__ );
    removeDevice( childDeviceInfo2.m_deviceId, __LINE__ );
    removeDevice( childDeviceInfo1.m_deviceId, __LINE__ );
    removeDevice( parentDeviceInfo.m_deviceId, __LINE__ );
}

TEST_F( ParentRoomTest, checkHubDeviceRoomSetErrorArgument ) {  // 2.2
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.device.room.set" },
                                                       { "params",
                                                         JSON( "_id"
                                                               << "deviceId"
                                                               << "roomId"
                                                               << "roomId"
                                                               << "separateChildDevices"
                                                               << "arr" ) } } );
    auto error = answer[ "error" ];
    EXPECT_TRUE( !error.empty() );

    error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "separateChildDevices" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( ParentRoomTest, checkRoomDeletion ) {  // 3.1
    const auto Room = "RoomID";
    auto roomId = setRoomCreateCommand( Room, __LINE__ );

    const auto Room1 = "RoomID1";
    auto roomId1 = setRoomCreateCommand( Room1, __LINE__ );

    auto parentDeviceInfo = addDeviceWithOneItemAndRoom( CoreD::Model::ValueType::Enum::Int, roomId, __LINE__ );
    auto childDeviceInfo1 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, ParentRoom, roomId, __LINE__ );
    auto childDeviceInfo2 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo.m_deviceId, SeparateRoom, roomId, __LINE__ );

    deleteRoomCommand( roomId, __LINE__ );

    checkBroadcast( parentDeviceInfo.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkBroadcast( childDeviceInfo1.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkBroadcast( childDeviceInfo2.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );

    checkRoomInDevice( parentDeviceInfo.m_deviceId, "", __LINE__ );
    checkRoomInDevice( childDeviceInfo1.m_deviceId, "", __LINE__ );
    checkRoomInDevice( childDeviceInfo2.m_deviceId, "", __LINE__ );

    removeDevice( childDeviceInfo2.m_deviceId, __LINE__ );
    removeDevice( childDeviceInfo1.m_deviceId, __LINE__ );
    removeDevice( parentDeviceInfo.m_deviceId, __LINE__ );
}

TEST_F( ParentRoomTest, checkRoomDeletionDifferentRooms ) {  // 3.2
    const auto Room2 = "RoomID";
    auto roomId2 = setRoomCreateCommand( Room2, __LINE__ );

    const auto Room3 = "RoomID3";
    auto roomId3 = setRoomCreateCommand( Room3, __LINE__ );
    auto parentDeviceInfo1 = addDeviceWithOneItemAndRoom( CoreD::Model::ValueType::Enum::Int, roomId2, __LINE__ );
    auto childDeviceInfo3 = addChildDeviceWithOneItemAndParentRoom(
        CoreD::Model::ValueType::Enum::Int, parentDeviceInfo1.m_deviceId, SeparateRoom, roomId2, __LINE__ );

    setParentRoomCommand( childDeviceInfo3.m_deviceId, roomId3, {}, false, __LINE__ );
    checkBroadcast( childDeviceInfo3.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, RoomIdKey } },
                    __LINE__ );
    checkRoomInDevice( childDeviceInfo3.m_deviceId, roomId3, __LINE__ );

    deleteRoomCommand( roomId2, __LINE__ );
    checkBroadcast( parentDeviceInfo1.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, "roomId" } },
                    __LINE__ );
    checkRoomInDevice( childDeviceInfo3.m_deviceId, roomId3, __LINE__ );

    // check delete child device room
    deleteRoomCommand( roomId3, __LINE__ );
    checkBroadcast( childDeviceInfo3.m_deviceId,
                    HubDeviceUpdatedBroadcast,
                    { { Resources::CheckBroadcast::Has, "roomId" } },
                    __LINE__ );
    checkRoomInDevice( childDeviceInfo3.m_deviceId, "", __LINE__ );

    removeDevice( childDeviceInfo3.m_deviceId, __LINE__ );
    removeDevice( parentDeviceInfo1.m_deviceId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
