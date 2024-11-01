#pragma once

#include "cored/model/resources/cored_resources.h"
#include "database_client/fake_db/fake_db_client.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

#include <string>

namespace HubPlatform {
namespace UT {

class MeshbotTemplatesFixture : public ::testing::Test {
public:
    const std::string DeviceId = CoreD::Model::Resources::DeviceFields::IdFieldName;
    const std::string DeviceName = CoreD::Model::Resources::DeviceFields::NameFieldName;
    const std::string DeviceCategory = CoreD::Model::Resources::DeviceFields::CategoryFieldName;

    const std::string ItemId = CoreD::Model::Resources::ItemFields::IdFieldName;
    const std::string ItemDeviceId = CoreD::Model::Resources::ItemFields::DeviceIdFieldName;
    const std::string ItemName = CoreD::Model::Resources::ItemFields::NameFieldName;

    std::string getId() {
        static size_t id{ 0 };
        return std::to_string( ++id );
    }

    Json dummyDevice( const std::string& name, const std::string& category ) {
        return JSON( DeviceId << getId() << DeviceName << name << DeviceCategory << category );
    }

    Json dummyItem( const Json& device, const std::string& name ) {
        auto deviceId = json_get_str( device, DeviceId );
        return JSON( ItemId << getId() << ItemDeviceId << deviceId << ItemName << name );
    }

    std::string getDeviceId( const Json& device ) {
        return json_get_str( device, DeviceId );
    }

    std::string getItemId( const Json& item ) {
        return json_get_str( item, ItemId );
    }

    IDbClient& dbClient() {
        return m_fakeDbClient;
    }

private:
    HubPlatform::UT::FakeDbClient m_fakeDbClient;
};

}  // namespace UT
}  // namespace HubPlatform
