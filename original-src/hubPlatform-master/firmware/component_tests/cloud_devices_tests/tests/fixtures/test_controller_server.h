#include "test_utils/component_tests/ws_server/test_server.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Tests {

static const auto TCSRequestTimeout = std::chrono::seconds( 60 );
static const auto TCSBroadcastTimeout = std::chrono::seconds( 45 );

class TestControllerServer {
public:
    TestControllerServer();

    std::string getAddress() const;

    void start( const std::string& crt = "test_controller.crt",
                const std::string& key = "test_controller.key",
                const std::string& ca = "root.crt" );

    bool isConnected() const;
    void waitForConnection( const char* file,
                            int line,
                            bool connected,
                            std::chrono::seconds timeout = TCSRequestTimeout );

    JsonObject waitForRequest( const char* file, int line, std::chrono::seconds timeout = TCSRequestTimeout );

    void sendResponse( const JsonObject& request,
                       Json result = JSON_EMPTY_OBJ,
                       Json error = JSON_EMPTY_OBJ,
                       Json sender = JSON_EMPTY_OBJ );

    JsonObject sendtoHubAndGetAnswer( const char* file,
                                      int line,
                                      JsonObject request,
                                      std::chrono::seconds timeout = TCSRequestTimeout );

    void sendBroadcast( const std::string& msgSubclass, JsonObject result );

    JsonObject waitForBroadcast( const std::string& msgSubclass,
                                 const char* file = "",
                                 int line = 0,
                                 std::chrono::milliseconds timeout = TCSBroadcastTimeout );

private:
    void setConnected( bool connected );

private:
    std::unique_ptr< TestServer > m_server;
    int m_port;

    std::mutex m_mutex;
    std::condition_variable m_condVar;
    std::deque< JsonObject > m_requestMessages;
    bool m_connected;

    using AnswersCollection = std::unordered_map< std::string, JsonObject >;
    AnswersCollection m_answers;
    std::deque< JsonObject > m_broadcastMessages;
};

}  // namespace Tests
}  // namespace HubPlatform
