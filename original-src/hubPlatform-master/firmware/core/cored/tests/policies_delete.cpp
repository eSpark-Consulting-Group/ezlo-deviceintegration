#include "cored/tests/fixtures/policy_operations_fixture.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include <gtest/gtest.h>

#include "service_interfaces/bus_wrapper.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/db_modifier/gateway_db_modifier.h"
#include "cored/db_modifier/device_db_modifier.h"
#include "cored/db_modifier/item_db_modifier.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
DONE     1. Delete a policy success
DONE     2. Delete a policy error
*/

class DeletePolicyTest : public PolicyOperationsFixture {
public:
    JsonObject createPolicyInfo( const std::string& id,
                                 const std::string& name,
                                 const std::string& persistent,
                                 const std::string& protectConfig,
                                 const std::string& deviceGroup,
                                 const std::string& meshbots ) {
        const auto request = R"(
        {
            "_id": __ID__,
            "name": __NAME__,
            "persistent": __PERSISTENT__,
            "protectConfig": __PROTECT_CONFIG__,
            "deviceGroup": __DEVICE_GROUP__,
            "meshbots": [__MESHBOT__]
        }
        )";

        KeyStringList keys = { { "__ID__", id },
                               { "__NAME__", name },
                               { "__PERSISTENT__", persistent },
                               { "__PROTECT_CONFIG__", protectConfig },
                               { "__DEVICE_GROUP__", deviceGroup },
                               { "__MESHBOT__", meshbots } };

        return Json::parse( replaceTokensInString( request, keys ) );
    }
};

TEST_F( DeletePolicyTest, deletePolicySuccess ) {  // 1
    const std::string Id = R"("123")";
    const std::string Name = R"("name")";
    const std::string Persistent = "false";
    const std::string protectConfig = R"("security1")";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1")";

    auto request = createPolicyInfo( Id, Name, Persistent, protectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_TRUE( answer.empty() );

    addPolicyJSON( request );
    const auto Policy1 =
        R"([{"_id":"123","deviceGroup":"device1","meshbots":true,"name":"name","persistent":false,"protectConfig":"security1"}])";
    EXPECT_EQ( getPolicyList( JSON_EMPTY_OBJ )[ "policies" ].dump(), Policy1 );

    deletePolicy( "123" );
    const auto Policy2 = R"([])";
    EXPECT_EQ( getPolicyList( JSON_EMPTY_OBJ )[ "policies" ].dump(), Policy2 );
}

TEST_F( DeletePolicyTest, deletePolicyError ) {  // 2
    const std::string Id = R"("123")";
    const std::string Name = R"("name")";
    const std::string Persistent = "false";
    const std::string protectConfig = R"("security1")";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1")";

    auto request = createPolicyInfo( Id, Name, Persistent, protectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_TRUE( answer.empty() );

    addPolicyJSON( request );
    const auto Policy1 =
        R"([{"_id":"123","deviceGroup":"device1","meshbots":true,"name":"name","persistent":false,"protectConfig":"security1"}])";
    EXPECT_EQ( getPolicyList( JSON_EMPTY_OBJ )[ "policies" ].dump(), Policy1 );

    deletePolicy( "124" );
    const auto Policy2 =
        R"([{"_id":"123","deviceGroup":"device1","meshbots":true,"name":"name","persistent":false,"protectConfig":"security1"}])";
    EXPECT_EQ( getPolicyList( JSON_EMPTY_OBJ )[ "policies" ].dump(), Policy2 );
}

}  // namespace UT
}  // namespace HubPlatform
