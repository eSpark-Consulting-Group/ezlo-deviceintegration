#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1 Protect Buttons
DONE        1.1 hub.modes.get/protectButtons
DONE        1.2 hub.modes.protect.buttons.set - add/update/update with the same value
DONE        1.3 hub.modes.protect.buttons.set - remove
DONE        1.4 hub.modes.protect.buttons.set - non-existent device
DONE        1.5 hub.modes.protect.buttons.set - bad service
DONE    2 SOS Button
DONE        2.1 Item update for protect_alarm
DONE    3 Panic Button
DONE        3.1 protect notification in disarmed state

*/

class ProtectButtonsTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

    void setUpPanicButton( const std::string& protectService, const std::string& message );

    void checkProtectButtonsBroadcast( const std::string& subClass,
                                       const std::string& deviceId,
                                       const std::string& protectService,
                                       const std::string& message );

    static void checkAnswerError( const Json& answer,
                                  int code,
                                  const std::string& data,
                                  const std::string& reason,
                                  const std::string& message );

    void setUpSOSButton( const std::string& protectService, const std::string& message );

    void checkPanicButtonItemUpdateBroadcast( const std::string& itemId, bool itemValue, bool protectNotification );

protected:
    std::string m_deviceId;
    std::string m_protectService;
};

void ProtectButtonsTests::SetUp() {
    CoreNmaTest::SetUp();
}

void ProtectButtonsTests::TearDown() {
    CoreNmaTest::clearExistingDevices( __LINE__ );
    m_deviceId.clear();
    m_protectService.clear();
    CoreNmaTest::TearDown();
}

void ProtectButtonsTests::setUpPanicButton( const std::string& protectService, const std::string& message ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device panicButton(
        "empty", "Panic Button", "controller.panic", "panic_button", "scene_controller", true );
    panicButton.setSubcategory( "panic" );
    m_deviceId = addDevice( coreRemoteInterface, panicButton, __LINE__ );

    auto service = CoreDHouseModes::toProtectService( protectService );
    auto answer = coreRemoteInterface.houseModesProtectButtonsSet( m_deviceId, service );

    checkProtectButtonsBroadcast( "hub.modes.protect.buttons.added", m_deviceId, protectService, message );

    Json error;
    ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << error << ": " << message;
    auto protectButtons = coreRemoteInterface.getHouseModesProtectButtons();
    ASSERT_FALSE( protectButtons.empty() ) << message;

    m_protectService = protectService;
}

void ProtectButtonsTests::checkProtectButtonsBroadcast( const std::string& subClass,
                                                        const std::string& deviceId,
                                                        const std::string& protectService,
                                                        const std::string& message ) {
    Json result;
    Json broadcast = waitForBroadcast( subClass, NMA_TEST_MAKE_MESSAGE( subClass ) + ", " + message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), subClass ) << message;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << message;
    EXPECT_EQ( json_get_str( result, "deviceId" ), m_deviceId ) << message;
    if( !protectService.empty() ) {
        EXPECT_EQ( json_get_str( result, "service" ), protectService ) << message;
    }
}

void ProtectButtonsTests::checkAnswerError( const Json& answer,
                                            int code,
                                            const std::string& data,
                                            const std::string& reason,
                                            const std::string& message ) {
    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    Json error;
    EXPECT_TRUE( json_get_obj( answer, "error", error ) ) << message;
    EXPECT_EQ( json_get_int( error, "code" ), code ) << message;
    EXPECT_EQ( json_get_str( error, "data" ), data ) << message;
    auto errorReason = json_get_str( error, "reason" );
    if( errorReason.empty() && !reason.empty() ) {
        errorReason = json_get_str( error, "message" );
    }
    EXPECT_EQ( errorReason, reason ) << message;
}

void ProtectButtonsTests::setUpSOSButton( const std::string& protectService, const std::string& message ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device sosButton(
        "empty", "SOS Button", "controller.panic", "sos_protect_alarm", "sos_button", true );
    sosButton.setSubcategory( "police_alarm" );
    m_deviceId = addDevice( coreRemoteInterface, sosButton, __LINE__ );

    auto service = CoreDHouseModes::toProtectService( protectService );
    auto answer = coreRemoteInterface.houseModesProtectButtonsSet( m_deviceId, service );

    checkProtectButtonsBroadcast( "hub.modes.protect.buttons.added", m_deviceId, protectService, message );

    Json error;
    ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << error << ": " << message;
    auto protectButtons = coreRemoteInterface.getHouseModesProtectButtons();
    ASSERT_FALSE( protectButtons.empty() ) << message;

    m_protectService = protectService;
}

void ProtectButtonsTests::checkPanicButtonItemUpdateBroadcast( const std::string& itemId,
                                                               bool itemValue,
                                                               bool protectNotification ) {
    auto broadcast = waitForBroadcast( "hub.item.updated", NMA_TEST_MAKE_MESSAGE( "item updated" ) );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemId );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), "security_threat" );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "protectNotification" ), protectNotification );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "protectService" ), "police" );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "value" ), itemValue );
}

TEST_F( ProtectButtonsTests, hubModesGet_ProtectButtons ) /* 1.1 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    const std::string protectService = CoreDHouseModes::Resources::ProtectService::Police;
    setUpPanicButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create panic button for the 'police' service" ) );

    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" } } );
    EXPECT_FALSE( answer[ "result" ].empty() );
    EXPECT_TRUE( answer[ "error" ].empty() );

    Json result, protectButtons;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( json_get_arr( result, "protectButtons", protectButtons ) );
    EXPECT_EQ( protectButtons.size(), 1 );
    EXPECT_EQ( protectButtons[ 0 ][ "id" ], m_deviceId );
    EXPECT_EQ( protectButtons[ 0 ][ "service" ], protectService );

    LOGGER_TRACE_RAW( cc::j( answer ) );
}

TEST_F( ProtectButtonsTests, hubModesGet_ProtectButtonsSet_Update ) /* 1.2 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    std::string protectService = CoreDHouseModes::Resources::ProtectService::Fire;
    setUpPanicButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create panic button for the 'police' service" ) );

    {
        protectService = CoreDHouseModes::Resources::ProtectService::Medical;
        ASSERT_NE( m_protectService, protectService ) << "Should differ";
        auto service = CoreDHouseModes::toProtectService( protectService );
        auto answer = coreRemoteInterface.houseModesProtectButtonsSet( m_deviceId, service );

        auto subClass = "hub.modes.protect.buttons.updated";
        checkProtectButtonsBroadcast( subClass,
                                      m_deviceId,
                                      protectService,
                                      NMA_TEST_MAKE_MESSAGE( std::string{ "expected broadcast: " } + subClass ) );

        Json error;
        ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << error;
        auto protectButtons = coreRemoteInterface.getHouseModesProtectButtons();
        ASSERT_FALSE( protectButtons.empty() );
    }

    {  // check no broadcast on changing to the same service
        protectService = CoreDHouseModes::Resources::ProtectService::Medical;
        auto service = CoreDHouseModes::toProtectService( protectService );
        auto answer = coreRemoteInterface.houseModesProtectButtonsSet( m_deviceId, service );

        checkNoBroadcastReceived(
            NMA_TEST_MAKE_MESSAGE( "Should be no hub.modes.protect.buttons.modified: same protect service" ),
            __LINE__ );
    }

    auto protectButtons = coreRemoteInterface.getHouseModesProtectButtons();
    ASSERT_EQ( protectButtons.size(), 1 );
}

TEST_F( ProtectButtonsTests, hubModesGet_ProtectButtonsSet_Remove ) /* 1.3 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    std::string protectService = CoreDHouseModes::Resources::ProtectService::Fire;
    setUpPanicButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create panic button for the 'police' service" ) );

    // Remove the panic button assignment from any service
    protectService = "";
    auto service = CoreDHouseModes::toProtectService( protectService );
    auto answer = coreRemoteInterface.houseModesProtectButtonsSet( m_deviceId, service );

    auto subClass = "hub.modes.protect.buttons.removed";
    checkProtectButtonsBroadcast(
        subClass, m_deviceId, "", NMA_TEST_MAKE_MESSAGE( std::string{ "expected broadcast: " } + subClass ) );

    Json error;
    ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << error;
    auto protectButtons = coreRemoteInterface.getHouseModesProtectButtons();
    ASSERT_EQ( protectButtons.size(), 0 );
}

TEST_F( ProtectButtonsTests, hubModesGet_ProtectButtonsSet_ErrorNonExistentDevice ) /* 1.4 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    std::string protectService = CoreDHouseModes::Resources::ProtectService::Fire;
    setUpPanicButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create panic button for the 'fire' service" ) );

    // Pass an invalid device id
    protectService = CoreDHouseModes::Resources::ProtectService::Medical;
    auto service = CoreDHouseModes::toProtectService( protectService );
    auto answer = coreRemoteInterface.houseModesProtectButtonsSet( "non-existent-device", service );

    LOGGER_TRACE_RAW( "Non-existent-device: " + cc::j( answer ) );
    checkNoBroadcastReceived(
        NMA_TEST_MAKE_MESSAGE( "Should be no hub.modes.protect.buttons.modified: same protect service" ), __LINE__ );

    checkAnswerError( answer, -32500, "ezlo.device.notfound", "No device found!", NMA_TEST_MAKE_MESSAGE( "" ) );
}

TEST_F( ProtectButtonsTests, hubModesGet_ProtectButtonsSet_ErrorsBadParams ) /* 1.5 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    std::string protectService = CoreDHouseModes::Resources::ProtectService::Fire;
    setUpPanicButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create panic button for the 'fire' service" ) );

    // Pass an invalid protect service string: only can do so via the ui command
    {
        const auto answer =
            nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.protect.buttons.set" },
                                           { "params",
                                             JSON( "deviceId" << m_deviceId << "service"
                                                              << "non-existent-protect-service-value" ) } } );

        LOGGER_TRACE_RAW( "non-existent-protect-service-value answer: " + cc::j( answer ) );

        checkAnswerError( answer,
                          -32602,
                          "rpc.params.invalid.service",
                          "Invalid name of a protect service",
                          NMA_TEST_MAKE_MESSAGE( "" ) );
        checkNoBroadcastReceived(
            NMA_TEST_MAKE_MESSAGE( "Should be no hub.modes.protect.buttons.added: invalid protect service" ),
            __LINE__ );
    }

    {  // Pass invalid type in a service field
        const Json badServiceValue = 111;
        const auto answer = nma().sendtoHubAndGetAnswer(
            { { "method", "hub.modes.protect.buttons.set" },
              { "params", JSON( "deviceId" << m_deviceId << "service" << badServiceValue ) } } );

        LOGGER_TRACE_RAW( "invalid type of a protect-service value, answer: " + cc::j( answer ) );

        checkAnswerError( answer,
                          -32602,
                          "rpc.params.invalid.service",
                          "Format of a required field is wrong",
                          NMA_TEST_MAKE_MESSAGE( "" ) );
        checkNoBroadcastReceived(
            NMA_TEST_MAKE_MESSAGE( "Should be no hub.modes.protect.buttons.added: invalid protect service" ),
            __LINE__ );
    }
}

TEST_F( ProtectButtonsTests, testProtectItemUpdateBroadcast ) /* 2.1 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    coreRemoteInterface.setSwitchDelay( 0 );
    coreRemoteInterface.setHouseMode( "2" );
    NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.switched" );
    NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.switched" );

    const std::string protectService = CoreDHouseModes::Resources::ProtectService::Police;
    setUpSOSButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create sos button for the 'police' service" ) );

    auto itemId = addItem(
        coreRemoteInterface, m_deviceId, "protect_alarm", CoreD::Model::ValueType::Enum::Token, true, true, __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( "alarm_active" ) );
    } );
    broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.item.updated" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "_id" ), itemId );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "name" ), "protect_alarm" );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "protectService" ), "police" );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "protectNotification" ), true );
    EXPECT_EQ( json_get_bool( broadcast[ "result" ], "userNotification" ), true );
    EXPECT_EQ( json_get_str( broadcast[ "result" ], "value" ), "alarm_active" );
    broadcast =
        waitForBroadcast( "hub.modes.alarmed", NMA_TEST_MAKE_MESSAGE( "ignoring housemodes alarmed broadcast" ) );
}

TEST_F( ProtectButtonsTests, panicButtonProtectNotification ) /* 3.1 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;
    if( coreRemoteInterface.getCurrentHouseModeId() != "1" ) {
        coreRemoteInterface.setAlarmDelay( 0 );
        coreRemoteInterface.setSwitchDelay( 0 );
        coreRemoteInterface.setHouseMode( "1" );
        NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.switched" );
        NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.switched" );
        NMA_TEST_WAIT_FOR_BROADCAST( "hub.modes.alarmed" );
    }

    const std::string protectService = CoreDHouseModes::Resources::ProtectService::Police;
    setUpPanicButton( protectService, NMA_TEST_MAKE_MESSAGE( "Create panic button for the 'police' service" ) );

    auto itemId = addItem(
        coreRemoteInterface, m_deviceId, "security_threat", CoreD::Model::ValueType::Enum::Bool, true, true, __LINE__ );

    checkDeviceUpdateBroadcast( m_deviceId, { { "armed", true } }, __LINE__ );

    EXPECT_EQ( coreRemoteInterface.getCurrentHouseModeId(), "1" );  // disarmed mode

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( true ) );
    } );
    checkPanicButtonItemUpdateBroadcast( itemId, true, true );
    testHouseModeAlarmedPhaseStatusBroadcast( "1", m_deviceId, "main", "begin", __LINE__ );

    coreRemoteInterface.updateItemValue( itemId, []( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( std::move( valueType ), Json( false ) );
    } );
    checkPanicButtonItemUpdateBroadcast( itemId, false, false );
}

}  // namespace Tests
}  // namespace HubPlatform
