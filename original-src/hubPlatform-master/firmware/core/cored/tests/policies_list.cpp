#include "cored/tests/fixtures/policy_operations_fixture.h"
#include "cored/policies/policies_updater.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Policy list with no filters
 2. Policy list with ids filter
 3. Policy list with ids filter error
 4. Policy list with version error
*/
static const auto Policy1 = "policy1";
static const auto Policy2 = "policy2";
static const auto Policy3 = "policy3";
static const auto Policy4 = "policy4";

static const auto PolicyName1 = "policyName1";
static const auto PolicyName2 = "policyName2";
static const auto PolicyName3 = "policyName3";
static const auto PolicyName4 = "policyName4";

class PolicyListTest : public PolicyOperationsFixture {
public:
    JsonObject policiesListTestRequestParams( const std::string ids, const std::string version = "" ) {
        std::string request;
        if( !ids.empty() ) {
            request = R"(
        {
            "policyIds": [__IDS__]
        }
        )";
        }
        else if( !version.empty() ) {
            request = R"(
        {
            "version": __VERSION__
        }
        )";
        }

        KeyStringList keys = { { "__IDS__", ids }, { "__VERSION__", version } };

        return Json::parse( PolicyOperationsFixture::replaceTokensInStringNoFail( request, keys ) );
    }
};

TEST_F( PolicyListTest, listWithNoFilters ) {  // 1

    /* remove test policies */
    deletePolicy( Policy1 );
    deletePolicy( Policy2 );
    deletePolicy( Policy3 );
    deletePolicy( Policy4 );

    addPolicy( Policy1, PolicyName1 );
    addPolicy( Policy2, PolicyName2 );

    auto answer = getPolicyList( JSON_EMPTY_OBJ );
    const auto Policies =
        R"([{"_id":"policy1","deviceGroup":"","meshbots":false,"name":"policyName1","persistent":false,"protectConfig":""},{"_id":"policy2","deviceGroup":"","meshbots":false,"name":"policyName2","persistent":false,"protectConfig":""}])";
    EXPECT_EQ( answer[ "policies" ].dump(), Policies );
}

TEST_F( PolicyListTest, listWithFilters ) {  // 2
    addPolicy( Policy1, PolicyName1 );
    addPolicy( Policy2, PolicyName2 );
    addPolicy( Policy3, PolicyName3 );
    addPolicy( Policy4, PolicyName4 );

    JsonArrayBuilder ids;
    const auto Ids = R"("policy2", "policy3")";

    auto requestParams = policiesListTestRequestParams( Ids );
    auto answer = getPolicyList( requestParams );
    const auto Policies =
        R"([{"_id":"policy2","deviceGroup":"","meshbots":false,"name":"policyName2","persistent":false,"protectConfig":""},{"_id":"policy3","deviceGroup":"","meshbots":false,"name":"policyName3","persistent":false,"protectConfig":""}])";
    EXPECT_EQ( answer[ "policies" ].dump(), Policies );
}

TEST_F( PolicyListTest, listWithFiltersError ) {  // 3
    JsonArrayBuilder ids;
    const auto Ids = "123";
    auto requestParams = policiesListTestRequestParams( Ids );
    auto answer = getPolicyList( requestParams );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "policyIds" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( PolicyListTest, listWithVersionError ) {  // 4
    const auto Ids = "";
    auto requestParams = policiesListTestRequestParams( Ids, R"(123)" );
    auto answer = getPolicyList( requestParams );
    const auto ExpectedAnswer =
        R"({"error":{"code":-32602,"data":"rpc.params.invalid","field":"version","message":"Wrong params","reason":"Wrong value type"}})";
    EXPECT_EQ( answer.dump(), ExpectedAnswer );

    /* remove test policies */
    deletePolicy( Policy1 );
    deletePolicy( Policy2 );
    deletePolicy( Policy3 );
    deletePolicy( Policy4 );
}

}  // namespace UT
}  // namespace HubPlatform
