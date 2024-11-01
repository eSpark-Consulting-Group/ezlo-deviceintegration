#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/house_modes/resources/house_mode_resources.h"

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1 Protect Devices
DONE        1.1 hub.modes.get/protectDevices
DONE        1.2 hub.modes.protect.devices.add - add a single device to the protectedDevices collection
DONE        1.3 hub.modes.protect.devices.remove - remove a single device from the protectedDevices collection
DONE        1.4 Add/remove multiple device IDs
DONE        1.5 Check errors
DONE        1.6 Nothing to remove
DONE        1.7 Remove already removed id
DONE        1.8 Add invalid device id
DONE        1.9 Remove invalid device id
*/

class ProtectDevicesTests : public CoreNmaTest {
public:
    using DeviceIDs = CoreDRemoteInterface::CoreDRemoteInterface::DeviceIDs;

public:
    void SetUp() override;
    void TearDown() override;

    void setUpProtectDevices( unsigned count, const std::string& message );
    void setUpDevices( unsigned count, const std::string& message );

    void checkProtectDevicesBroadcast( const std::string& subClass,
                                       const DeviceIDs& deviceIds,
                                       const std::string& message );

    static void checkAnswerNoError( const Json& answer, const std::string& message );
    static void checkAnswerError( const Json& answer,
                                  int code,
                                  const std::string& data,
                                  const std::string& reason,
                                  const std::string& message );

    Json protectDevicesMethod( const std::string& method, const DeviceIDs& deviceIds );

protected:
    std::string m_deviceId;
    DeviceIDs m_deviceIds;
};

void ProtectDevicesTests::SetUp() {
    CoreNmaTest::SetUp();
}

void ProtectDevicesTests::TearDown() {
    CoreNmaTest::clearExistingDevices( __LINE__ );
    m_deviceId.clear();
    CoreNmaTest::TearDown();
}

void ProtectDevicesTests::setUpDevices( unsigned count, const std::string& message ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    CoreD::Model::Device protectDevice( "empty", "Smoke sensor", "smoke", "134_3_24", "security_sensor", true );
    protectDevice.setSubcategory( "smoke" );

    for( unsigned i = 0; i < count; i++ ) {
        auto id = addDevice( coreRemoteInterface, protectDevice, __LINE__ );
        m_deviceIds.push_back( id );
        if( 0 == i ) {
            m_deviceId = id;
        }
    }
    auto protectDevicesIds = coreRemoteInterface.getHouseModesProtectDevicesIDs();
    ASSERT_TRUE( protectDevicesIds.empty() ) << message;
}

void ProtectDevicesTests::setUpProtectDevices( unsigned count, const std::string& message ) {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    setUpDevices( count, message );
    for( const auto& id : m_deviceIds ) {
        auto answer = coreRemoteInterface.houseModesProtectDevicesAdd( { id } );
        Json error;
        ASSERT_FALSE( json_get_obj( answer, "error", error ) ) << error << ": " << message;
        checkProtectDevicesBroadcast( "hub.modes.protect.devices.added", { id }, message );
    }

    auto protectDevicesIds = coreRemoteInterface.getHouseModesProtectDevicesIDs();
    ASSERT_FALSE( protectDevicesIds.empty() ) << message;
    ASSERT_EQ( protectDevicesIds.size(), m_deviceIds.size() ) << message;
}

void ProtectDevicesTests::checkProtectDevicesBroadcast( const std::string& subClass,
                                                        const DeviceIDs& deviceIds,
                                                        const std::string& message ) {
    Json result, resultDeviceIds;
    Json broadcast = waitForBroadcast( subClass, NMA_TEST_MAKE_MESSAGE( subClass ) + ", " + message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), subClass ) << message;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << message;
    EXPECT_TRUE( json_get_arr( result, "deviceIds", resultDeviceIds ) ) << message;
    EXPECT_EQ( resultDeviceIds.size(), deviceIds.size() ) << message;
}

void ProtectDevicesTests::checkAnswerError( const Json& answer,
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

void ProtectDevicesTests::checkAnswerNoError( const Json& answer, const std::string& message ) {
    Json error;
    EXPECT_FALSE( json_get_obj( answer, "error", error ) ) << message;
}

Json ProtectDevicesTests::protectDevicesMethod( const std::string& method,
                                                const ProtectDevicesTests::DeviceIDs& deviceIds ) {
    auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", method }, { "params", JSON( "deviceIds" << deviceIds ) } } );
    return answer;
}

TEST_F( ProtectDevicesTests, hubModesGet_ProtectDevices ) /* 1.1 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    setUpProtectDevices( 1, NMA_TEST_MAKE_MESSAGE( "Create devices" ) );

    const auto answer = nma().sendtoHubAndGetAnswer( { { "method", "hub.modes.get" } } );
    EXPECT_FALSE( answer[ "result" ].empty() );
    EXPECT_TRUE( answer[ "error" ].empty() );

    Json result, protectDevices;
    EXPECT_TRUE( json_get_obj( answer, "result", result ) );
    EXPECT_TRUE( json_get_arr( result, "protectDevices", protectDevices ) );
    EXPECT_EQ( protectDevices.size(), 1 );

    LOGGER_TRACE_RAW( cc::j( answer ) );
}

TEST_F( ProtectDevicesTests, protectDevicesAdd ) /* 1.2 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    setUpDevices( 1, NMA_TEST_MAKE_MESSAGE( "Create new item in the protectDevices" ) );

    {
        const auto answer = protectDevicesMethod( "hub.modes.protect.devices.add", JSON_ARRAY_( m_deviceId ) );
        checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "no error" ) );

        auto subClass = "hub.modes.protect.devices.added";
        checkProtectDevicesBroadcast( subClass, m_deviceIds, "device should be added to protectDevices" );

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_FALSE( protectDevices.empty() );
        EXPECT_EQ( protectDevices.size(), 1 );
    }
}

TEST_F( ProtectDevicesTests, protectDevicesRemove ) /* 1.3 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    setUpProtectDevices( 1, NMA_TEST_MAKE_MESSAGE( "Create Protect device" ) );

    const auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.modes.protect.devices.remove" }, { "params", { { "deviceIds", m_deviceIds } } } } );
    checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "no error" ) );

    auto subClass = "hub.modes.protect.devices.removed";
    checkProtectDevicesBroadcast(
        subClass, { m_deviceId }, NMA_TEST_MAKE_MESSAGE( std::string{ "expected broadcast: " } + subClass ) );

    auto devices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
    ASSERT_EQ( devices.size(), 0 );
}

TEST_F( ProtectDevicesTests, protectDevicesAddRemoveMultiple ) /* 1.4 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    const unsigned expectedDevices = 3;
    setUpDevices( expectedDevices, NMA_TEST_MAKE_MESSAGE( "Create new protectDevice" ) );

    {
        LOGGER_DEBUG_RAW( "Add multiple ids" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.add", m_deviceIds );
        LOGGER_TRACE_RAW( "protect.devices.add: Answer:" + cc::j( answer ) );
        checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "should be no error" ) );

        auto subClass = "hub.modes.protect.devices.added";
        checkProtectDevicesBroadcast(
            subClass, m_deviceIds, NMA_TEST_MAKE_MESSAGE( std::string{ "No broadcast " } + subClass ) );

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_FALSE( protectDevices.empty() );
        EXPECT_EQ( protectDevices.size(), expectedDevices );
    }

    {
        LOGGER_DEBUG_RAW( "Remove multiple ids" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.remove", m_deviceIds );
        LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
        checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "should be no error" ) );

        auto subClass = "hub.modes.protect.devices.removed";
        checkProtectDevicesBroadcast(
            subClass, m_deviceIds, NMA_TEST_MAKE_MESSAGE( std::string{ "No broadcast " } + subClass ) );
        m_deviceIds.clear();

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_TRUE( protectDevices.empty() ) << "All should be deleted";
    }
}

TEST_F( ProtectDevicesTests, protectDevicesAddErrors ) /* 1.5 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    setUpProtectDevices( 1, NMA_TEST_MAKE_MESSAGE( "Create new protectDevice" ) );

    {
        LOGGER_DEBUG_RAW( "Add a non-existent device id" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.add", { "" } );
        LOGGER_TRACE_RAW( "protect.devices.add: Answer:" + cc::j( answer ) );

        checkAnswerError( answer,
                          -32602,
                          "rpc.params.invalid.deviceIds",
                          "Format of a required field is wrong",
                          NMA_TEST_MAKE_MESSAGE( "Should have invalid deviceIds error" ) );

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_FALSE( protectDevices.empty() );
        EXPECT_EQ( protectDevices.size(), 1 );
    }
}

TEST_F( ProtectDevicesTests, protectDevicesNothingToRemove ) /* 1.6 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    const unsigned expectedDevices = 3;
    setUpProtectDevices( expectedDevices, NMA_TEST_MAKE_MESSAGE( "Create new protectDevice" ) );

    auto storedIds = m_deviceIds;
    {
        LOGGER_DEBUG_RAW( "Remove multiple ids" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.remove", m_deviceIds );
        LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
        checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "should be no error" ) );

        auto subClass = "hub.modes.protect.devices.removed";
        checkProtectDevicesBroadcast(
            subClass, m_deviceIds, NMA_TEST_MAKE_MESSAGE( std::string{ "No broadcast " } + subClass ) );
        m_deviceIds.clear();

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_TRUE( protectDevices.empty() ) << "All should be deleted";
    }

    {
        LOGGER_DEBUG_RAW( "Remove multiple ids again" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.remove", storedIds );
        LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
        checkAnswerError( answer,
                          -32602,
                          "rpc.params.invalid",
                          "Nothing to remove",
                          NMA_TEST_MAKE_MESSAGE( "Should have 'Nothing to remove' error" ) );

        m_deviceIds.clear();

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_TRUE( protectDevices.empty() ) << "All should be deleted";
    }
}

TEST_F( ProtectDevicesTests, protectDevicesArrayWithAlreadyRemovedOne ) /* 1.7 */ {
    CoreDRemoteInterface::CoreDRemoteInterface coreRemoteInterface( nma() );
    JsonObject broadcast;

    const unsigned expectedDevices = 3;
    setUpProtectDevices( expectedDevices, NMA_TEST_MAKE_MESSAGE( "Create new protectDevice" ) );

    auto storedIds = m_deviceIds;
    {
        LOGGER_DEBUG_RAW( "Remove only the last id" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.remove", { m_deviceIds.back() } );
        LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
        checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "should be no error" ) );

        auto subClass = "hub.modes.protect.devices.removed";
        checkProtectDevicesBroadcast( subClass,
                                      { m_deviceIds.back() },
                                      NMA_TEST_MAKE_MESSAGE( std::string{ "1/3 shoudls be removed " } + subClass ) );
        m_deviceIds.pop_back();

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_EQ( protectDevices.size(), m_deviceIds.size() ) << "One should be deleted";
    }

    {
        LOGGER_DEBUG_RAW( "Remove multiple ids, including the already removed one" );
        auto answer = protectDevicesMethod( "hub.modes.protect.devices.remove", storedIds );
        LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
        checkAnswerNoError( answer, NMA_TEST_MAKE_MESSAGE( "should ignore such error" ) );

        auto subClass = "hub.modes.protect.devices.removed";
        checkProtectDevicesBroadcast(
            subClass, m_deviceIds, NMA_TEST_MAKE_MESSAGE( std::string{ "2/3 should be removed" } + subClass ) );
        m_deviceIds.clear();

        auto protectDevices = coreRemoteInterface.getHouseModesProtectDevicesIDs();
        EXPECT_TRUE( protectDevices.empty() ) << "All should be deleted";
    }
}

TEST_F( ProtectDevicesTests, protectDevicesAddInvalidDeviceId ) /* 1.8 */ {
    LOGGER_DEBUG_RAW( "Add invalid id" );
    auto answer = protectDevicesMethod( "hub.modes.protect.devices.add", { "does-not-exist" } );
    LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
    checkAnswerError( answer,
                      -32602,
                      "rpc.params.invalid",
                      "Nothing to add",
                      NMA_TEST_MAKE_MESSAGE( "Should have invalid deviceIds error" ) );
}

TEST_F( ProtectDevicesTests, protectDevicesRemoveInvalidDeviceId ) /* 1.9 */ {
    LOGGER_DEBUG_RAW( "Remove invalid id" );
    auto answer = protectDevicesMethod( "hub.modes.protect.devices.remove", { "does-not-exist" } );
    LOGGER_TRACE_RAW( "protect.devices.remove: Answer:" + cc::j( answer ) );
    checkAnswerError( answer,
                      -32602,
                      "rpc.params.invalid",
                      "Nothing to remove",
                      NMA_TEST_MAKE_MESSAGE( "Should have invalid deviceIds error" ) );
}

}  // namespace Tests
}  // namespace HubPlatform
