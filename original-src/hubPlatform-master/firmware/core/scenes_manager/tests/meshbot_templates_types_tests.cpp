#include "scenes_manager/scenes_processor/meshbot_templates/utils/utils.h"

#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN
DONE    1 MeshbotTemplate type tests
DONE        1.1 Parse meshbot template

*/

class MeshbotTemplatesTypesTests : public ::testing::Test {
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
};

TEST_F( MeshbotTemplatesTypesTests, parseMeshbotTemplate ) {  // 1.1
    using MeshbotTemplateRecord = CloudDevices::MeshbotTemplateRecord;
    using MeshbotTemplate = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate;
    using Rule = MeshbotTemplate::Rule;

    MeshbotTemplateRecord record;
    record.uuid = "<uuid>";
    record.title = "<title>";
    record.description = "<description>";
    record.id = "<id>";
    record.oemId = "<oem_id>";
    record.partnerUUID = "<partner_uuid>";
    record.localMeshbot = "<meshbot_template>";
    record.rules = R"([
            {
                "type": "device",
                "field": "category",
                "value": "<category>",
                "operation": "=="
            },
            {
                "type": "item",
                "field": "name",
                "value": [ "<name1>", "<name2>" ],
                "operation": "in"
            }
        ])"_json;

    MeshbotTemplate meshbotTemplate( record );

    EXPECT_EQ( meshbotTemplate.uuid, record.uuid );
    EXPECT_EQ( meshbotTemplate.title, record.title );
    EXPECT_EQ( meshbotTemplate.description, record.description );
    EXPECT_EQ( meshbotTemplate.id, record.id );
    EXPECT_EQ( meshbotTemplate.oemId, record.oemId );
    EXPECT_EQ( meshbotTemplate.partnerUUID, record.partnerUUID );
    EXPECT_EQ( meshbotTemplate.localMeshbot, record.localMeshbot );

    ASSERT_EQ( meshbotTemplate.rules.size(), 2 );

    {
        auto& rule = meshbotTemplate.rules.at( 0 );
        EXPECT_EQ( rule.type, Rule::Type::Device );
        EXPECT_EQ( rule.field, "category" );
        EXPECT_EQ( rule.value, "<category>" );
        EXPECT_EQ( rule.operation, Rule::Operation::Equal );
    }

    {
        auto& rule = meshbotTemplate.rules.at( 1 );
        EXPECT_EQ( rule.type, Rule::Type::Item );
        EXPECT_EQ( rule.field, "name" );
        EXPECT_EQ( rule.value,
                   JSON_ARRAY( "<name1>"
                               << "<name2>" ) );
        EXPECT_EQ( rule.operation, Rule::Operation::In );
    }
}

}  // namespace UT
}  // namespace HubPlatform
