#include "cored/tests/fixtures/group_operations_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
DONE   1. Find by group Id
DONE   2. Find by group Ids
DONE   3. Find by group category
DONE   4. Find by group subcategory
DONE   5. Find by group deviceId
DONE   6. Request errors checking
DONE   7. Find not exist category
DONE   8. Find with category with empty subcategories
DONE   9. Find not exist subcategory
*/

static const auto Group1 = "group1";
static const auto Group2 = "group2";
static const auto Group3 = "group3";
static const auto GroupName1 = "groupName1";
static const auto GroupName2 = "groupName2";
static const auto GroupName3 = "groupName3";

class DeviceGroupFindTest : public GroupOperationsFixture {};

TEST_F( DeviceGroupFindTest, findByGroupId ) {  // 1
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "deviceGroupId" << Group1 ) ) );

    auto Groups =
        R"({"deviceGroups":[{"id":"group1","name":"groupName1"}]})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findByGroupIds ) {  // 2
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );
    addDeviceGroup( Group3, GroupName3 );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "deviceGroupIds" << JSON_ARRAY_( "group1", "group3" ) ) ) );
    auto Groups = R"({"deviceGroups":[{"id":"group1","name":"groupName1"},{"id":"group3","name":"groupName3"}]})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findByCategory ) {  // 3
    JsonObject Categories3 = JSON_ARRAY( JSON( "category"
                                               << "security_sensor"
                                               << "subcategories" << JSON_ARRAY_( "co2", "leak" ) ) );
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    addDeviceGroupWithCategoriesAndDevices( Group3, GroupName3, Categories3, JSON_EMPTY_ARR, JSON_EMPTY_ARR );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "category"
                                                        << "security_sensor" ) ) );
    const auto Groups = R"({"deviceGroups":[{"id":"group3","name":"groupName3"}]})";

    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findBySubcategory ) {  // 4
    JsonObject Categories3 = JSON_ARRAY( JSON( "category"
                                               << "security_sensor"
                                               << "subcategories" << JSON_ARRAY_( "co2", "leak" ) ) );
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    addDeviceGroupWithCategoriesAndDevices( Group3, GroupName3, Categories3, JSON_EMPTY_ARR, JSON_EMPTY_ARR );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "subcategory"
                                                        << "co2" ) ) );
    const auto Groups =
        R"({"deviceGroups":[{"id":"group3","name":"groupName3"}]})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findByDevice ) {  // 5
    addDeviceGroupWithCategoriesAndDevices( Group1, GroupName1, JSON_EMPTY_ARR, JSON_EMPTY_ARR, JSON_EMPTY_ARR );
    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "deviceId"
                                                        << "123" ) ) );
    auto Groups = R"({"deviceGroups":[]})";
    EXPECT_EQ( answer.dump(), Groups );

    addDeviceGroupWithCategoriesAndDevices(
        Group2, GroupName2, JSON_EMPTY_ARR, JSON_ARRAY( "121" ), JSON_ARRAY( "111" ) );

    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "deviceId"
                                                        << "121" ) ) );
    Groups = R"({"deviceGroups":[{"id":"group2","name":"groupName2"}]})";
    EXPECT_EQ( answer.dump(), Groups );

    addDeviceGroupWithCategoriesAndDevices(
        Group3, GroupName3, JSON_EMPTY_ARR, JSON_ARRAY( "123" ), JSON_ARRAY_( "111", "123" ) );
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "deviceId"
                                                        << "123" ) ) );
    Groups = R"({"deviceGroups":[]})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findCheckErrors ) {  // 6
    auto checkField = [&]( const std::string& fieldName, int codeLine ) {
        const std::string codeLineStr = getCodeLineString( codeLine );
        auto answer = getDeviceGroupFind( JSON( fieldName << 123 ) );
        auto error = answer[ "error" ];
        ASSERT_TRUE( !error.empty() ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "field" ), fieldName ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "reason" ), "Wrong value type" ) << codeLineStr;

        EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( fieldName << "" ) ) );
        EXPECT_NO_THROW( error = answer[ "error" ] );
        ASSERT_TRUE( !error.empty() );
        EXPECT_EQ( json_get_str( error, "field" ), fieldName ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "reason" ), "Empty field value" ) << codeLineStr;
    };

    checkField( "deviceGroupId", __LINE__ );
    checkField( "category", __LINE__ );
    checkField( "subcategory", __LINE__ );
    checkField( "deviceId", __LINE__ );

    const std::string DeviceGroupIds = "deviceGroupIds";
    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( DeviceGroupIds << 123 ) ) );
    auto error = answer[ "error" ];
    ASSERT_TRUE( !error.empty() );
    EXPECT_EQ( json_get_str( error, "field" ), DeviceGroupIds );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Wrong value type" );

    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( DeviceGroupIds << JSON_EMPTY_ARR ) ) );
    EXPECT_NO_THROW( error = answer[ "error" ] );
    ASSERT_TRUE( !error.empty() );
    EXPECT_EQ( json_get_str( error, "field" ), DeviceGroupIds );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "reason" ), "Empty field value" );

    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON_EMPTY_OBJ ) );
    EXPECT_NO_THROW( error = answer[ "error" ] );
    ASSERT_TRUE( !error.empty() );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.notfound" );
}

TEST_F( DeviceGroupFindTest, findNotExistCategory ) {  // 7
    JsonObject Categories3 = JSON_ARRAY( JSON( "category"
                                               << "security_sensor"
                                               << "subcategories" << JSON_ARRAY_( "co2", "leak" ) ) );
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    addDeviceGroupWithCategoriesAndDevices( Group3, GroupName3, Categories3, JSON_EMPTY_ARR, JSON_EMPTY_ARR );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "category"
                                                        << "security_sensor1" ) ) );
    const auto Groups =
        R"({"deviceGroups":[]})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findByCategoryWithEmptySubcategory ) {  // 8
    JsonObject Categories3 = JSON_ARRAY( JSON( "category"
                                               << "security_sensor" ) );
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    addDeviceGroupWithCategoriesAndDevices( Group3, GroupName3, Categories3, JSON_EMPTY_ARR, JSON_EMPTY_ARR );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "category"
                                                        << "security_sensor"
                                                        << "subcategory"
                                                        << "co2" ) ) );
    const auto Groups = R"({"deviceGroups":[{"id":"group3","name":"groupName3"}]})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( DeviceGroupFindTest, findNotExistSubCategory ) {  // 9
    JsonObject Categories3 = JSON_ARRAY( JSON( "category"
                                               << "security_sensor"
                                               << "subcategories" << JSON_ARRAY_( "leak" ) ) );
    addDeviceGroup( Group1, GroupName1 );
    addDeviceGroup( Group2, GroupName2 );

    addDeviceGroupWithCategoriesAndDevices( Group3, GroupName3, Categories3, JSON_EMPTY_ARR, JSON_EMPTY_ARR );

    JsonObject answer;
    EXPECT_NO_THROW( answer = getDeviceGroupFind( JSON( "category"
                                                        << "security_sensor"
                                                        << "subcategory"
                                                        << "test_co2" ) ) );
    const auto Groups =
        R"({"deviceGroups":[]})";
    EXPECT_EQ( answer.dump(), Groups );
}

}  // namespace UT
}  // namespace HubPlatform
