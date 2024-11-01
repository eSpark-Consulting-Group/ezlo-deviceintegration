#include "cored/tests/fixtures/group_operations_fixture.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*
TEST PLAN
 1. Update device group success
 2. update device 'sync' false success
 3. update device and exceptions removed success
 4. update device add exceptions added success

*/

class UpdateDeviceGroupTest : public GroupOperationsFixture {
public:
    JsonObject savedData( const std::string& name,
                          const std::string& category,
                          const std::string& devices,
                          const std::string& exceptions ) {
        const auto request = R"(
        {
            "name": "__NAME__",
            "categories": [__CATEGORIES_LIST__],
            "devices": [__DEVICES__],
            "exceptions": [__EXCEPTIONS__]

        }
        )";

        KeyStringList keys = { { "__NAME__", name },
                               { "__CATEGORIES_LIST__", category },
                               { "__DEVICES__", devices },
                               { "__EXCEPTIONS__", exceptions } };
        return Json::parse( replaceTokensInString( request, keys ) );
    }
};

TEST_F( UpdateDeviceGroupTest, updateDeviceGroupSuccess ) {  // 1
    auto category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    auto saved = savedData( "name", category, "", "" );
    category = R"({ "category": "security_sensor", "subcategories": [ "co", "leak" ]  })";
    auto updated = savedData( "name1", category, "", "" );
    auto updatedJson = getDeviceGroupUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"categories":[{"category":"security_sensor","subcategories":["co","leak"]}],"name":"name1","role":"","sync":true})";
    EXPECT_EQ( updatedJson.dump(), expectedString );
}

TEST_F( UpdateDeviceGroupTest, updateDeviceSyncFalseSuccess ) {  // 2
    auto category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    auto saved = savedData( "name", category, "", "" );
    category = R"({ "category": "security_sensor", "subcategories": [ "co", "leak" ]  })";
    auto updated = savedData( "name", category, "", "" );
    auto updatedJson = getDeviceGroupUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"categories":[{"category":"security_sensor","subcategories":["co","leak"]}],"role":"","sync":false})";
    EXPECT_EQ( updatedJson.dump(), expectedString );
}

TEST_F( UpdateDeviceGroupTest, updateDeviceExceptionsRemovedSuccess ) {  // 3
    auto category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    auto saved = savedData( "name", category, R"("device1")", R"("exception1")" );
    category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    auto updated = savedData( "name", category, "", "" );
    auto updatedJson = getDeviceGroupUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"devices":[],"exceptions":[],"removedDevices":["device1"],"removedExceptions":["exception1"],"role":"","sync":false})";
    EXPECT_EQ( updatedJson.dump(), expectedString );
}

TEST_F( UpdateDeviceGroupTest, updateDeviceExceptionsAddedSuccess ) {  // 4
    auto category = R"({ "category": "security_sensor", "subcategories": [ "co2", "leak" ]  })";
    auto saved = savedData( "name", category, "", "" );
    category = "";
    auto devices = R"("device1")";
    auto exceptions = R"("exception1")";
    auto updated = savedData( "name", category, devices, exceptions );
    auto updatedJson = getDeviceGroupUpdateBroadcastData( updated, saved );
    auto expectedString =
        R"({"addedDevices":["device1"],"addedExceptions":["exception1"],"categories":[],"devices":["device1"],"exceptions":["exception1"],"role":"","sync":false})";
    EXPECT_EQ( updatedJson.dump(), expectedString );
}

}  // namespace UT
}  // namespace HubPlatform
