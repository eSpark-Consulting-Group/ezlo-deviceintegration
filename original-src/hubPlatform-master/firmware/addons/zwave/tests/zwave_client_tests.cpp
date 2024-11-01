
#include "zwave/tests/fixtures/zwave_client_fixture.h"

#include "zwave/zwave_client.h"
#include <znet/znet_node.hpp>

#include "test_utils/test_utils.h"

#include <gtest/gtest.h>

/*

TESTPLAN

DONE    1. Get node info as json

*/

namespace HubPlatform {
namespace Zwave {
namespace UT {

using namespace ::testing;

class ZwaveClientTestSuite : public ZwaveClientFixture {};

TEST_F( ZwaveClientTestSuite, testGetNodeInfoAsJson /*1*/ ) {
    const JsonObject expectedNodeInfo = R"({
        "id": "1",
        "basic_type": 0,
        "generic_type": "GENERIC_TYPE_UNKNOWN",
        "icon_type": null,
        "is_battery_powered": false,
        "command_span": 0,
        "is_reachable": false,
        "is_smart_include": false,
        "manufacturer_id": 0,
        "product_id": 0,
        "product_type": 0,
        "is_rediscover_required": false,
        "security": [ "none" ],
        "security_class_started": [ "none" ],
        "specific_type": "SPECIFIC_TYPE_UNKNOWN"
    })"_json;

    constexpr uint8_t nodeId{ 1 };

    addNode( nodeId );
    auto zwaveClient = createZwaveClient();

    EXPECT_EQ( expectedNodeInfo, zwaveClient.getNodeInfoAsJson( nodeId ) );
}

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
