#include "cored/tests/fixtures/group_operations_fixture.h"
#include "scenes_manager/tests/resources/generic_scene_json.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Create device group success
 2. Failed name length
 3. Categories error cases
 4. Devices error
 5. Exceptions error
 6. Create with wrong device error
*/

class CreateDeviceGroupTest : public GroupOperationsFixture {
public:
};

TEST_F( CreateDeviceGroupTest, createDeviceGroupSucces ) {  // 1
    auto category = R"({ "category": "scene_controller", "exceptions": [ "panic" ] })";
    auto request = createDeviceGroupRequest( "name", category, "", "" );
    Json answer = validateDeviceGroupCreate( request );
    EXPECT_TRUE( answer.empty() );

    category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    request = createDeviceGroupRequest( "name", category, "", "" );
    answer = validateDeviceGroupCreate( request );
    EXPECT_TRUE( answer.empty() );
}

TEST_F( CreateDeviceGroupTest, createDeviceGrouErrorName ) {  // 2
    auto category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    auto request =
        createDeviceGroupRequest( "namekhyfhfkuyfuyfdfsgdfhfgjghukhjlj;kjhjygulyvujyviyu,"
                                  "mnkjhbikuhgiuhgfvcluycutcjhvgukigliuyfvutctrxertztrzkuyfiuftdc;''ghjljklsqwe",
                                  category,
                                  "",
                                  "" );
    auto answer = validateDeviceGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "name" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.value.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is too long" );
}

TEST_F( CreateDeviceGroupTest, createDeviceGroupCategoriesError ) {  // 3
    auto category = R"({ "category": "scene_controller", "exceptions": [ 123 ] })";
    auto request = createDeviceGroupRequest( "name", category, "", "" );
    Json answer = validateDeviceGroupCreate( request );
    LOGGER_ERROR( "answer" + cc::j( answer ) );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "exceptions" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );

    category = R"({ "category": 123, "exceptions": [ 123 ] })";
    request = createDeviceGroupRequest( "name", category, "", "" );
    answer = validateDeviceGroupCreate( request );
    LOGGER_ERROR( "answer" + cc::j( answer ) );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "category" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );

    category = R"({ "category": "security_sensor", "subcategories": [ 123, "leak" ]  })";
    request = createDeviceGroupRequest( "name", category, "", "" );
    answer = validateDeviceGroupCreate( request );
    error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "subcategories" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateDeviceGroupTest, createDeviceGroupDevicesError ) {  // 4
    auto category = "";
    const auto device = "123";
    addDevice( device );
    auto request = createDeviceGroupRequest( "name", category, "123", "" );
    Json answer = validateDeviceGroupCreate( request );
    LOGGER_ERROR( "answer" + cc::j( answer ) );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "devices" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateDeviceGroupTest, createDeviceGroupExceptionsError ) {  // 5
    auto category = "";
    auto request = createDeviceGroupRequest( "name", category, R"("123")", "123" );
    Json answer = validateDeviceGroupCreate( request );
    LOGGER_ERROR( "answer" + cc::j( answer ) );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "exceptions" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateDeviceGroupTest, createDeviceGroupWrongDevice ) {  // 6
    auto category = "";
    auto request = createDeviceGroupRequest( "name", category, R"("123")", "" );
    Json answer = validateDeviceGroupCreate( request );
    LOGGER_ERROR( "answer" + cc::j( answer ) );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), R"(/"params"/"devices")" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.device.notfound" );
    EXPECT_EQ( json_get_str( error, "message" ), "No device found!" );
}

}  // namespace UT
}  // namespace HubPlatform