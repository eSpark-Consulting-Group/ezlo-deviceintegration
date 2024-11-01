#include "nma_api_tests/application.h"
#include "jsonrpc/jsonrpc.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include <database_client/dbqueryparser.h>

/*

TESTPLAN
1. test success

*/

namespace HubPlatform {
namespace Tests {

using namespace CoreD;
using namespace CoreDHouseModes;
using namespace Services;

class HubNetworkCommunicationManualTest : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;
    JsonObject sendCommunicationTest( const std::string& operationId, const std::string& interface, int tiimeout );
};

void HubNetworkCommunicationManualTest::SetUp() {
    CoreNmaTest::SetUp();
    callingFile( __FILE__ );
}

void HubNetworkCommunicationManualTest::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );
    CommonNmaTests::TearDown();
}

JsonObject HubNetworkCommunicationManualTest::sendCommunicationTest( const std::string& operationId,
                                                                     const std::string& interface,
                                                                     int timeout ) {
    CoreDRemoteInterface::CoreDInterface& remoteInterface = makeRemote( nma() );
    auto answer = nma().sendtoHubAndGetAnswer(
        { { "method", "hub.network.communication.test" },
          { "params", JSON( "operationId" << operationId << "interface" << interface << "timeout" << timeout ) } } );

    return answer;
}

}  // namespace Tests
}  // namespace HubPlatform
