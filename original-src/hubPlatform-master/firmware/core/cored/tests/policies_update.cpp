#include "cored/tests/fixtures/policy_operations_fixture.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Update policy success
 2. update policy 'sync' false success

*/

class UpdatePolicyTest : public PolicyOperationsFixture {
public:
    JsonObject savedData( const std::string& name, const std::string& protectConfig ) {
        const auto request = R"(
        {
            "name": "__NAME__",
            "protectConfig": "__PROTECT_CONFIG__"
        }
        )";

        KeyStringList keys = { { "__NAME__", name }, { "__PROTECT_CONFIG__", protectConfig } };
        return Json::parse( replaceTokensInString( request, keys ) );
    }
};

TEST_F( UpdatePolicyTest, updatePolicySuccess ) {  // 1
    auto saved = savedData( "name", "test" );
    auto updated = savedData( "name1", "test1" );
    auto updatedJson = getPolicyUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"name":"name1","protectConfig":"test1","sync":true})";
    EXPECT_EQ( updatedJson.dump(), expectedString );
}

TEST_F( UpdatePolicyTest, updatePolicySyncFalseSuccess ) {  // 2
    auto saved = savedData( "name", "test" );
    auto updated = savedData( "name", "test1" );
    auto updatedJson = getPolicyUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"protectConfig":"test1","sync":false})";
    EXPECT_EQ( updatedJson.dump(), expectedString );
}

}  // namespace UT
}  // namespace HubPlatform
