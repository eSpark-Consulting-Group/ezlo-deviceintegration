#include "cored/tests/fixtures/group_operations_fixture.h"
#include "cored/group_operations/device_group_updater.h"
#include "cored/group_operations/group_operations_utils.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Device group list with no filters
 2. Device group list with ids filter
 3. Device group list with ids filter error
 4. Device group list with version error
*/
static const auto Group1 = "group1";
static const auto Group2 = "group2";
static const auto Group3 = "group3";
static const auto Group4 = "group4";

static const auto GroupName1 = "groupName1";
static const auto GroupName2 = "groupName2";
static const auto GroupName3 = "groupName3";
static const auto GroupName4 = "groupName4";

class DeviceGroupListTest : public GroupOperationsFixture {
public:
    JsonObject devicesGroupsListTestRequestParams( const std::string ids, const std::string version = "" ) {
        std::string request;
        if( !ids.empty() ) {
            request = R"(
        {
            "ids": [__IDS__]
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

        return Json::parse( GroupOperationsFixture::replaceTokensInStringNoFail( request, keys ) );
    }
};

TEST_F( DeviceGroupListTest, listWithNoFilters ) {  // 1
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    auto answer = getDeviceGroupsList( JSON_EMPTY_OBJ );
    const auto Groups =
        R"([{"_id":"group1","categories":[],"devices":[],"entryDelay":"none","exceptions":[],"followEntry":false,"name":"groupName1","packageId":"","parentPolicyId":"","persistent":false,"role":"user"},{"_id":"group2","categories":[],"devices":[],"entryDelay":"none","exceptions":[],"followEntry":false,"name":"groupName2","packageId":"","parentPolicyId":"","persistent":false,"role":"user"}])";
    EXPECT_EQ( answer[ "deviceGroups" ].dump(), Groups );
}

TEST_F( DeviceGroupListTest, listWithFilters ) {  // 2
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );
    addDeviceGroup( Group3, GroupName3 );
    addDeviceGroup( Group4, GroupName4 );

    JsonArrayBuilder ids;
    const auto Ids = R"("group2", "group3")";

    auto requestParams = devicesGroupsListTestRequestParams( Ids );
    auto answer = getDeviceGroupsList( requestParams );
    const auto Groups =
        R"([{"_id":"group2","categories":[],"devices":[],"entryDelay":"none","exceptions":[],"followEntry":false,"name":"groupName2","packageId":"","parentPolicyId":"","persistent":false,"role":"user"},{"_id":"group3","categories":[],"devices":[],"entryDelay":"none","exceptions":[],"followEntry":false,"name":"groupName3","packageId":"","parentPolicyId":"","persistent":false,"role":"user"}])";
    EXPECT_EQ( answer[ "deviceGroups" ].dump(), Groups );
}

TEST_F( DeviceGroupListTest, listWithFiltersError ) {  // 3
    JsonArrayBuilder ids;
    const auto Ids = "123";
    auto requestParams = devicesGroupsListTestRequestParams( Ids );
    auto answer = getDeviceGroupsList( requestParams );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "ids" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( DeviceGroupListTest, listWithVersionError ) {  // 4
    const auto Ids = "";
    auto requestParams = devicesGroupsListTestRequestParams( Ids, R"(123)" );
    auto answer = getDeviceGroupsList( requestParams );
    const auto ExpectedAnswer =
        R"({"error":{"code":-32602,"data":"rpc.params.invalid","field":"version","message":"Wrong params","reason":"Wrong value type"}})";
    EXPECT_EQ( answer.dump(), ExpectedAnswer );
}

}  // namespace UT
}  // namespace HubPlatform
