#include "cored/tests/fixtures/group_operations_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Item group get
 2. Item group get error param
 3. Item group get group not exist error

*/

class ItemGroupGetTest : public GroupOperationsFixture {};

TEST_F( ItemGroupGetTest, getSuccess ) {  // 1
    const auto Group1 = "group1";
    const auto Group2 = "group2";
    const auto GroupName1 = "groupName1";
    const auto GroupName2 = "groupName2";

    addItemGroup( Group1, GroupName1 );
    addItemGroup( Group2, GroupName2 );

    auto answer = getItemGroupGet( JSON( "id" << Group1 << "name" << GroupName1 ) );
    const auto Groups =
        R"({"_id":"group1","enum":[],"hasGetter":false,"hasSetter":false,"info":{},"itemNames":[],"name":"groupName1","persistent":false,"role":"","valueType":"","valueTypeFamily":""})";
    EXPECT_EQ( answer.dump(), Groups );
}

TEST_F( ItemGroupGetTest, getBadParams ) {  // 2
    auto checkError = []( const JsonObject& answer, int codeLine ) {
        std::string codeLineStr = "code line: " + std::to_string( codeLine );
        auto error = answer[ "error" ];
        EXPECT_EQ( json_get_str( error, "field" ), "id" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" ) << codeLineStr;
        EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" ) << codeLineStr;
    };

    auto answer = getDeviceGroupGet( JSON( "id" << 123 ) );
    checkError( answer, __LINE__ );

    answer = getDeviceGroupGet( JSON_EMPTY_OBJ );
    checkError( answer, __LINE__ );
}

TEST_F( ItemGroupGetTest, getGroupNotExist ) {  // 3
    const auto Group = "group1";
    auto answer = getItemGroupGet( JSON( "id" << Group ) );
    auto error = answer[ "error" ];
    ASSERT_TRUE( !error.empty() );
    EXPECT_EQ( json_get_str( error, "field" ), "_id" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.not.exist" );
    EXPECT_EQ( json_get_str( error, "message" ), "Record with the key does not exist" );
}

}  // namespace UT
}  // namespace HubPlatform