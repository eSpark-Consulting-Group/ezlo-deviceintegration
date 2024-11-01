#include "test_utils/component_tests/common_nma_test_fixture.h"

#include "json_utils/json_wrapper.h"

namespace HubPlatform {
namespace Tests {

static const auto RequestTimeout = std::chrono::seconds( 60 );
static const auto DbUpdateTimeout = std::chrono::milliseconds( 1000 );
static const auto AvoidedRequestTimeout = std::chrono::milliseconds( 500 );

class CommonCloudDevicesTests : public CommonNmaTests {
public:
    void SetUp() override;
    void TearDown() override;

    JsonObject waitForRequest( const char* file = "", int line = 0, std::chrono::seconds timeout = RequestTimeout );
    JsonObject waitForDbEntry( const std::string& dbName,
                               const Json& query,
                               const char* file = "",
                               int line = 0,
                               std::chrono::milliseconds timeout = DbUpdateTimeout );
    void checkNoRequestsReceived( const char* file = "",
                                  int line = 0,
                                  std::chrono::milliseconds timeout = AvoidedRequestTimeout );

    static void sendResponse( const JsonObject& request,
                              Json result = JSON_EMPTY_OBJ,
                              Json error = JSON_EMPTY_OBJ,
                              Json sender = JSON_EMPTY_OBJ );
    static void sendBroadcast( const std::string& msgSubclass, JsonObject result, JsonObject sender = JSON_EMPTY_OBJ );

    static void sendCloudRegistered();
    static void sendCloudUnregistered();

    void timeoutResponse();

    void setCurrentVersion( int version );
    int getCurrentVersion() const;
    void increaseCurrentVersion();

private:
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    std::deque< JsonObject > m_requestMessages;

private:
    std::chrono::seconds m_cloudRequestTimeout;
    int m_currentVersion;
};

#define TEST_WAIT_FOR_REQUEST() waitForRequest( __FILE__, __LINE__ )
#define TEST_WAIT_FOR_DB_ENTRY( dbName, query ) waitForDbEntry( dbName, query, __FILE__, __LINE__ )
#define TEST_WAIT_NO_REQUESTS() checkNoRequestsReceived( __FILE__, __LINE__ )

}  // namespace Tests
}  // namespace HubPlatform
