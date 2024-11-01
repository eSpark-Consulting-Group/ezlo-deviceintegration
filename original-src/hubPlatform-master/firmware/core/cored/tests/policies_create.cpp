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
DONE     1. Create policy success
DONE     2. Missed name error
DONE     3. Failed name length error
DONE     4. Failed Persistent error
DONE     5. Failed ProtectConfig error
DONE     6. Failed DeviceGroup error
DONE     7. Create policy with Meshbots success
*/

class CreatePolicyTest : public PolicyOperationsFixture {
public:
    JsonObject createPolicyInfo( const std::string& name,
                                 const std::string& persistent,
                                 const std::string& protectConfig,
                                 const std::string& deviceGroup,
                                 const std::string& meshbots ) {
        const auto request = R"(
        {
            "name": __NAME__,
            "persistent": __PERSISTENT__,
            "protectConfig": __PROTECT_CONFIG__,
            "deviceGroup": __DEVICE_GROUP__,
            "meshbots": [__MESHBOT__]
        }
        )";

        KeyStringList keys = { { "__NAME__", name },
                               { "__PERSISTENT__", persistent },
                               { "__PROTECT_CONFIG__", protectConfig },
                               { "__DEVICE_GROUP__", deviceGroup },
                               { "__MESHBOT__", meshbots } };

        return Json::parse( replaceTokensInString( request, keys ) );
    }
};

TEST_F( CreatePolicyTest, createPolicySuccess ) {  // 1
    const std::string Name = R"("name")";
    const std::string Persistent = "true";
    const std::string ProtectConfig = R"("security1")";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1", "meshbot2")";

    auto request = createPolicyInfo( Name, Persistent, ProtectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_TRUE( answer.empty() );
}

TEST_F( CreatePolicyTest, createPolicyMissedName ) {  // 2

    auto request = JSON_EMPTY_OBJ;

    Json answer = validatePolicyCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "name" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
}

TEST_F( CreatePolicyTest, createPolicyErrorName ) {  // 3

    const std::string Name =
        R"("namedfgdflkjghdfrthybnrtioeyrtyhoijrthnkfcdgkhlrotpjhrpthjnrthyrtnamedfgdflkjghdfrthybnrtioeyrtyhoijrthnkfcdgkhlrotpjhrpthjnrthyrt")";
    const std::string Persistent = "true";
    const std::string ProtectConfig = R"("security1")";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1", "meshbot2")";

    auto request = createPolicyInfo( Name, Persistent, ProtectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "name" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.value.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is too long" );
}

TEST_F( CreatePolicyTest, createPolicyErrorPersistent ) {  // 4
    const std::string Name = R"("name")";
    const std::string Persistent = R"("true")";
    const std::string ProtectConfig = R"("security1")";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1", "meshbot2")";

    auto request = createPolicyInfo( Name, Persistent, ProtectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "persistent" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreatePolicyTest, createPolicyErrorProtectConfig ) {  // 5
    const std::string Name = R"("name")";
    const std::string Persistent = "true";
    const std::string ProtectConfig = R"(123)";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1", "meshbot2")";

    auto request = createPolicyInfo( Name, Persistent, ProtectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "protectConfig" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreatePolicyTest, createWithWrongDeviceGroupError ) {  // 6
    const std::string Name = R"("name")";
    const std::string Persistent = "true";
    const std::string ProtectConfig = R"("security1")";
    const std::string DeviceGroup = R"(123)";
    const std::string Meshbots = R"("meshbot1", "meshbot2")";

    auto request = createPolicyInfo( Name, Persistent, ProtectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "deviceGroup" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreatePolicyTest, createPolicyWithMeshbotsSuccess ) {  // 7
    const std::string Name = R"("name")";
    const std::string Persistent = "true";
    const std::string ProtectConfig = R"("security1")";
    const std::string DeviceGroup = R"("device1")";
    const std::string Meshbots = R"("meshbot1", "meshbot2")";

    auto request = createPolicyInfo( Name, Persistent, ProtectConfig, DeviceGroup, Meshbots );

    Json answer = validatePolicyCreate( request );
    EXPECT_TRUE( answer.empty() );
}

}  // namespace UT
}  // namespace HubPlatform
