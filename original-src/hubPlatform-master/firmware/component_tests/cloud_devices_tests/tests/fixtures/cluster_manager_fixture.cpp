#include "cloud_devices_tests/tests/fixtures/cluster_manager_fixture.h"

#include "common_utils/hub_info_helper.h"

namespace HubPlatform {
namespace Tests {

ClusterManagerTest::ClusterManagerTest()
: m_iccInterface( nma() ) {}

void ClusterManagerTest::SetUp() {
    ExternalControllersTest::SetUp();
}

const std::string& ClusterManagerTest::getLocalControllerUuid() {
    return HubInfoHelper::getControllerUUID();
}

const std::string& ClusterManagerTest::getSerial() const {
    static std::string serial = "77777777";
    return serial;
}

const std::string& ClusterManagerTest::getLowerSerial() const {
    static std::string serial = "########";
    return serial;
}

const std::string& ClusterManagerTest::getHigherSerial() const {
    static std::string serial = "XXXXXXXX";
    return serial;
}

const std::string& ClusterManagerTest::getUUID() const {
    static std::string uuid = "77777777-7777-7777-7777-777777777777";
    return uuid;
}

TestControllerServer& ClusterManagerTest::server() {
    return m_server;
}

TestControllerClient& ClusterManagerTest::client() {
    return m_client;
}

ExternalControllers::RemoteInterface& ClusterManagerTest::externalControllers() {
    // NOTE: wait a little bit for firmware to process messages that were sent
    //       by other channels
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    return ExternalControllersTest::externalControllers();
}

InterControllerCommunication::RemoteInterface& ClusterManagerTest::icc() {
    // NOTE: wait a little bit for firmware to process messages that were sent
    //       by other channels
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    return m_iccInterface;
}

void ClusterManagerTest::setupRegisterAndHandleLogin( const char* file,
                                                      int line,
                                                      int version,
                                                      const std::string& controllerId,
                                                      const std::string& controllerUuid,
                                                      const std::vector< std::string >& keys ) {
    server().start();

    setCredentials( version, controllerId, controllerUuid, keys );
    setBridgeControllerConnection( controllerUuid, { server().getAddress() } );

    icc().registerControllerConnection( controllerId );

    server().waitForConnection( file, line, true );

    {
        auto request = server().waitForRequest( file, line );

        if( json_get_str( request, "method" ) != "hub.offline.login.hub" ) {
            ADD_FAILURE_AT( file, line ) << "Expected hub.offline.login.hub request";
        }
        EXPECT_EQ( json_get_str( request[ "params" ], "hub" ), getLocalControllerUuid() );
        EXPECT_EQ( json_get_str( request[ "params" ], "token" ), keys.front() );
        server().sendResponse( request );
    }
}

void ClusterManagerTest::unregisterAndTeardown( const char* file,
                                                int line,
                                                int version,
                                                const std::string& controllerId,
                                                const std::string& controllerUuid ) {
    icc().unregisterControllerConnection( controllerId );

    // TODO: reduce waitForConnection timeout to 10 sec: it doesn't help, tests start to fail, problem is not here
    server().waitForConnection( file, line, false /* , std::chrono::seconds( 20 ) */ );

    setBridgeControllerConnection( controllerUuid, {} );
    setCredentials( version, "", "", {}, FwCertsType::Broken );
}

void ClusterManagerTest::setupAndLogin( const char* file,
                                        int line,
                                        int version,
                                        const std::string& controllerId,
                                        const std::string& controllerUuid,
                                        const std::vector< std::string >& keys ) {
    setCredentials( version, controllerId, controllerUuid, keys );

    int localServerPort = 0;
    {
        auto broadcast = NMA_TEST_WAIT_FOR_INTERNAL_BROADCAST();
        if( broadcast.msgSubClass != LOCAL_SERVER_STARTED ) {
            ADD_FAILURE_AT( file, line ) << "Expected LOCAL_SERVER_STARTED broadcast";
        }
        localServerPort = json_get_int( broadcast.data, "port", 0 );
    }

    EXPECT_TRUE( client().start( localServerPort, controllerUuid, keys.front() ) );
}

}  // namespace Tests
}  // namespace HubPlatform
