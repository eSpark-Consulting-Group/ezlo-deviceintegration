#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/resources/house_mode_resources.h"
#include "nma_api_tests/tests/fixtures/house_mode_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "common_test_resources/resources/scene_manager_resources.h"
#include "cored/house_modes/house_mode_utils.h"
#include "common_test_resources/resources/cored_resources.h"
#include "cored/protect/protect_utils.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1. Set disarmed default
DONE        1.2 Set disarmed default for another mode, use device.houseModesOptions to tune mode."disarmed"
DONE    2. check default notifications
DONE    3. check disarmed default logic

DONE    5. Broadcasts
DONE        5.1 hub.modes.alarms_off
DONE        5.2 hub.modes.disarmed_devices
DONE        5.3 hub.modes.alarmed
DONE        5.4 hub.modes.alarmed using Device group delay

DONE    6. Check mode.armed field
DONE    7. Entry Delay
DONE        7.1 Check entry_delay.set api pass
DONE        7.2 Check entry_delay.set api fail
DONE        7.3 Check entry_delay.reset api

DONE    8. House Modes Version

DONE    9. Exit Time Runs Out
DONE    10. Abort Window
DONE    11. Local Alarm Turn Off
            11.1 Success
            11.2 Failed
DONE    12. House modes force disarm
DONE        12.1 Success
DONE        12.2 Failed
DONE    13. Protect Alarm Item
DONE    14. Silent Alarm Flag
DONE    15. Alarm Delay Value Doesn't overrides

*/

class TestsHouseMode : public TestsHouseModeFixture {
public:
    void SetUp() override;

    Json executeScript( const std::string& script, const Json& scriptParams, int codeLine );

    void ignoreHouseModesAlarmedBroadcast();
};

void TestsHouseMode::SetUp() {
    TestsHouseModeFixture::SetUp();
    callingFile( __FILE__ );
}

Json TestsHouseMode::executeScript( const std::string& script, const Json& scriptParams, int codeLine ) {
    std::string codeLineStr = getCodeLineString( codeLine );

    auto request = replaceTokens( hubExtensionsPluginRun, { { "__SCRIPT__", script } } );
    request[ "params" ][ "scriptParams" ] = scriptParams;

    Json result;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    auto error = answer[ "error" ];
    EXPECT_TRUE( error.empty() ) << error.dump( 4 ) << codeLineStr;
    if( error.empty() ) {
        auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", codeLineStr );
        EXPECT_FALSE( broadcast.empty() ) << codeLineStr;

        Json broadcastResult;
        EXPECT_TRUE( json_get_obj( broadcast, "result", broadcastResult ) ) << codeLineStr;
        if( !broadcastResult.empty() )
            EXPECT_TRUE( json_get( broadcastResult, "value", result ) );

        checkExtensionsPluginRunProgressBroadcast( "started", __LINE__ );
        checkExtensionsPluginRunProgressBroadcast( "finished", __LINE__ );
    }

    return result;
}

void TestsHouseMode::ignoreHouseModesAlarmedBroadcast() {
    ignoreBroadcast(
        []( const JsonObject& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.modes.alarmed"; },
        std::chrono::seconds( 1 ) );
}

using namespace CoreDHouseModes::Resources;

TEST_F( TestsHouseMode, setDisarmedDefault /*1*/ ) {
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    auto gatewayId = getMainGateway( coreRemoteInterface, __LINE__ );
    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeId != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        checkHouseModeSwitched( NMA_TEST_MAKE_MESSAGE( "" ) );
        checkHouseModeSwitched( NMA_TEST_MAKE_MESSAGE( "" ) );
    }
    houseModeId = "1";

    // add armed device1 and disarmed device2 in HouseMode 1;
    CoreD::Model::Device device01(
        gatewayId, "testDevice1", "smoke", "134_3_24", "security_sensor", false /*batteryPowered*/ );
    device01.setSubcategory( "smoke" );
    bool isGood;
    std::string device1Id, device2Id;
    std::tie( device1Id, isGood ) = coreRemoteInterface.addDevice( device01 );
    ASSERT_TRUE( isGood ) << "device wasn't created";

    checkBroadcast( device1Id,
                    "hub.device.added",
                    { { "$has-not", DeviceFields::ArmedFieldName } },
                    NMA_TEST_MAKE_MESSAGE( "device1 created" ) );

    auto securityItemId1 = addItem(
        coreRemoteInterface, device1Id, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( device1Id, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    CoreD::Model::Device device02(
        gatewayId, "testDevice2", "motion", "134_3_24", "security_sensor", false /*batteryPowered*/ );
    device02.setSubcategory( "motion" );
    std::tie( device2Id, isGood ) = coreRemoteInterface.addDevice( device02 );
    ASSERT_TRUE( isGood );

    checkBroadcast( device2Id,
                    "hub.device.added",
                    { { "$has-not", DeviceFields::ArmedFieldName } },
                    NMA_TEST_MAKE_MESSAGE( "device2 created" ) );

    auto securityItemId2 = addItem(
        coreRemoteInterface, device2Id, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( device2Id, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    // check DisarmedDefault  in HouseMode 1;
    {
        auto device1 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_TRUE( json_get_bool( device1, DeviceFields::ArmedFieldName ) );
        auto device2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_FALSE( json_get_bool( device2, DeviceFields::ArmedFieldName ) );

        auto houseMode = coreRemoteInterface.getHouseModeById( "1" );
        EXPECT_TRUE( houseMode->m_disarmedDefault ) << houseMode->toJson();
        std::set< std::string > expected{ device2Id };
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expected );
    }
    // add DisarmedDevice (change setDisarmedDefault to false) and check DisarmedDefault in HouseMode 1;
    {
        const auto answer =
            nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.disarmed_devices.add" },
                                           { "params", JSON( "modeId" << houseModeId << "deviceId" << device1Id ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        EXPECT_FALSE( json_get_bool( broadcast[ "result" ], "disarmedDefault" ) );
        int64_t timestamp;
        EXPECT_TRUE( json_get( broadcast[ "result" ], "timestamp", timestamp ) );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.disarmed_devices.added" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.disarmed_devices.added" );

        coreRemoteInterface.setDeviceArmed( device2Id, true );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

        auto device1 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_FALSE( json_get_bool( device1, DeviceFields::ArmedFieldName ) );
        auto device2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get_bool( device2, DeviceFields::ArmedFieldName ) );

        auto houseMode = coreRemoteInterface.getHouseModeById( "1" );
        EXPECT_FALSE( houseMode->m_disarmedDefault );
        std::set< std::string > expected{ device2Id, device1Id };
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expected );
    }
    // change setDisarmedDefault to true and check DisarmedDefault in HouseMode 1;
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.disarmed_default.set" },
              { "params", JSON( "modeId" << houseModeId << "disarmedDefault" << true ) } } );
        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        EXPECT_TRUE( json_get_bool( broadcast[ "result" ], "disarmedDefault" ) );

        auto device1 = coreRemoteInterface.getSingleDevice( device1Id );
        EXPECT_TRUE( json_get_bool( device1, DeviceFields::ArmedFieldName ) );
        auto device2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_FALSE( json_get_bool( device2, DeviceFields::ArmedFieldName ) );

        auto houseMode1 = coreRemoteInterface.getHouseModeById( "1" );
        EXPECT_TRUE( houseMode1->m_disarmedDefault );
        std::set< std::string > expected{ device2Id };
        EXPECT_EQ( houseMode1->m_disarmedDeviceIds, expected );
    }
    // remove devices
    size_t itemsNumber = coreRemoteInterface.getItemsByDeviceId( device1Id ).size();
    removeDevice( coreRemoteInterface, device1Id, itemsNumber, 0, __LINE__ );

    itemsNumber = coreRemoteInterface.getItemsByDeviceId( device2Id ).size();
    removeDevice( coreRemoteInterface, device2Id, itemsNumber, 0, __LINE__ );
}

TEST_F( TestsHouseMode, checkDefaultNotifications /*2*/ ) {
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ } } );
    JsonObject result;
    JsonArray houseModes;
    ASSERT_TRUE( json_get_obj( answer, "result", result ) );
    ASSERT_TRUE( json_get_arr( result, "modes", houseModes ) );
    for( const auto& mode : houseModes ) {
        const JsonArray& notifications = mode.at( "notifications" );
        EXPECT_TRUE( notifications.is_null() );
    }
}

TEST_F( TestsHouseMode, checkDisarmedDefaultLogic /*3*/ ) {
    auto checkHouseModes =
        []( std::function< void( const JsonArray& houseModes, const std::string& codeLineStr ) > callback,
            int codeLine ) {
            std::string codeLineStr = "code line: " + std::to_string( codeLine );
            const auto answer =
                nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ } } );
            JsonObject result;
            JsonArray houseModes;
            ASSERT_TRUE( json_get_obj( answer, "result", result ) ) << codeLineStr;
            ASSERT_TRUE( json_get_arr( result, "modes", houseModes ) ) << codeLineStr;
            callback( houseModes, codeLineStr );
        };
    checkHouseModes(
        []( const JsonArray& houseModes, const std::string& codeLineStr ) {
            for( const auto& mode : houseModes ) {
                EXPECT_TRUE( json_get_bool( mode, "disarmedDefault" ) ) << codeLineStr;
            }
        },
        __LINE__ );
    for( std::size_t i = 1; i <= 4; ++i ) {
        std::string id = std::to_string( i );
        std::string userId = "userId" + id;
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.notifications.set" },
              { "params", JSON( "modeId" << id << "all" << false << "userIds" << JSON_ARRAY( userId ) ) } } );
        EXPECT_TRUE( answer[ "error" ].empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        EXPECT_FALSE( json_get_bool( broadcast[ "result" ], "disarmedDefault" ) );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.notifications.notify_all" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.notifications.notify_all" );
        EXPECT_FALSE( json_get_bool( broadcast[ "result" ], "all" ) );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.notifications.added" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.notifications.added" );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "notification" ), userId );
    }
    checkHouseModes(
        []( const JsonArray& houseModes, const std::string& codeLineStr ) {
            std::size_t i = 1;
            for( const auto& mode : houseModes ) {
                std::string userId = "userId" + std::to_string( i );
                const JsonArray& notifications = mode.at( "notifications" );
                for( auto it = notifications.begin(); it != notifications.end(); ++it ) {
                    EXPECT_EQ( json_get_str( *it ), userId ) << codeLineStr;
                }
                EXPECT_FALSE( json_get_bool( mode, "disarmedDefault" ) ) << codeLineStr;
                ++i;
            }
        },
        __LINE__ );

    for( std::size_t i = 1; i <= 4; ++i ) {
        std::string id = std::to_string( i );
        const auto answer =
            nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.disarmed_default.set" },
                                           { "params", JSON( "modeId" << id << "disarmedDefault" << true ) } } );
        EXPECT_TRUE( answer[ "error" ].empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        EXPECT_TRUE( json_get_bool( broadcast[ "result" ], "disarmedDefault" ) );
    }
    checkHouseModes(
        []( const JsonArray& houseModes, const std::string& codeLineStr ) {
            for( const auto& mode : houseModes ) {
                const JsonArray& notifications = mode.at( "notifications" );
                EXPECT_TRUE( notifications.is_null() ) << codeLineStr;
                EXPECT_TRUE( json_get_bool( mode, "disarmedDefault" ) ) << codeLineStr;
            }
        },
        __LINE__ );
}

TEST_F( TestsHouseMode, checkBroadcastsAlarmsOff /*5.1*/ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    // deviceId will be added by default to every houseMode's alarmsOffDevices (4 times). So, first, remove, than add.
    const auto deviceId =
        addDevice( coreRemoteInterface, "Controller Siren", "siren", "134_3_24", "siren", "", false, __LINE__ );

    // hub.modes.alarms_off.added
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.alarms_off.add" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "1"
                                                                   << "deviceId" << deviceId ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarms_off.added" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarms_off.added" );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), houseModeId );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], BroadcastFields::AlarmsOffDeviceFieldName ), deviceId );

        broadcast =
            waitForBroadcast( "hub.modes.changed", NMA_TEST_MAKE_MESSAGE( "disarmedDefault change broadcast" ) );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        EXPECT_FALSE( json_get_bool( broadcast[ "result" ], "disarmedDefault" ) );
    }

    // hub.modes.alarms_off.removed
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.alarms_off.remove" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "1"
                                                                   << "deviceId" << deviceId ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarms_off.removed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarms_off.removed" );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), houseModeId );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], BroadcastFields::AlarmsOffDeviceFieldName ), deviceId );
    }
}

TEST_F( TestsHouseMode, checkBroadcastsDisarmedDevices /*5.2*/ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );

    const std::string houseModeId( "1" );
    const auto deviceId = addDevice(
        coreRemoteInterface, "Smoke sensor", "smoke", "134_3_24", "security_sensor", "smoke", false, __LINE__ );

    using namespace CoreD::Model::Resources;
    auto securityItemId1 = addItem(
        coreRemoteInterface, deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    // hub.modes.disarmed_devices.added
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.disarmed_devices.add" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "1"
                                                                   << "deviceId" << deviceId ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.changed" );
        EXPECT_FALSE( json_get_bool( broadcast[ "result" ], "disarmedDefault" ) );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.disarmed_devices.added" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.disarmed_devices.added" );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), houseModeId );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], BroadcastFields::DisarmedDeviceFieldName ), deviceId );
    }

    // hub.modes.disarmed_devices.removed
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.disarmed_devices.remove" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "1"
                                                                   << "deviceId" << deviceId ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.disarmed_devices.removed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.disarmed_devices.removed" );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], "modeId" ), houseModeId );
        EXPECT_EQ( json_get_str( broadcast[ "result" ], BroadcastFields::DisarmedDeviceFieldName ), deviceId );
    }
}

TEST_F( TestsHouseMode, checkBroadcastsHouseModeAlarmed /*5.3*/ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coredRemote( nma() );
    coredRemote.setAlarmDelay( 0 );
    const std::string houseModeId( "1" );
    const auto deviceId =
        addDevice( coredRemote, "Smoke sensor", "smoke", "134_3_24", "security_sensor", "smoke", false, __LINE__ );

    const auto valueType = CoreD::Model::ValueType::Enum::Bool;
    const auto itemId = addItem( coredRemote, deviceId, "security_threat", valueType, true, true, __LINE__ );

    using namespace CoreD::Model::Resources;
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    coredRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << true ) );
    const Json broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    testHouseModeAlarmedBroadcast( houseModeId, deviceId, __LINE__ );
}

TEST_F( TestsHouseMode, houseModeAlarmedByDeviceGroup /*5.4*/ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coredRemote( nma() );
    coredRemote.resetEntryDelay();
    Json broadcast =
        waitForBroadcast( "hub.modes.entry_delay.changed", NMA_TEST_MAKE_MESSAGE( "entry delay changed" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.entry_delay.changed" );
    const std::string deviceCategory = "security_sensor";
    const std::string deviceSubcategory = "smoke";
    auto expectedDelayValue = -1;
    auto createGroupRequest = Json::parse( scenesDeviceGroupCreate );
    createGroupRequest[ "params" ][ "name" ] = "testingGroup";
    createGroupRequest[ "params" ][ "categories" ] =
        JSON_ARRAY( JSON( "category" << deviceCategory << "subcategories" << JSON_ARRAY( deviceSubcategory ) ) );

    createGroupRequest[ "params" ][ "entryDelay" ] = "normal";
    auto deviceGroupId = createDeviceGroup( createGroupRequest, __LINE__ );
    const std::string houseModeId( "1" );
    const auto deviceId = addDevice(
        coredRemote, "Smoke sensor", "smoke", "134_3_24", deviceCategory, deviceSubcategory, false, __LINE__ );

    const auto valueType = CoreD::Model::ValueType::Enum::Bool;
    const auto itemId = addItem( coredRemote, deviceId, "security_threat", valueType, true, true, __LINE__ );

    using namespace CoreD::Model::Resources;
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    coredRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << true ) );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    const Json& result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), houseModeId );
    EXPECT_EQ( json_get_str( result, "deviceId" ), deviceId );
    EXPECT_EQ( json_get_str( result, "phase" ), "main" );
    EXPECT_EQ( json_get_int( result, "pendingDelay" ), expectedDelayValue );
    EXPECT_EQ( json_get_str( result, "status" ), "begin" );

    deleteDeviceGroup( deviceGroupId, __LINE__ );
}

TEST_F( TestsHouseMode, setDisarmedDefaultForDeviceHouseModeOptions /*1.2*/ ) {
    LOGGER_TRACE( "setDisarmedDefaultForDeviceHouseModeOptions STARTED..." );
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    const std::string houseModeHome = "1";
    const std::string houseModeNight = "3";

    auto gatewayId = getMainGateway( coreRemoteInterface, __LINE__ );
    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeId != houseModeHome ) {
        coreRemoteInterface.setHouseMode( houseModeHome );
        checkHouseModeSwitched( NMA_TEST_MAKE_MESSAGE( "" ) );
    }

    houseModeId = houseModeHome;

    // add a device here with houseModesOptions dictionary set.
    namespace Fields = HubPlatform::CoreD::Model::Resources::DeviceFields;
    CoreD::Model::Device device02(
        gatewayId, "testDeviceWithHouseModesOptions2", "motion", "sensor.motion", "security_sensor", false );

    Json houseModeOptions = { { Fields::HouseModesOptions::DisarmedFieldName,
                                Json::array( { houseModeHome, houseModeNight } ) } };
    device02.setHouseModesOptions( houseModeOptions );

    device02.setSubcategory( "motion" );

    bool isGood;
    std::string device2Id;
    std::tie( device2Id, isGood ) = coreRemoteInterface.addDevice( device02 );
    ASSERT_TRUE( isGood );

    checkBroadcast( device2Id,
                    "hub.device.added",
                    { { "$has-not", "armed" }, { "subcategory", "motion" } },
                    NMA_TEST_MAKE_MESSAGE( "motion sensor device added" ) );

    using namespace CoreD::Model::Resources;
    auto securityItemId2 = addItem(
        coreRemoteInterface, device2Id, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );
    checkDeviceUpdateBroadcast( device2Id, { { DeviceFields::ArmedFieldName, false } }, __LINE__ );

    {
        auto houseMode = coreRemoteInterface.getHouseModeById( houseModeHome );
        LOGGER_TRACE( houseMode->toJson().dump() );
        EXPECT_TRUE( houseMode->m_disarmedDefault );
        std::set< std::string > expected{ device2Id };
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expected );

        houseMode = coreRemoteInterface.getHouseModeById( houseModeNight );
        LOGGER_TRACE( houseMode->toJson().dump() );
        std::set< std::string > expectedAtNight{ device2Id };
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expectedAtNight );

        // armed
        houseMode = coreRemoteInterface.getHouseModeById( "2" );
        LOGGER_TRACE( houseMode->toJson().dump() );
        EXPECT_TRUE( houseMode->m_disarmedDeviceIds.empty() ) << "Should be armed when Away is selected";

        houseMode = coreRemoteInterface.getHouseModeById( "4" );
        LOGGER_TRACE( houseMode->toJson().dump() );
        EXPECT_TRUE( houseMode->m_disarmedDeviceIds.empty() ) << "Should be armed when Vacation is selected";
    }

    // remove Device from disarmed devices of Night house mode: it will set "disarmedDefault" to false and removes from
    // "disarmed" in HouseMode 2;
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.disarmed_devices.remove" },
              { "params", JSON( "modeId" << houseModeNight << "deviceId" << device2Id ) } } );

        EXPECT_TRUE( answer[ "result" ].empty() );
        EXPECT_TRUE( answer[ "error" ].empty() );
        checkModesChangedDisarmedDefault( false, NMA_TEST_MAKE_MESSAGE( "disarmedDefault should change to false" ) );
        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.disarmed_devices.removed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.disarmed_devices.removed" );

        coreRemoteInterface.setDeviceArmed( device2Id, true );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.device.updated" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.device.updated" );

        auto device2 = coreRemoteInterface.getSingleDevice( device2Id );
        EXPECT_TRUE( json_get_bool( device2, "armed" ) );

        auto houseMode = coreRemoteInterface.getHouseModeById( houseModeNight );
        EXPECT_FALSE( houseMode->m_disarmedDefault );
        std::set< std::string > expected{};
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expected );
    }

    // change setDisarmedDefault to true and check DisarmedDefault in HouseMode 3;
    {
        const bool disarmedDefault = true;
        setDisarmedDefault(
            houseModeNight, disarmedDefault, NMA_TEST_MAKE_MESSAGE( "disarmedDefault should change to true" ) );

        auto houseMode = coreRemoteInterface.getHouseModeById( houseModeNight );
        EXPECT_NE( houseMode, nullptr );
        EXPECT_TRUE( houseMode->m_disarmedDefault );
        std::set< std::string > expected{ device2Id };
        EXPECT_EQ( houseMode->m_disarmedDeviceIds, expected );
    }
    // remove devices
    unsigned long itemsNumber = coreRemoteInterface.getItemsByDeviceId( device2Id ).size();
    removeDevice( coreRemoteInterface, device2Id, itemsNumber, 0, __LINE__ );
}

TEST_F( TestsHouseMode, checkArmedField /* 6 */ ) {
    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ } } );
    JsonObject result;
    JsonArray houseModes;
    ASSERT_TRUE( json_get_obj( answer, "result", result ) );
    ASSERT_TRUE( json_get_arr( result, "modes", houseModes ) );
    for( const auto& mode : houseModes ) {
        Json armed;
        ASSERT_TRUE( json_get( mode, "armed", armed ) );
        ASSERT_TRUE( armed.is_boolean() );
    }
}

TEST_F( TestsHouseMode, checkEntryDelaySet /* 7.1 */ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto entryDelayObject = R"({
        "normal": 30,
         "longExtended": 120,
         "extended": 60,
         "instant": 0
    })"_json;

    const auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.set" }, { "params", entryDelayObject } } );
    JsonObject result, broadcast;
    auto error = json_get_str( answer, "error" );
    ASSERT_TRUE( error.empty() );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.entry_delay.changed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.entry_delay.changed" );
    json_get_obj( broadcast, "result", result );
    EXPECT_TRUE( json_contains( result, "entryDelay" ) );
    EXPECT_EQ( result[ "entryDelay" ], entryDelayObject );
    auto houseModeEntryDelay = coreRemoteInterface.getEntryDelay();
    EXPECT_EQ( houseModeEntryDelay, entryDelayObject );
}

TEST_F( TestsHouseMode, checkEntryDelaySetFail /* 7.2 */ ) {
    auto entryDelayObject = R"({
    "normal": 30,
     "longExtended": 120,
     "extended": 400,
     "instant": 0
})"_json;

    auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.set" }, { "params", entryDelayObject } } );

    JsonObject error;
    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.range.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is not in valid range" );

    entryDelayObject = R"({
                            "normal": 30,
                            "longExtended": 120,
                            "extended": 60
                            })"_json;

    answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.set" }, { "params", entryDelayObject } } );

    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );

    entryDelayObject = R"({
                        "normal": 30,
                        "longExtended": 120,
                        "extend": 40,
                        "instant": 0
                        })"_json;

    answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.set" }, { "params", entryDelayObject } } );

    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "nma.user.notfound.key" );
    EXPECT_EQ( json_get_str( error, "message" ), "Key not found" );

    entryDelayObject = R"({
                        "normal": 30,
                        "longExtended": 120,
                        "extended": 60,
                        "instant": 20
                        })"_json;

    answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.entry_delay.set" }, { "params", entryDelayObject } } );

    json_get_obj( answer, "error", error );
    ASSERT_FALSE( error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.read_only" );
    EXPECT_EQ( json_get_str( error, "message" ), "Constant params" );
}

TEST_F( TestsHouseMode, checkEntryDelayReset /* 7.3 */ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    auto entryDelayObject = R"({
    "normal": 60,
     "longExtended": 70,
     "extended": 40,
     "instant": 0
})"_json;

    auto defaultEntryDelayObject = R"({
    "normal": 30,
     "longExtended": 120,
     "extended": 60,
     "instant": 0
})"_json;
    JsonObject result, broadcast;

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.entry_delay.set" }, { "params", entryDelayObject } } );

        auto error = json_get_str( answer, "error" );
        ASSERT_TRUE( error.empty() );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.entry_delay.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.entry_delay.changed" );
        json_get_obj( broadcast, "result", result );
        EXPECT_EQ( result[ "entryDelay" ], entryDelayObject );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.entry_delay.reset" }, { "params", JSON_EMPTY_OBJ } } );

        auto error = json_get_str( answer, "error" );
        ASSERT_TRUE( error.empty() );

        broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.entry_delay.changed" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.entry_delay.changed" );
        json_get_obj( broadcast, "result", result );
        EXPECT_TRUE( json_contains( result, "entryDelay" ) );
        EXPECT_EQ( result[ "entryDelay" ], defaultEntryDelayObject );
        auto houseModeEntryDelay = coreRemoteInterface.getEntryDelay();
        EXPECT_EQ( houseModeEntryDelay, defaultEntryDelayObject );
    }
}

TEST_F( TestsHouseMode, checkHouseModesVersion /* 8 */ ) {
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "2.0" } } );
        JsonObject result;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_TRUE( json_contains( result, "alarmDelay" ) );
        ASSERT_FALSE( json_contains( result, "entryDelay" ) );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "3.0" } } );
        JsonObject result;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_FALSE( json_contains( result, "alarmDelay" ) );
        ASSERT_TRUE( json_contains( result, "entryDelay" ) );
    }
}

TEST_F( TestsHouseMode, checkExitTimeField /* 9 */ ) {
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "2.0" } } );
        JsonObject result;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_FALSE( json_contains( result, "switchToDelayRunsOut" ) );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "3.0" } } );
        JsonObject result;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_TRUE( json_contains( result, "switchToDelayRunsOut" ) );
        EXPECT_EQ( json_get_int( result, "switchToDelayRunsOut" ), 10 );
    }
}

TEST_F( TestsHouseMode, checkAbortWindowField /* 10 */ ) {
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "2.0" } } );
        JsonObject result;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_FALSE( json_contains( result, "abortWindow" ) );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "3.0" } } );
        JsonObject result, abortWindow;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_TRUE( json_contains( result, "abortWindow" ) );
        ASSERT_TRUE( json_get_obj( result, "abortWindow", abortWindow ) );
        EXPECT_EQ( json_get_int( abortWindow, "default" ), 30 );
        EXPECT_EQ( json_get_int( abortWindow, "minimum" ), 0 );
        EXPECT_EQ( json_get_int( abortWindow, "maximum" ), 45 );
    }
}

TEST_F( TestsHouseMode, turnOffLocalAlarmSuccess /* 11.1 */ ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    coreRemoteInterface.setSwitchDelay( 0 );

    auto houseModeId = coreRemoteInterface.getCurrentHouseModeId();
    if( houseModeId != "1" ) {
        coreRemoteInterface.setHouseMode( "1" );
        testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
        testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    }
    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );

    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.local.alarm.off" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "1" ) } } );

        JsonObject error;
        json_get_obj( answer, "error", error );
        ASSERT_TRUE( error.empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.local.alarmed.turned.off" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.local.alarmed.turned.off" );
    }
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.local.alarm.off" }, { "params", JSON( "armed" << false ) } } );

        JsonObject error;
        json_get_obj( answer, "error", error );
        ASSERT_TRUE( error.empty() );

        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.local.alarmed.turned.off" );
        EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.local.alarmed.turned.off" );
    }
}

TEST_F( TestsHouseMode, turnOffLocalAlarmFailed /* 11.2 */ ) {
    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.local.alarm.off" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "2" ) } } );

        JsonObject error;
        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.house_mode.id.not.correspond" );
    }
    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.local.alarm.off" }, { "params", JSON( "armed" << true ) } } );

        JsonObject error;
        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.house_mode.id.not.correspond" );
    }
}

TEST_F( TestsHouseMode, houseModesForceDisarmSuccess /* 12.1 */ ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );
    if( !CoreDHouseModes::getHouseModesAlarmDevicesEvents( nma().getDBClient() ).empty() ) {
        CoreDHouseModes::clearDeviceFromHouseModesAlarmedList( nma().getDBClient() );
    }

    coreRemoteInterface.setAlarmPhase( "entryDelay", "main" );
    CoreDHouseModes::addDeviceToHouseModesAlarmedList( nma().getDBClient(), "testId1", 124587545, 0, false );
    CoreDHouseModes::addDeviceToHouseModesAlarmedList( nma().getDBClient(), "testId2", 124587545, 0, false );

    auto devicesJsonArray = CoreDHouseModes::getHouseModesAlarmDevicesEvents( nma().getDBClient() );
    EXPECT_TRUE( !devicesJsonArray.empty() );
    EXPECT_EQ( json_get_str( devicesJsonArray[ 0 ][ "_id" ] ), "testId1" );
    EXPECT_EQ( json_get_str( devicesJsonArray[ 1 ][ "_id" ] ), "testId2" );

    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.force_disarm" },
                                                       { "params",
                                                         JSON( "modeId"
                                                               << "1" ) } } );

    JsonObject error;
    json_get_obj( answer, "error", error );
    ASSERT_TRUE( error.empty() );
    using namespace CoreD::Model::Resources;
    testHouseModeAlarmedPhaseStatusMainBroadcast( "1", "", HouseModeAlarmStatus::Done, __LINE__ );
    devicesJsonArray = CoreDHouseModes::getHouseModesAlarmDevicesEvents( nma().getDBClient() );
    EXPECT_TRUE( devicesJsonArray.empty() );

    auto expectedAlarmPhase = std::make_tuple( "idle", "" );
    auto alarmPhase = coreRemoteInterface.getAlarmPhase();
    EXPECT_EQ( alarmPhase, expectedAlarmPhase );
}

TEST_F( TestsHouseMode, houseModesForceDisarmFailed /* 12.2 */ ) {
    CoreDRemoteInterface::CoreDInterface& coreRemoteInterface = makeRemote( nma() );

    coreRemoteInterface.setSwitchDelay( 0 );
    coreRemoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );

    {
        const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.force_disarm" },
                                                           { "params",
                                                             JSON( "modeId"
                                                                   << "1" ) } } );

        JsonObject error;
        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );
        EXPECT_EQ( json_get_str( error, "data" ), "ezlo.house_mode.id.not.correspond" );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.force_disarm" }, { "params", JSON( "modeId" << 1 ) } } );

        JsonObject error;
        json_get_obj( answer, "error", error );
        ASSERT_FALSE( error.empty() );
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
        EXPECT_EQ( json_get_str( error, "reason" ), "mode id must be a string" );
    }
    coreRemoteInterface.setHouseMode( "1" );
    testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
}

TEST_F( TestsHouseMode, houseModeAlarmedWithProtectAlarmItem ) {  // 13

    auto& dbClient = nma().getDBClient();
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setHouseMode( "2" );
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
    auto device = addDeviceWithProtectAlarmItem( __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, "alarm_active", __LINE__ );
    const auto CurrentModeId = "2";

    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, "alarm_active_silent", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__, true );

    remoteInterface.setHouseMode( "1" );
    testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
}

TEST_F( TestsHouseMode, checkSilentAlarmField /* 14 */ ) {
    updatePredefineGroupsForTest( "normal" );
    auto& dbClient = nma().getDBClient();
    const auto AlarmDelay = 1;
    CoreDHouseModes::setAlarmDelay( dbClient, AlarmDelay );
    CoreDRemoteInterface::CoreDRemoteInterface remoteInterface( nma() );
    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );

    // Wait for broadcast first
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    LOGGER_WARN( "DEV: hub.modes.alarmed expected broadcast: " + cc::j( broadcast ) );

    // Only then ask hub.modes.get for the "alarmed" field.

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "2.0" } } );
        JsonObject result, alarmed;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_TRUE( json_get_obj( result, "alarmed", alarmed ) );
        ASSERT_FALSE( json_contains( alarmed, "silent" ) );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "3.0" } } );
        JsonObject result, alarmed;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_TRUE( json_get_obj( result, "alarmed", alarmed ) );
        ASSERT_TRUE( json_contains( alarmed, "silent" ) );
    }
}

TEST_F( TestsHouseMode, houseModeAlarmDelayValueCheck ) {  // 15

    auto& dbClient = nma().getDBClient();
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );

    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setHouseMode( "2" );  // setting armed mode
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );

    const int alarmDelay = 45;
    remoteInterface.setAlarmDelay( alarmDelay );
    auto device = addDeviceWithProtectAlarmItem( __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, "alarm_active", __LINE__ );
    const auto CurrentModeId = "2";

    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );

    setItemValue( remoteInterface, device.m_itemId, "alarm_active_silent", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__, true );

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "2.0" } } );
        JsonObject result;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        EXPECT_EQ( json_get_int( result, "alarmDelay" ), alarmDelay );
    }

    {
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.get" }, { "params", JSON_EMPTY_OBJ }, { "api", "3.0" } } );
        JsonObject result, entryDelay;
        ASSERT_TRUE( json_get_obj( answer, "result", result ) );
        ASSERT_TRUE( json_get_obj( result, "entryDelay", entryDelay ) );
        EXPECT_EQ( json_get_int( entryDelay, "normal" ), alarmDelay );
    }

    remoteInterface.setHouseMode( "1" );  // reset house mode to disarmed
    testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( "1", "", "entryDelay", "canceled", __LINE__, true );
    ignoreHouseModesAlarmedBroadcast();
}

TEST_F( TestsHouseMode, noEntryDelayCheckForSosButtons ) {  // 16

    auto& dbClient = nma().getDBClient();
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    remoteInterface.setSwitchDelay( 0 );
    remoteInterface.setHouseMode( "2" );  // setting armed mode
    testHouseModesSwitchedBroadcast( "2", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "2", "done", __LINE__ );
    auto device = addDeviceWithProtectAlarmItem( __LINE__ );
    const JsonObject deviceObject =
        dbClient.findOne( CoreD::Model::Resources::DBNames::DevicesDB,
                          JSON( CoreD::Model::Resources::DeviceFields::IdFieldName << device.m_deviceId ),
                          nullptr );
    auto securityConfigId = json_get_str( deviceObject, CoreD::Model::Resources::DeviceFields::ProtectConfig );
    EXPECT_EQ( securityConfigId, "default" );  // verifying security config default
    auto securityConfigObject = CoreD::Protect::ProtectUtils::getSecurityConfigObject( dbClient, securityConfigId );
    EXPECT_EQ( json_get_str( securityConfigObject, "entryDelay" ),
               "normal" );  // verifying default security config has normal entry delay

    setItemValue( remoteInterface, device.m_itemId, "alarm_active", __LINE__ );
    const auto CurrentModeId = "2";

    testHouseModeAlarmedPhaseStatusBroadcast(
        CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );  // no entry delay verify

    setItemValue( remoteInterface, device.m_itemId, "alarm_active_silent", __LINE__ );
    testHouseModeAlarmedPhaseStatusBroadcast( CurrentModeId, device.m_deviceId, "main", "begin", __LINE__, true );
}

TEST_F( TestsHouseMode, NoEntryDelayCheckForSmokeSensor ) {  // 17

    auto& dbClient = nma().getDBClient();
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );

    auto device = addDeviceWithSmokeSensorItem( __LINE__ );
    const JsonObject deviceObject =
        dbClient.findOne( CoreD::Model::Resources::DBNames::DevicesDB,
                          JSON( CoreD::Model::Resources::DeviceFields::IdFieldName << device.m_deviceId ),
                          nullptr );
    auto securityConfigId = json_get_str( deviceObject, CoreD::Model::Resources::DeviceFields::ProtectConfig );
    EXPECT_EQ( securityConfigId, "default" );  // verifying security config default
    auto securityConfigObject = CoreD::Protect::ProtectUtils::getSecurityConfigObject( dbClient, securityConfigId );
    EXPECT_EQ( json_get_str( securityConfigObject, "entryDelay" ),
               "normal" );  // verifying default security config has normal entry delay

    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "2";

    testHouseModeAlarmedPhaseStatusBroadcast(
        CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );  // no entry delay verify
}

TEST_F( TestsHouseMode, noEntryDelayCheckForSecurityItem ) {  // 18

    auto& dbClient = nma().getDBClient();
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );

    remoteInterface.setSwitchDelay( 0 );

    auto device = addDeviceWithSecurityThreatItem( __LINE__ );
    const JsonObject deviceObject =
        dbClient.findOne( CoreD::Model::Resources::DBNames::DevicesDB,
                          JSON( CoreD::Model::Resources::DeviceFields::IdFieldName << device.m_deviceId ),
                          nullptr );
    auto securityConfigId = json_get_str( deviceObject, CoreD::Model::Resources::DeviceFields::ProtectConfig );
    EXPECT_EQ( securityConfigId, "default" );  // verifying security config default
    auto securityConfigObject = CoreD::Protect::ProtectUtils::getSecurityConfigObject( dbClient, securityConfigId );
    EXPECT_EQ( json_get_str( securityConfigObject, "entryDelay" ),
               "normal" );  // verifying default security config has normal entry delay

    setItemValue( remoteInterface, device.m_itemId, true, __LINE__ );
    const auto CurrentModeId = "2";

    testHouseModeAlarmedPhaseStatusBroadcast(
        CurrentModeId, device.m_deviceId, "main", "begin", __LINE__ );  // no entry delay verify

    remoteInterface.setHouseMode( "1" );  // reset house mode to disarmed
    testHouseModesSwitchedBroadcast( "1", "begin", __LINE__ );
    testHouseModesSwitchedBroadcast( "1", "done", __LINE__ );
    ignoreHouseModesAlarmedBroadcast();
}

TEST_F( TestsHouseMode, panicButtonAlarm /*19*/ ) {
    CoreDRemoteInterface::CoreDRemoteInterface coredRemote( nma() );
    const std::string deviceCategory = "scene_controller";
    const std::string deviceSubcategory = "panic";
    auto expectedDelayValue = -1;

    const std::string houseModeId( "1" );
    const auto deviceId = addDevice(
        coredRemote, "Panic button", "panic", "134_3_24", deviceCategory, deviceSubcategory, false, __LINE__ );

    const auto valueType = CoreD::Model::ValueType::Enum::Bool;
    const auto itemId = addItem( coredRemote, deviceId, "security_threat", valueType, true, true, __LINE__ );

    using namespace CoreD::Model::Resources;
    checkDeviceUpdateBroadcast( deviceId, { { DeviceFields::ArmedFieldName, true } }, __LINE__ );

    coredRemote.setItemValue( JSON( ItemFields::IdFieldName << itemId << ItemFields::ValueFieldName << true ) );
    auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );

    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.modes.alarmed" );

    const Json& result = broadcast[ "result" ];
    ASSERT_NE( result, nullptr );
    EXPECT_EQ( json_get_str( result, "modeId" ), houseModeId );
    EXPECT_EQ( json_get_str( result, "deviceId" ), deviceId );
    EXPECT_EQ( json_get_str( result, "phase" ), "main" );
    EXPECT_EQ( json_get_int( result, "pendingDelay" ), expectedDelayValue );
    EXPECT_EQ( json_get_str( result, "status" ), "begin" );
}

}  // namespace Tests
}  // namespace HubPlatform
