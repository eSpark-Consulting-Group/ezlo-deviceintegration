#include "scenes_manager/tests/fixtures/meshbot_templates_fixture.h"

#include "scenes_manager/scenes_processor/meshbot_templates/utils/utils.h"

#include "json_utils/json_utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN
DONE    1 Find meshbot triggers tests
DONE        1.1 Query all
DONE        1.2 Query all with no devices
DONE        1.3 Query device by name
DONE        1.4 Query multiple devices by name
DONE        1.5 Query multiple devices by multiple names
DONE        1.6 Query device by name and category
DONE        1.7 Query device by complex query
DONE        1.8 Query item by name
DONE        1.9 Query devices and items by complex query
DONE    2 Bindings database operations tests
DONE        2.1 Store/read/delete bindings operations
DONE        2.2 Query bindings

*/

class MeshbotTemplatesUtilsTests : public MeshbotTemplatesFixture {};

TEST_F( MeshbotTemplatesUtilsTests, queryAll ) {  // 1.1
    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;

    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 2 );

    EXPECT_EQ( triggers.at( 0 ).deviceId, devices.at( 0 ).at( DeviceId ) );
    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 0 ) );
    EXPECT_EQ( triggers.at( 0 ).itemId, items.at( 0 ).at( ItemId ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 0 ) );

    EXPECT_EQ( triggers.at( 1 ).deviceId, devices.at( 1 ).at( DeviceId ) );
    EXPECT_EQ( triggers.at( 1 ).device, devices.at( 1 ) );
    EXPECT_EQ( triggers.at( 1 ).itemId, items.at( 1 ).at( ItemId ) );
    EXPECT_EQ( triggers.at( 1 ).item, items.at( 1 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryAllWithNoDevices ) {  // 1.2
    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;

    std::vector< Json > devices;
    std::vector< Json > items;

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 0 );
}

TEST_F( MeshbotTemplatesUtilsTests, queryDeviceByName ) {  // 1.3
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    devices.emplace_back( dummyDevice( "deviceNameC", "deviceCategoryC" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    meshbotTemplate.rules.emplace_back();
    auto& rule = meshbotTemplate.rules.back();
    rule.type = Rule::Type::Device;
    rule.field = DeviceName;
    rule.value = "deviceNameB";
    rule.operation = Rule::Operation::Equal;

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 1 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 1 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 1 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryMultipleDevicesByName ) {  // 1.4
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameCommon", "deviceCategoryB" ) );
    devices.emplace_back( dummyDevice( "deviceNameCommon", "deviceCategoryC" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    meshbotTemplate.rules.emplace_back();
    auto& rule = meshbotTemplate.rules.back();
    rule.type = Rule::Type::Device;
    rule.field = DeviceName;
    rule.value = "deviceNameCommon";
    rule.operation = Rule::Operation::Equal;

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 2 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 1 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 1 ) );

    EXPECT_EQ( triggers.at( 1 ).device, devices.at( 2 ) );
    EXPECT_EQ( triggers.at( 1 ).item, items.at( 2 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryMultipleDevicesByMultipleNames ) {  // 1.5
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    devices.emplace_back( dummyDevice( "deviceNameC", "deviceCategoryC" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    meshbotTemplate.rules.emplace_back();
    auto& rule = meshbotTemplate.rules.back();
    rule.type = Rule::Type::Device;
    rule.field = DeviceName;
    rule.value = JSON_ARRAY( "deviceNameB"
                             << "deviceNameC" );
    rule.operation = Rule::Operation::In;

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 2 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 1 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 1 ) );

    EXPECT_EQ( triggers.at( 1 ).device, devices.at( 2 ) );
    EXPECT_EQ( triggers.at( 1 ).item, items.at( 2 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryDeviceByNameAndCategory ) {  // 1.6
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryCommon" ) );
    devices.emplace_back( dummyDevice( "deviceNameCommon", "deviceCategoryCommon" ) );
    devices.emplace_back( dummyDevice( "deviceNameCommon", "deviceCategoryCommon" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Device;
        rule.field = DeviceName;
        rule.value = "deviceNameCommon";
        rule.operation = Rule::Operation::Equal;
    }

    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Device;
        rule.field = DeviceCategory;
        rule.value = "deviceCategoryCommon";
        rule.operation = Rule::Operation::Equal;
    }

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 2 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 1 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 1 ) );

    EXPECT_EQ( triggers.at( 1 ).device, devices.at( 2 ) );
    EXPECT_EQ( triggers.at( 1 ).item, items.at( 2 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryDeviceByComplexQuery ) {  // 1.7
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    devices.emplace_back( dummyDevice( "deviceNameC", "deviceCategoryC" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Device;
        rule.field = DeviceName;
        rule.value = JSON_ARRAY( "deviceNameB"
                                 << "deviceNameC" );
        rule.operation = Rule::Operation::In;
    }

    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Device;
        rule.field = DeviceCategory;
        rule.value = "deviceCategoryC";
        rule.operation = Rule::Operation::Equal;
    }

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 1 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 2 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 2 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryItemByName ) {  // 1.8
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    devices.emplace_back( dummyDevice( "deviceNameC", "deviceCategoryC" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    meshbotTemplate.rules.emplace_back();
    auto& rule = meshbotTemplate.rules.back();
    rule.type = Rule::Type::Item;
    rule.field = ItemName;
    rule.value = "itemNameB";
    rule.operation = Rule::Operation::Equal;

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 1 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 1 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 1 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, queryDevicesAndItemsByComplexQuery ) {  // 1.9
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;
    std::vector< Json > devices;
    std::vector< Json > items;

    devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryCommon" ) );
    devices.emplace_back( dummyDevice( "deviceNameC", "deviceCategoryCommon" ) );
    devices.emplace_back( dummyDevice( "deviceNameD", "deviceCategoryCommon" ) );

    items.emplace_back( dummyItem( devices.at( 0 ), "itemNameA" ) );
    items.emplace_back( dummyItem( devices.at( 1 ), "itemNameB" ) );
    items.emplace_back( dummyItem( devices.at( 2 ), "itemNameC" ) );
    items.emplace_back( dummyItem( devices.at( 3 ), "itemNameD" ) );

    ScenesManager::MeshbotTemplates::Types::MeshbotTemplate meshbotTemplate;
    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Device;
        rule.field = DeviceName;
        rule.value = JSON_ARRAY( "deviceNameB"
                                 << "deviceNameC"
                                 << "deviceNameD" );
        rule.operation = Rule::Operation::In;
    }

    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Device;
        rule.field = DeviceCategory;
        rule.value = "deviceCategoryCommon";
        rule.operation = Rule::Operation::Equal;
    }

    {
        meshbotTemplate.rules.emplace_back();
        auto& rule = meshbotTemplate.rules.back();
        rule.type = Rule::Type::Item;
        rule.field = ItemName;
        rule.value = JSON_ARRAY( "itemNameC"
                                 << "itemNameD" );
        rule.operation = Rule::Operation::In;
    }

    auto triggers = ScenesManager::MeshbotTemplates::Utils::findMeshbotTriggers( devices, items, meshbotTemplate );
    ASSERT_EQ( triggers.size(), 2 );

    EXPECT_EQ( triggers.at( 0 ).device, devices.at( 2 ) );
    EXPECT_EQ( triggers.at( 0 ).item, items.at( 2 ) );

    EXPECT_EQ( triggers.at( 1 ).device, devices.at( 3 ) );
    EXPECT_EQ( triggers.at( 1 ).item, items.at( 3 ) );
}

TEST_F( MeshbotTemplatesUtilsTests, bindingsDbOperations ) {  // 2.1
    using TemplateBinding = ScenesManager::MeshbotTemplates::Types::TemplateBinding;

    TemplateBinding binding_A;
    binding_A.bindingId = "<binding-id-A>";
    binding_A.templateUUID = "<template-uuid-A>";
    binding_A.templateVersion = "<template-version-A>";
    binding_A.sceneId = "<scene-id-A>";
    binding_A.triggerDeviceId = "<trigger-device-id-A>";
    binding_A.triggerItemId = "<trigger-item-id-A>";

    TemplateBinding binding_B;
    binding_B.bindingId = "<binding-id-B>";
    binding_B.templateUUID = "<template-uuid-B>";
    binding_B.templateVersion = "<template-version-B>";
    binding_B.sceneId = "<scene-id-B>";
    binding_B.triggerDeviceId = "<trigger-device-id-B>";
    binding_B.triggerItemId = "<trigger-item-id-B>";

    {  // Read from empty DB
        auto result = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
        ASSERT_EQ( result.size(), 0 );
    }

    // Add binding A
    ScenesManager::MeshbotTemplates::Utils::storeTemplateBinding( dbClient(), binding_A );

    {  // Read single binding
        auto result = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );
    }

    // Add binding B
    ScenesManager::MeshbotTemplates::Utils::storeTemplateBinding( dbClient(), binding_B );

    {  // Read both bindings
        auto result = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
        ASSERT_EQ( result.size(), 2 );
        EXPECT_EQ( result.at( 0 ), binding_A );
        EXPECT_EQ( result.at( 1 ), binding_B );
        EXPECT_NE( result.at( 0 ), result.at( 1 ) );
    }

    // Update binding values
    binding_A.templateUUID = "<template-uuid-A-updated>";
    binding_A.templateVersion = "<template-version-A-updated>";
    binding_A.sceneId = "<scene-id-A-updated>";
    binding_A.triggerDeviceId = "<trigger-device-id-A-updated>";
    binding_A.triggerItemId = "<trigger-item-id-A-updated>";

    ScenesManager::MeshbotTemplates::Utils::storeTemplateBinding( dbClient(), binding_A );

    {  // Read both bindings
        auto result = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
        ASSERT_EQ( result.size(), 2 );
        EXPECT_EQ( result.at( 0 ), binding_A );
        EXPECT_EQ( result.at( 1 ), binding_B );
    }

    // Remove binding A
    ScenesManager::MeshbotTemplates::Utils::deleteTemplateBinding( dbClient(), binding_A.bindingId );

    {  // Read remaining binding
        auto result = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_B );
    }

    // Remove binding B
    ScenesManager::MeshbotTemplates::Utils::deleteTemplateBinding( dbClient(), binding_B.bindingId );

    {  // Read remaining binding
        auto result = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
        ASSERT_EQ( result.size(), 0 );
    }
}

TEST_F( MeshbotTemplatesUtilsTests, queryBindings ) {  // 2.2
    using TemplateBinding = ScenesManager::MeshbotTemplates::Types::TemplateBinding;

    TemplateBinding binding_A;
    binding_A.bindingId = "<binding-id-A>";
    binding_A.templateUUID = "<template-uuid-A>";
    binding_A.templateVersion = "<template-version-A>";
    binding_A.sceneId = "<scene-id-A>";
    binding_A.triggerDeviceId = "<trigger-device-id-A>";
    binding_A.triggerItemId = "<trigger-item-id-A>";

    TemplateBinding binding_B;
    binding_B.bindingId = "<binding-id-B>";
    binding_B.templateUUID = "<template-uuid-B>";
    binding_B.templateVersion = "<template-version-B>";
    binding_B.sceneId = "<scene-id-B>";
    binding_B.triggerDeviceId = "<trigger-device-id-B>";
    binding_B.triggerItemId = "<trigger-item-id-B>";

    ScenesManager::MeshbotTemplates::Utils::storeTemplateBinding( dbClient(), binding_A );
    ScenesManager::MeshbotTemplates::Utils::storeTemplateBinding( dbClient(), binding_B );

    {  // Query by id
        TemplateBinding query;
        query.bindingId = "<binding-id-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );

        query.bindingId = "unknown";
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 0 );
    }

    {  // Query by template uuid
        TemplateBinding query;
        query.templateUUID = "<template-uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );

        query.templateUUID = "unknown";
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 0 );
    }

    {  // Query by template version
        TemplateBinding query;
        query.templateVersion = "<template-version-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );

        query.templateVersion = "unknown";
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 0 );
    }

    {  // Query by scene id
        TemplateBinding query;
        query.sceneId = "<scene-id-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );

        query.sceneId = "unknown";
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 0 );
    }

    {  // Query by device id
        TemplateBinding query;
        query.triggerDeviceId = "<trigger-device-id-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );

        query.triggerDeviceId = "unknown";
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 0 );
    }

    {  // Query by item id
        TemplateBinding query;
        query.triggerItemId = "<trigger-item-id-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ), binding_A );

        query.triggerItemId = "unknown";
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 0 );
    }

    {  // Query all
        TemplateBinding query;
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 2 );
    }
}

}  // namespace UT
}  // namespace HubPlatform
