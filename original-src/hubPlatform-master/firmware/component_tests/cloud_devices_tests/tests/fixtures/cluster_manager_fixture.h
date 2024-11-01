#include "cloud_devices_tests/tests/fixtures/external_controllers_fixture.h"
#include "cloud_devices_tests/tests/fixtures/test_controller_server.h"
#include "cloud_devices_tests/tests/fixtures/test_controller_client.h"
#include "test_utils/component_tests/ws_server/test_server.h"
#include "uid/icc_interface/icc_remote_interface.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Tests {

class ClusterManagerTest : public ExternalControllersTest {
public:
    ClusterManagerTest();

    void SetUp() override;

    const std::string& getLocalControllerUuid();

    const std::string& getSerial() const;
    const std::string& getLowerSerial() const;
    const std::string& getHigherSerial() const;
    const std::string& getUUID() const;

    TestControllerServer& server();
    TestControllerClient& client();

    ExternalControllers::RemoteInterface& externalControllers() override;
    InterControllerCommunication::RemoteInterface& icc();

    void setupRegisterAndHandleLogin( const char* file,
                                      int line,
                                      int version,
                                      const std::string& controllerId,
                                      const std::string& controllerUuid,
                                      const std::vector< std::string >& keys );

    void setupAndLogin( const char* file,
                        int line,
                        int version,
                        const std::string& controllerId,
                        const std::string& controllerUuid,
                        const std::vector< std::string >& keys );

    void unregisterAndTeardown( const char* file,
                                int line,
                                int version,
                                const std::string& controllerId,
                                const std::string& controllerUuid );

    void waitForPredicate( int timeout, std::function< bool() > predicate ) {
        const int waitForTimeout = timeout, piece = waitForTimeout < 100 ? waitForTimeout : 100;
        for( int i = 0; i < waitForTimeout / piece; i++ ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( piece ) );
            if( predicate != nullptr && !predicate() )
                break;
        }
    }

    void expectServerNotConnected( int timeoutMs, const char* file, int line ) {
        const int waitForTimeout = timeoutMs < 0 ? 5000 : timeoutMs;
        waitForPredicate( waitForTimeout, [&]() -> bool { return server().isConnected(); } );
        EXPECT_FALSE( server().isConnected() ) << file << ":" << line;
    }

private:
    TestControllerServer m_server;
    TestControllerClient m_client;

private:
    InterControllerCommunication::RemoteInterface m_iccInterface;
};

#define TEST_CONTROLLER_SERVER_WAIT_FOR_REQUEST() server().waitForRequest( __FILE__, __LINE__ )
#define TEST_CONTROLLER_SERVER_WAIT_FOR_CONNECTION( connected ) \
    server().waitForConnection( __FILE__, __LINE__, connected )
#define TEST_CONTROLLER_SERVER_SEND_TO_HUB_AND_WAIT_ANSWER( request ) \
    server().sendtoHubAndGetAnswer( __FILE__, __LINE__, request )
#define TEST_CONTROLLER_SERVER_WAIT_FOR_BROADCAST( messageSubclass ) \
    server().waitForBroadcast( messageSubclass, __FILE__, __LINE__ )

#define TEST_CONTROLLER_CLIENT_WAIT_FOR_REQUEST() client().waitForRequest( __FILE__, __LINE__ )
#define TEST_CONTROLLER_CLIENT_WAIT_FOR_CONNECTION( connected ) \
    client().waitForConnection( __FILE__, __LINE__, connected )
#define TEST_CONTROLLER_CLIENT_SEND_TO_HUB_AND_WAIT_ANSWER( request ) \
    client().sendtoHubAndGetAnswer( __FILE__, __LINE__, request )
#define TEST_CONTROLLER_CLIENT_WAIT_FOR_BROADCAST( messageSubclass ) \
    client().waitForBroadcast( messageSubclass, __FILE__, __LINE__ )

#define TEST_SETUP_REGISTER_AND_HANDLE_LOGIN( ... ) setupRegisterAndHandleLogin( __FILE__, __LINE__, __VA_ARGS__ )
#define TEST_UNREGISTER_AND_TEARDOWN( ... ) unregisterAndTeardown( __FILE__, __LINE__, __VA_ARGS__ )

#define TEST_SETUP_AND_LOGIN( ... ) setupAndLogin( __FILE__, __LINE__, __VA_ARGS__ )

}  // namespace Tests
}  // namespace HubPlatform
