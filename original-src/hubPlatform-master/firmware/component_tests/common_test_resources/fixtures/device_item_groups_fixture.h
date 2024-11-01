#pragma once

#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class DeviceItemGroupsFixture : public SceneManagerTestsFixture {
public:
    DeviceItemGroupsFixture();

    void SetUp() override;
    void TearDown() override;

    JsonObject waitForDeviceGroupCreatedBroadcast( int codeLine );
    JsonObject waitForDeviceGroupDeletedBroadcast( int codeLine );

    std::string createDeviceGroup( const JsonObject& request, int codeLine );
    void deleteDeviceGroup( const std::string& groupId, int codeLine );
    std::string createItemGroup( const JsonObject& request, int codeLine );
    void deleteItemGroup( const std::string& groupId, int codeLine );

    std::string makeDevice( const std::string& category, const std::string& subcategory, int codeLine );
    std::string makeIntItem( const std::string& deviceId, const std::string& itemName, int value, int codeLine );
    std::string makeBooleanItem( const std::string& deviceId, const std::string& itemName, bool value, int codeLine );
    std::string makeStringItem( const std::string& deviceId,
                                const std::string& itemName,
                                const std::string& value,
                                int codeLine );

    inline std::string quote( const std::string& str ) {
        return "\"" + str + "\"";
    }

protected:
    CoreDRemoteInterface::CoreDInterface& m_remote;
};

}  // namespace Tests
}  // namespace HubPlatform
