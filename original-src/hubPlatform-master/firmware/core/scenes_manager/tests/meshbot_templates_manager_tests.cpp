#include "scenes_manager/tests/fixtures/meshbot_templates_fixture.h"
#include "scenes_manager/scenes_processor/meshbot_templates/templates_manager/templates_manager.h"
#include "scenes_manager/scenes_processor/meshbot_templates/utils/utils.h"

#include "json_utils/json_utils.h"
#include "database_client/dbqueryparser.h"
#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace HubPlatform {
namespace UT {

/*

TEST PLAN
DONE    1 Handle Sync meshbots
DONE        1.1 Initial update
DONE        1.2 Update templates complex
DONE        1.3 Sync outdated template version
DONE        1.4 Sync removed devices
DONE        1.5 Sync removed items
DONE        1.6 Sync added devices
DONE        1.7 Sync added items
DONE        1.8 Sync inactive triggers
DONE    2 Handle device updated
DONE        2.1 Handle device updated
DONE    3 Handle item updated
DONE        3.1 Handle item updated
DONE    4 Handle migrate meshbots
DONE        4.1 Handle migrate meshbots

*/

namespace {
class FakeHandler : public ScenesManager::MeshbotTemplates::TemplatesManager::HandlerInterface {
public:
    using Version = std::pair< int, int >;
    using MeshbotTemplate = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate;

    std::vector< MeshbotTemplate > getMeshbotTemplates() const override {
        auto version = Version( m_tcdsMeshbotsDataVersion, m_tcdsMeshbotsDataFileVersion );
        auto range = m_meshbotTemplates.equal_range( version );
        std::vector< MeshbotTemplate > meshbots;
        for( auto it = range.first; it != range.second; ++it ) {
            meshbots.emplace_back( it->second );
            ;
        }
        return meshbots;
    }

    std::vector< MeshbotTemplate > getMeshbotTemplates( int meshbotsVersion, int fileVersion ) const override {
        auto version = Version( meshbotsVersion, fileVersion );
        auto range = m_meshbotTemplates.equal_range( version );
        std::vector< MeshbotTemplate > meshbots;
        for( auto it = range.first; it != range.second; ++it ) {
            meshbots.emplace_back( it->second );
            ;
        }
        return meshbots;
    }

    std::vector< Json > getDevices() const override {
        return m_devices;
    }

    std::vector< Json > getItems() const override {
        return m_items;
    }

    int getTcdsGeneralDataVersion() const override {
        return m_tcdsGeneralDataVersion;
    }

    int getTcdsMeshbotsDataVersion() const override {
        return m_tcdsMeshbotsDataVersion;
    }

    void switchGeneralDataVersion( int generalDataVersion, int fileVersion ) override {
        m_tcdsGeneralDataVersion = generalDataVersion;
        m_tcdsGeneralDataFileVersion = fileVersion;
        ++m_switchGeneralDataVersionCalls;
    }

    void switchMeshbotsVersion( int meshbotsVersion, int fileVersion ) override {
        m_tcdsMeshbotsDataVersion = meshbotsVersion;
        m_tcdsMeshbotsDataFileVersion = fileVersion;
        ++m_switchMeshbotsVersionCalls;
    }

    std::string getControllerSerial() const override {
        return m_controllerSerial;
    }

    void createScene( Json scene ) override {
        std::string id = json_get_str( scene, db::ID_FIELD_NAME );
        m_scenes[ id ] = scene;
        ++m_createScenesCalls;
    }

    void deleteScene( const std::string& sceneId ) override {
        m_scenes.erase( sceneId );
        ++m_deleteScenesCalls;
    }

    bool isSceneModified( const std::string& sceneId ) const override {
        auto it = m_scenes.find( sceneId );
        if( it != m_scenes.cend() )
            return json_get_str( it->second.at( "generator" ), "role" ) == "user";
        return false;
    }

    void resetCounters() {
        m_createScenesCalls = 0;
        m_deleteScenesCalls = 0;
        m_switchGeneralDataVersionCalls = 0;
        m_switchMeshbotsVersionCalls = 0;
    }

public:
    std::multimap< Version, ScenesManager::MeshbotTemplates::Types::MeshbotTemplate > m_meshbotTemplates;
    std::vector< Json > m_devices;
    std::vector< Json > m_items;
    int m_tcdsGeneralDataVersion = 1;
    int m_tcdsMeshbotsDataVersion = 1;
    int m_tcdsGeneralDataFileVersion = 1;
    int m_tcdsMeshbotsDataFileVersion = 1;
    std::string m_controllerSerial = "12345678";

    std::map< std::string, Json > m_scenes;
    size_t m_createScenesCalls = 0;
    size_t m_deleteScenesCalls = 0;
    size_t m_switchGeneralDataVersionCalls = 0;
    size_t m_switchMeshbotsVersionCalls = 0;
};
}  // namespace

class MeshbotTemplatesManagerTests : public MeshbotTemplatesFixture {
public:
    using MeshbotTemplate = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate;
    using TemplateBinding = ScenesManager::MeshbotTemplates::Types::TemplateBinding;
    using Rule = ScenesManager::MeshbotTemplates::Types::MeshbotTemplate::Rule;

    std::string getDummyTemplateString( const std::string& sceneName = "" ) {
        return JSON( "name" << sceneName << "generator"
                            << JSON( "role"
                                     << "system" ) )
            .dump();
    }

    Rule createMatchDeviceNameRule( std::string deviceName ) {
        Rule rule;
        rule.type = Rule::Type::Device;
        rule.field = DeviceName;
        rule.value = std::move( deviceName );
        rule.operation = Rule::Operation::Equal;
        return rule;
    }

    Rule createMatchItemNameRule( std::string itemName ) {
        Rule rule;
        rule.type = Rule::Type::Item;
        rule.field = ItemName;
        rule.value = std::move( itemName );
        rule.operation = Rule::Operation::Equal;
        return rule;
    }
};

TEST_F( MeshbotTemplatesManagerTests, updateTemplatesInitial ) {  // 1.1
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    {  // Match all devices/items pairs
        auto it = handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), FakeHandler::MeshbotTemplate{} );
        auto& meshbotTemplate = it->second;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules = {};
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto it = handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), FakeHandler::MeshbotTemplate{} );
        auto& meshbotTemplate = it->second;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    manager.handleSyncMeshbotTemplates();

    EXPECT_EQ( handler.m_scenes.size(), 3 );
    for( auto& element : handler.m_scenes ) {
        EXPECT_FALSE( json_get_str( element.second, db::ID_FIELD_NAME ).empty() );
    }

    auto bindings = ScenesManager::MeshbotTemplates::Utils::readAllTemplateBindings( dbClient() );
    EXPECT_EQ( bindings.size(), 3 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        query.triggerDeviceId = getDeviceId( handler.m_devices.at( 0 ) );
        query.triggerItemId = getItemId( handler.m_items.at( 0 ) );
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 1 );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        query.triggerDeviceId = getDeviceId( handler.m_devices.at( 1 ) );
        query.triggerItemId = getItemId( handler.m_items.at( 1 ) );
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 1 );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        query.triggerDeviceId = getDeviceId( handler.m_devices.at( 1 ) );
        query.triggerItemId = getItemId( handler.m_items.at( 1 ) );
        EXPECT_EQ( ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query ).size(), 1 );
    }

    EXPECT_EQ( handler.m_createScenesCalls, 3 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );
}

TEST_F( MeshbotTemplatesManagerTests, updateTemplatesComplex ) {  // 1.2
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameC", "deviceCategoryC" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameD", "deviceCategoryD" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 2 ), "itemNameC" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 3 ), "itemNameD" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;
    MeshbotTemplate meshbotTemplateC;
    MeshbotTemplate meshbotTemplateD;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameC
        auto& meshbotTemplate = meshbotTemplateC;
        meshbotTemplate.uuid = "<uuid-C>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameC" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameD
        auto& meshbotTemplate = meshbotTemplateD;
        meshbotTemplate.uuid = "<uuid-D>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameD" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateC );

    // Initial update: create 3 scenes for items A, B, C
    manager.handleSyncMeshbotTemplates();

    EXPECT_EQ( handler.m_scenes.size(), 3 );
    EXPECT_EQ( handler.m_createScenesCalls, 3 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // New version:
    // - update meshbot <uuid-A>
    // - mark meshbot <uuid-B> as user modified
    // - delete meshbot <uuid-C>
    // - add new meshbot <uuid-D>

    meshbotTemplateA.version = "<version-2>";
    meshbotTemplateB.version = "<version-2>";
    meshbotTemplateD.version = "<version-2>";

    handler.m_meshbotTemplates.clear();
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 2 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 2 ), meshbotTemplateB );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 2 ), meshbotTemplateD );
    handler.switchMeshbotsVersion( 2, 2 );
    handler.switchGeneralDataVersion( 2, 2 );

    {  // Mark meshbot B as user modified
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        auto& scene = handler.m_scenes.at( result.at( 0 ).sceneId );
        scene.at( "generator" ).at( "role" ) = "user";
    }

    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();

    EXPECT_EQ( handler.m_scenes.size(), 3 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 ) << "1 (update) + 1 (add new meshbot)";
    EXPECT_EQ( handler.m_deleteScenesCalls, 2 ) << "1 (update) + 1 (remove)";

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ).templateVersion, "<version-2>" );
        EXPECT_TRUE( handler.m_scenes.count( result.at( 0 ).sceneId ) );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 ) << "should keep track of user modified meshbots";
        EXPECT_EQ( result.at( 0 ).templateVersion, "<version-1>" );
        EXPECT_TRUE( handler.m_scenes.count( result.at( 0 ).sceneId ) );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-C>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 0 );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-D>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ).templateVersion, "<version-2>" );
        EXPECT_TRUE( handler.m_scenes.count( result.at( 0 ).sceneId ) );
    }
}

TEST_F( MeshbotTemplatesManagerTests, syncOutdatedTemplateVersion ) {  // 1.3
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial update: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Update template B version
    meshbotTemplateB.version = "<version-2>";
    meshbotTemplateB.localMeshbot = getDummyTemplateString( "updated" );

    handler.m_meshbotTemplates.clear();
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 2 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 2 ), meshbotTemplateB );
    handler.switchMeshbotsVersion( 2, 2 );
    handler.switchGeneralDataVersion( 2, 2 );

    // Update outdated template B
    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 ) << "1 (update)";
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 ) << "1 (update)";

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ).templateVersion, "<version-2>" );
        auto& sceneId = result.at( 0 ).sceneId;
        ASSERT_TRUE( handler.m_scenes.count( sceneId ) );
        EXPECT_EQ( json_get_str( handler.m_scenes.at( sceneId ), "name" ), "updated" );
    }
}

TEST_F( MeshbotTemplatesManagerTests, syncRemovedDevices ) {  // 1.4
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial update: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    handler.m_devices.pop_back();

    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }
}

TEST_F( MeshbotTemplatesManagerTests, syncRemovedItems ) {  // 1.5
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial update: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    handler.m_items.pop_back();

    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }
}

TEST_F( MeshbotTemplatesManagerTests, syncAddedDevices ) {  // 1.6
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match deviceNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchDeviceNameRule( "deviceNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial update: create 1 scene for items A
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Create device that matches device name rule
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    // Create scene for device B
    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }
}

TEST_F( MeshbotTemplatesManagerTests, syncAddedItems ) {  // 1.7
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial update: create 1 scene for items A
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Create item that matches item name rule
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    // Create scene for item B
    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }
}

TEST_F( MeshbotTemplatesManagerTests, syncInactiveTriggers ) {  // 1.8
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial update: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Update item name
    handler.m_items.back().at( "name" ) = "new-name";

    // Remove outdated trigger for item B
    handler.resetCounters();
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
    }
}

TEST_F( MeshbotTemplatesManagerTests, handleDeviceUpdated ) {  // 2.1
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match deviceNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchDeviceNameRule( "deviceNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    const auto deviceIdA = ScenesManager::MeshbotTemplates::Utils::getDeviceId( handler.m_devices.at( 0 ) );
    const auto deviceIdB = ScenesManager::MeshbotTemplates::Utils::getDeviceId( handler.m_devices.at( 1 ) );

    // Initial update: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    handler.resetCounters();
    manager.handleDeviceUpdated( "unknown-device-id" );
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Handle device update: device B still matches meshbot rules
    handler.resetCounters();
    manager.handleDeviceUpdated( deviceIdB );
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 ) << "1 (update)";
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 ) << "1 (update)";

    // Handle device update: device B no longer matches meshbot rules
    handler.m_devices.back().at( "name" ) = "new-device-name";  // Break rule matching
    handler.resetCounters();
    manager.handleDeviceUpdated( deviceIdB );
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 ) << "1 (remove outdated)";

    {  // Mark meshbot A as user modified
        TemplateBinding query;
        query.triggerDeviceId = deviceIdA;
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        auto& scene = handler.m_scenes.at( result.at( 0 ).sceneId );
        scene.at( "generator" ).at( "role" ) = "user";
    }

    // Handle device update: meshbot for device A is user modified
    handler.resetCounters();
    manager.handleDeviceUpdated( deviceIdB );
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );
}

TEST_F( MeshbotTemplatesManagerTests, handleItemUpdated ) {  // 3.1
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    const auto itemIdA = ScenesManager::MeshbotTemplates::Utils::getDeviceId( handler.m_items.at( 0 ) );
    const auto itemIdB = ScenesManager::MeshbotTemplates::Utils::getDeviceId( handler.m_items.at( 1 ) );

    // Initial update: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    handler.resetCounters();
    manager.handleItemUpdated( "unknown-item-id" );
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Handle item update: item B still matches meshbot rules
    handler.resetCounters();
    manager.handleItemUpdated( itemIdB );
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 1 ) << "1 (update)";
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 ) << "1 (update)";

    // Handle item update: item B no longer matches meshbot rules
    handler.m_items.back().at( "name" ) = "new-item-name";  // Break rule matching
    handler.resetCounters();
    manager.handleItemUpdated( itemIdB );
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 1 ) << "1 (remove outdated)";

    {  // Mark meshbot A as user modified
        TemplateBinding query;
        query.triggerItemId = itemIdA;
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        auto& scene = handler.m_scenes.at( result.at( 0 ).sceneId );
        scene.at( "generator" ).at( "role" ) = "user";
    }

    // Handle device update: meshbot for device A is user modified
    handler.resetCounters();
    manager.handleItemUpdated( itemIdB );
    EXPECT_EQ( handler.m_scenes.size(), 1 );
    EXPECT_EQ( handler.m_createScenesCalls, 0 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );
}

TEST_F( MeshbotTemplatesManagerTests, migrateMeshbots ) {  // 4.1
    auto handlerObject = std::make_unique< FakeHandler >();
    auto& handler = *handlerObject.get();

    ScenesManager::MeshbotTemplates::TemplatesManager manager( dbClient(), std::move( handlerObject ) );

    handler.m_devices.emplace_back( dummyDevice( "deviceNameA", "deviceCategoryA" ) );
    handler.m_devices.emplace_back( dummyDevice( "deviceNameB", "deviceCategoryB" ) );

    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 0 ), "itemNameA" ) );
    handler.m_items.emplace_back( dummyItem( handler.m_devices.at( 1 ), "itemNameB" ) );

    MeshbotTemplate meshbotTemplateA;
    MeshbotTemplate meshbotTemplateB;

    {  // Match itemNameA
        auto& meshbotTemplate = meshbotTemplateA;
        meshbotTemplate.uuid = "<uuid-A>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameA" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    {  // Match itemNameB
        auto& meshbotTemplate = meshbotTemplateB;
        meshbotTemplate.uuid = "<uuid-B>";
        meshbotTemplate.version = "<version-1>";
        meshbotTemplate.rules.emplace_back( createMatchItemNameRule( "itemNameB" ) );
        meshbotTemplate.localMeshbot = getDummyTemplateString();
    }

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 1, 1 ), meshbotTemplateB );

    // Initial sync: create 2 scenes for items A, B
    manager.handleSyncMeshbotTemplates();
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 );
    EXPECT_EQ( handler.m_deleteScenesCalls, 0 );

    // Update templates to version 2
    meshbotTemplateA.version = "<version-2>";
    meshbotTemplateA.localMeshbot = getDummyTemplateString( "updated" );
    meshbotTemplateB.version = "<version-2>";
    meshbotTemplateB.localMeshbot = getDummyTemplateString( "updated" );

    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 3 ), meshbotTemplateA );
    handler.m_meshbotTemplates.emplace( FakeHandler::Version( 2, 3 ), meshbotTemplateB );

    handler.resetCounters();
    manager.handleMigrateMeshbotTemplates( 22, 33, 2, 3 );

    // Migrate templates
    EXPECT_EQ( handler.m_scenes.size(), 2 );
    EXPECT_EQ( handler.m_createScenesCalls, 2 ) << "2 (update)";
    EXPECT_EQ( handler.m_deleteScenesCalls, 2 ) << "2 (update)";
    EXPECT_EQ( handler.m_switchMeshbotsVersionCalls, 1 ) << "1 (update)";
    EXPECT_EQ( handler.m_switchGeneralDataVersionCalls, 1 ) << "1 (update)";
    EXPECT_EQ( handler.m_tcdsGeneralDataVersion, 22 );
    EXPECT_EQ( handler.m_tcdsGeneralDataFileVersion, 33 );
    EXPECT_EQ( handler.m_tcdsMeshbotsDataVersion, 2 );
    EXPECT_EQ( handler.m_tcdsMeshbotsDataFileVersion, 3 );

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-A>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ).templateVersion, "<version-2>" );
        auto& sceneId = result.at( 0 ).sceneId;
        ASSERT_TRUE( handler.m_scenes.count( sceneId ) );
        EXPECT_EQ( json_get_str( handler.m_scenes.at( sceneId ), "name" ), "updated" );
    }

    {
        TemplateBinding query;
        query.templateUUID = "<uuid-B>";
        auto result = ScenesManager::MeshbotTemplates::Utils::queryBindings( dbClient(), query );
        ASSERT_EQ( result.size(), 1 );
        EXPECT_EQ( result.at( 0 ).templateVersion, "<version-2>" );
        auto& sceneId = result.at( 0 ).sceneId;
        ASSERT_TRUE( handler.m_scenes.count( sceneId ) );
        EXPECT_EQ( json_get_str( handler.m_scenes.at( sceneId ), "name" ), "updated" );
    }
}

}  // namespace UT
}  // namespace HubPlatform
