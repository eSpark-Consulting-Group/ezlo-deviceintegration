#include "cloud_devices_tests/tests/fixtures/test_client.h"

#include "common_utils/cpp14.h"

#include <deque>

namespace HubPlatform {
namespace Tests {

static const auto TCCRequestTimeout = std::chrono::seconds( 60 );
static const auto TCCBroadcastTimeout = std::chrono::seconds( 45 );

class TestControllerClient {
public:
    TestControllerClient();

    bool start( int localServerPort,
                const std::string& controllerUuid,
                const std::string& token,
                const std::string& crt = "test_controller.crt",
                const std::string& key = "test_controller.key",
                const std::string& ca = "root.crt" );
    void stop();

    JsonObject waitForRequest( const char* file, int line, std::chrono::seconds timeout = TCCRequestTimeout );

    void sendResponse( const JsonObject& request,
                       Json result = JSON_EMPTY_OBJ,
                       Json error = JSON_EMPTY_OBJ,
                       Json sender = JSON_EMPTY_OBJ );

    JsonObject sendtoHubAndGetAnswer( const char* file,
                                      int line,
                                      JsonObject request,
                                      std::chrono::seconds timeout = TCCRequestTimeout );

    void sendBroadcast( const std::string& msgSubclass, JsonObject result );

    JsonObject waitForBroadcast( const std::string& msgSubclass,
                                 const char* file = "",
                                 int line = 0,
                                 std::chrono::milliseconds timeout = TCCBroadcastTimeout );

private:
    std::unique_ptr< TestClient > m_client;
    int m_port;

    std::mutex m_mutex;
    std::condition_variable m_condVar;
    std::deque< JsonObject > m_requestMessages;

    using AnswersCollection = std::unordered_map< std::string, JsonObject >;
    AnswersCollection m_answers;
    std::deque< JsonObject > m_broadcastMessages;
};

}  // namespace Tests
}  // namespace HubPlatform
