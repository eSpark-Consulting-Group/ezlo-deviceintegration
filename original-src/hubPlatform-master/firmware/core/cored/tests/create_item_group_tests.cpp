#include "cored/tests/fixtures/group_operations_fixture.h"
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
DONE     1. Create item group success
DONE     2. Missed name error
DONE     3. Failed name length error
DONE     4. Failed hasGetter error
DONE     5. Failed hasSetter error
DONE     6. Failed Persistent error
DONE     7. Failed ItemNames error
DONE     8. Missed ItemNames error
DONE     9. Failed Enum error
DONE     10. Failed ValueTypeFamily error
DONE     11. Failed ValueType error
DONE     12. Failed ValueType is not contains in family error
DONE     13. Missed ValueTypeFamily error
DONE     14. Create item with Enum success
*/

class CreateItemGroupTest : public GroupOperationsFixture {
public:
    JsonObject createItemGroupInfo( const std::string& name,
                                    const std::string& hasGetter,
                                    const std::string& hasSetter,
                                    const std::string& persistent,
                                    const std::string& itemNames,
                                    const std::string& valueType,
                                    const std::string& theEnum,
                                    const std::string& valueTypeFamily ) {
        const auto request = R"(
        {
            "name": __NAME__,
            "hasGetter": __HAS_GETTER__,
            "hasSetter": __HAS_SETTER__,
            "persistent": __PERSISTENT__,
            "itemNames": [__ITEM_NAMES__],
            "valueType": __VALUE_TYPE__,
            "enum": [__ENUM__],
            "valueTypeFamily": __VALUE_TYPE_FAMILY__
        }
        )";

        KeyStringList keys = {
            { "__NAME__", name },
            { "__HAS_GETTER__", hasGetter },
            { "__HAS_SETTER__", hasSetter },
            { "__PERSISTENT__", persistent },
            { "__ITEM_NAMES__", itemNames },
            { "__VALUE_TYPE__", valueType },
            { "__ENUM__", theEnum },
            { "__VALUE_TYPE_FAMILY__", valueTypeFamily },
        };

        return Json::parse( replaceTokensInString( request, keys ) );
    }
};

TEST_F( CreateItemGroupTest, createItemGroupSucces ) {  // 1
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("token")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("strings")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_TRUE( answer.empty() );
}

TEST_F( CreateItemGroupTest, createItemGroudMissedName ) {  // 2

    auto request = JSON_EMPTY_OBJ;

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "name" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
}

TEST_F( CreateItemGroupTest, createItemGrouErrorName ) {  // 3

    const std::string Name =
        R"("namedfgdflkjghdfrthybnrtioeyrtyhoijrthnkfcdgkhlrotpjhrpthjnrthyrtnamedfgdflkjghdfrthybnrtioeyrtyhoijrthnkfcdgkhlrotpjhrpthjnrthyrt")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "name" );
    EXPECT_EQ( json_get_str( error, "data" ), "ezlo.value.too.long" );
    EXPECT_EQ( json_get_str( error, "message" ), "Value is too long" );
}

TEST_F( CreateItemGroupTest, createItemGrouErrorHasGetter ) {  // 4
    const std::string Name = R"("name")";
    const std::string HasGetter = R"("true")";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "hasGetter" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createItemGrouErrorHasSetter ) {  // 5
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = R"("true")";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "hasSetter" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createItemGrouErrorPersistent ) {  // 6
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = R"("true")";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "persistent" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createItemGrouErrorItemNames ) {  // 7
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"(1, 2)";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"("name1", "name2")";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "itemNames" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createItemGroudMissedItemNames ) {  // 8

    auto request = JSON( "name"
                         << "name" );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "itemNames" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
}

TEST_F( CreateItemGroupTest, createItemGroupEnumError ) {  // 9
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"(1, 2)";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "enum" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createItemGroupError ) {  // 10
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = "123";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("temperature")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "valueType" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createValueTypeFamilyError ) {  // 10
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("int")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = "123";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "valueTypeFamily" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createWithEwongValueTypeError ) {  // 11
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("boolean")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("strings")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), R"(/"params"/"valueType")" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createWithEwongValueTypeFamilyError ) {  // 12
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("token")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("numeric")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), R"(/"params"/"valueTypeFamily")" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.invalid" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong params" );
}

TEST_F( CreateItemGroupTest, createItemGroudMissedValueTypeFamily ) {  // 13

    auto request = JSON( "name"
                         << "name"
                         << "itemNames" << JSON_ARRAY( "item1" ) );

    Json answer = validateItemGroupCreate( request );
    EXPECT_FALSE( answer.empty() );
    auto error = answer[ "error" ];
    EXPECT_EQ( json_get_str( error, "field" ), "valueTypeFamily" );
    EXPECT_EQ( json_get_str( error, "data" ), "rpc.params.wrong_field" );
    EXPECT_EQ( json_get_str( error, "message" ), "Wrong field of object" );
}

TEST_F( CreateItemGroupTest, createItemGroupWithEnumSucces ) {  // 14
    const std::string Name = R"("name")";
    const std::string HasGetter = "true";
    const std::string HasSetter = "true";
    const std::string Persistent = "true";
    const std::string ItemNames = R"("name1", "name2")";
    const std::string ValueType = R"("string")";
    const std::string Enum = R"("item1", "item1")";
    const std::string ValueTypeFamily = R"("strings")";

    auto request =
        createItemGroupInfo( Name, HasGetter, HasSetter, Persistent, ItemNames, ValueType, Enum, ValueTypeFamily );

    Json answer = validateItemGroupCreate( request );
    EXPECT_TRUE( answer.empty() );
}

}  // namespace UT
}  // namespace HubPlatform