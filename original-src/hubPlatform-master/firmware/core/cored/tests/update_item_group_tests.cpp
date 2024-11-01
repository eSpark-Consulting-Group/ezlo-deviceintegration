#include "cored/tests/fixtures/group_operations_fixture.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
DONE    1. Update item group success
DONE    2. update name only success
DONE    3. update hasGetter and ahaSetter success
DONE    4. update itemValueType success
DONE    5. update itemNames success
DONE    6. update enum success
DONE    7. update ValueTypeFamily success

*/

static const auto Name = R"("name ")";
static const auto HasGetter = "true";
static const auto HasSetter = "true";
static const auto ValueType = R"("int")";
static const auto ItemNames = R"("name1","name2")";
static const auto Enum = R"("1","2")";
static const auto ValueTypeFamily = R"("family1")";

class UpdateItemGroupTest : public GroupOperationsFixture {
public:
    JsonObject createItemGroupInfo( const std::string& name,
                                    const std::string& hasGetter,
                                    const std::string& hasSetter,
                                    const std::string& itemNames,
                                    const std::string& valueType,
                                    const std::string& theEnum,
                                    const std::string& valueTypeFamily ) {
        const auto request = R"(
        {
            "name": __NAME__,
            "hasGetter": __HAS_GETTER__,
            "hasSetter": __HAS_SETTER__,
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
            { "__ITEM_NAMES__", itemNames },
            { "__VALUE_TYPE__", valueType },
            { "__ENUM__", theEnum },
            { "__VALUE_TYPE_FAMILY__", valueTypeFamily },
        };
        return Json::parse( replaceTokensInString( request, keys ) );
    }
};

TEST_F( UpdateItemGroupTest, updateItemGroupSuccess ) {  // 1

    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );

    const auto Name = R"("name1")";
    const auto HasGetter = "false";
    const auto HasSetter = "false";
    const auto ValueType = R"("token")";
    const auto ItemNames = R"("name3","name4")";
    const auto TheEnum = R"("3","4")";
    const auto ValueTypeFamily = R"("strings")";

    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, TheEnum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"enum":["3","4"],"hasGetter":false,"hasSetter":false,"itemNames":["name3","name4"],"name":"name1","sync":true,"valueType":"token","valueTypeFamily":"strings"})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

TEST_F( UpdateItemGroupTest, updateItemOnlyNameSuccess ) {  // 2
    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );

    const auto Name = R"("name1")";
    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"name":"name1","sync":true})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

TEST_F( UpdateItemGroupTest, updateItemOnlyGetterSetterSuccess ) {  // 3
    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );

    const auto HasGetter = "false";
    const auto HasSetter = "false";

    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString = R"({"hasGetter":false,"hasSetter":false,"sync":false})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

TEST_F( UpdateItemGroupTest, updateItemValueTypeSuccess ) {  // 4
    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    const auto ValueType = R"("token")";
    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString = R"({"sync":false,"valueType":"token"})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

TEST_F( UpdateItemGroupTest, updateItemNamesSuccess ) {  // 5
    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    const auto ItemNames = R"("name3","name4")";
    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString = R"({"itemNames":["name3","name4"],"sync":false})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

TEST_F( UpdateItemGroupTest, updateItemEnumSuccess ) {  // 6
    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    const auto Enum = R"("3","4")";
    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString = R"({"enum":["3","4"],"sync":false})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

TEST_F( UpdateItemGroupTest, updateItemOnlyValueTypeFamilySuccess ) {  // 7
    auto saved = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    const auto ValueTypeFamily = R"("family2")";
    auto updated = createItemGroupInfo( Name, HasGetter, HasSetter, ItemNames, ValueType, Enum, ValueTypeFamily );
    auto broadcastData = getItemGroupUpdateBroadcastData( updated, saved );
    auto expectedString = R"({"sync":false,"valueTypeFamily":"family2"})";
    EXPECT_EQ( broadcastData.dump(), expectedString );
}

}  // namespace UT
}  // namespace HubPlatform