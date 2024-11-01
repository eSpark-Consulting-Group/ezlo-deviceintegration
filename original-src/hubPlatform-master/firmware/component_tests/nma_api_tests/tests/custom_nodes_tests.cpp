#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "common_test_resources/resources/uid_resources.h"

#include "nma_api_tests/application.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "base_utils/utils.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class CustomNodesTest : public CommonNmaTests {
protected:
    CustomNodesTest();
    static int getWebServerPort();

    void checkGatewayAddedBroadcast( const std::string& nodeName, int codeLine );
    void checkGatewayAddedBroadcast( const JsonObject& broadcast, const std::string& nodeName, int codeLine );
    void checkGatewayUpdatedBroadcast( int codeLine );
    void checkGatewayUpdatedBroadcast( const JsonObject& broadcast, int codeLine );
    void checkIgnoreGatewayUpdatedBroadcast();
    void checkGatewayRemovedBroadcast( int codeLine );
    void checkGatewayRemovedBroadcast( const JsonObject& broadcast, int codeLine );
    void checkNodeStartedBroadcast( const std::string& msg, int codeLine );
    void checkNodeStartedBroadcast( const JsonObject& broadcast, const std::string& msg, int codeLine );
    void checkUpdateProgressBroadcast( int progress, int codeLine );
    void checkUpdateProgressBroadcast( const JsonObject& broadcast, int progress, int codeLine );
    void checkUpdateAborted( int codeLine,
                             const std::chrono::seconds& waitBroadcastTimeout = std::chrono::seconds( 60 ) );
    void checkNodeInstalled( const std::string& nodeName, int codeLine );
    void checkNodeRemoved( const std::string& nodeName, int codeLine );
    void checkNodeList( const std::string& nodeId,
                        const std::string& shortName,
                        const std::string& prefix,
                        bool showMeta,
                        bool showAdvanced,
                        bool showInterfaces,
                        int codeLine );
    static std::string fillSoftwareUpdatePluginTemplate( const std::string& operation,
                                                         const std::string& url,
                                                         const std::string& nodeId,
                                                         const std::string& version,
                                                         const std::string& hash );
    std::string fillSoftwareUpdatePluginUninstallTemplate( const std::string& nodeId );

    static std::string makeWebUrl( const std::string& archiveName );

protected:
    void SetUp() override;
    void TearDown() override;
};

void CustomNodesTest::SetUp() {
    CommonNmaTests::SetUp();
    callingFile( __FILE__ );
}

void CustomNodesTest::TearDown() {
    ignoreBroadcasts(
        []( const Json& broadcast ) {
            Log::error( cc::error( "# TearDown: Not handled broadcasts: " ) + cc::j( broadcast ) );
            return true;
        },
        __FILE__,
        __LINE__,
        std::chrono::milliseconds( 10 ) );
    CommonNmaTests::TearDown();
}

int CustomNodesTest::getWebServerPort() {
    return dynamic_cast< NmaApiTestsApplication& >( nma() ).getCustomPluginsWebServerPort();
}

void CustomNodesTest::checkGatewayAddedBroadcast( const std::string& nodeName, int codeLine ) {
    auto broadcast = waitForBroadcast( "hub.gateway.added", getCodeLineString( codeLine ) );
    checkGatewayAddedBroadcast( broadcast, nodeName, codeLine );
}

void CustomNodesTest::checkGatewayAddedBroadcast( const JsonObject& broadcast,
                                                  const std::string& nodeName,
                                                  int codeLine ) {
    Json result;

    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.gateway.added" ) << getCodeLineString( codeLine );
    ASSERT_EQ( json_get_str( result, "pluginId" ), nodeName ) << getCodeLineString( codeLine );
}

void CustomNodesTest::checkGatewayUpdatedBroadcast( int codeLine ) {
    auto broadcast = waitForBroadcast( "hub.gateway.updated", getCodeLineString( codeLine ) );
    checkGatewayUpdatedBroadcast( broadcast, codeLine );
}

void CustomNodesTest::checkGatewayUpdatedBroadcast( const JsonObject& broadcast, int codeLine ) {
    Json result;
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.gateway.updated" ) << getCodeLineString( codeLine );
}

void CustomNodesTest::checkGatewayRemovedBroadcast( int codeLine ) {
    auto broadcast = waitForBroadcast( "hub.gateway.removed", getCodeLineString( codeLine ) );
    checkGatewayRemovedBroadcast( broadcast, codeLine );
}

void CustomNodesTest::checkGatewayRemovedBroadcast( const JsonObject& broadcast, int codeLine ) {
    Json result;

    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.gateway.removed" ) << getCodeLineString( codeLine );
}

void CustomNodesTest::checkNodeStartedBroadcast( const std::string& msg, int codeLine ) {
    auto broadcast = waitForBroadcast( "hub.extensions.plugin.ui_broadcast", getCodeLineString( codeLine ) );
    checkNodeStartedBroadcast( broadcast, msg, codeLine );
}

void CustomNodesTest::checkNodeStartedBroadcast( const JsonObject& broadcast, const std::string& msg, int codeLine ) {
    Json result;

    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.extensions.plugin.ui_broadcast" )
        << getCodeLineString( codeLine );
    ASSERT_EQ( json_get_str( result, "msg" ), msg ) << getCodeLineString( codeLine );
}

void CustomNodesTest::checkUpdateProgressBroadcast( int progress, int codeLine ) {
    Json broadcast = waitForBroadcast( "hub.software.update.progress",
                                       getCodeLineString( codeLine ) + ", progress: " + std::to_string( progress ) );
    checkUpdateProgressBroadcast( broadcast, progress, codeLine );
}

void CustomNodesTest::checkUpdateProgressBroadcast( const JsonObject& broadcast, int progress, int codeLine ) {
    Json result;

    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.software.update.progress" )
        << getCodeLineString( codeLine );
    ASSERT_EQ( json_get_int( result, "completed" ), progress ) << getCodeLineString( codeLine );
}

void CustomNodesTest::checkIgnoreGatewayUpdatedBroadcast() {
    ignoreBroadcast(
        []( const JsonObject& broadcast ) {
            return json_get_str( broadcast, "msg_subclass" ) == "hub.gateway.updated";
        },
        std::chrono::seconds( 1 ) );
}

void CustomNodesTest::checkUpdateAborted( int codeLine, const std::chrono::seconds& waitBroadcastTimeout ) {
    Json result;
    auto broadcast =
        waitForBroadcast( "hub.software.update.progress", getCodeLineString( __LINE__ ) + ", progress: aborted" );
    EXPECT_TRUE( json_get_obj( broadcast, "result", result ) ) << getCodeLineString( codeLine );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.software.update.progress" )
        << getCodeLineString( codeLine );
    ASSERT_EQ( json_get_str( result, "status" ), "aborted" ) << getCodeLineString( codeLine );
}

void CustomNodesTest::checkNodeInstalled( const std::string& nodeName, int codeLine ) {
    EXPECT_TRUE( filesystem::isDirectoryExist( HubPlatform::Environment::getCustomPluginsFolder() + "/" + nodeName ) )
        << getCodeLineString( codeLine );
}

void CustomNodesTest::checkNodeRemoved( const std::string& nodeName, int codeLine ) {
    EXPECT_FALSE( filesystem::isDirectoryExist( HubPlatform::Environment::getCustomPluginsFolder() + "/" + nodeName ) )
        << getCodeLineString( codeLine );
}

void CustomNodesTest::checkNodeList( const std::string& nodeId,
                                     const std::string& shortName,
                                     const std::string& prefix,
                                     bool showMeta,
                                     bool showAdvanced,
                                     bool showInterface,
                                     int codeLine ) {
    Json request = R"({"method": "hub.custom.nodes.list", "id": "__ID__", "params": {}})"_json;
    request[ "params" ][ "showMeta" ] = showMeta;
    request[ "params" ][ "showAdvanced" ] = showAdvanced;
    request[ "params" ][ "showInterface" ] = showInterface;

    auto answer = nma().sendtoHubAndGetAnswer( request );
    const std::string& errorAt = getCodeLineString( codeLine );

    JsonArray nodes;
    ASSERT_TRUE( json_get( answer, { "result", "nodes" }, nodes ) ) << errorAt;
    ASSERT_TRUE( nodes.is_array() ) << errorAt;
    ASSERT_EQ( nodes.size(), 1 ) << errorAt;

    Json& node = nodes[ 0 ];
    ASSERT_EQ( json_get_str( node, "plugin" ), nodeId ) << errorAt;
    ASSERT_EQ( json_get_str( node, "shortName" ), shortName ) << errorAt;

    if( showMeta ) {
        ASSERT_EQ( node.count( "meta" ), 1 ) << errorAt;
        Json& metadata = node[ "meta" ];
        ASSERT_EQ( json_get_str( metadata, "prefix" ), prefix ) << errorAt;
        ASSERT_EQ( json_get_str( metadata, "type" ), "node" ) << errorAt;
    }
    else {
        ASSERT_EQ( node.count( "meta" ), 0 ) << errorAt;
    }
    if( showAdvanced ) {
        ASSERT_EQ( node.count( "configuration" ), 1 ) << errorAt;
    }
    else {
        ASSERT_EQ( node.count( "configuration" ), 0 ) << errorAt;
    }
    if( showInterface ) {
        ASSERT_EQ( node.count( "interface" ), 1 ) << errorAt;
    }
    else {
        ASSERT_EQ( node.count( "interface" ), 0 ) << errorAt;
    }
}

std::string CustomNodesTest::fillSoftwareUpdatePluginTemplate( const std::string& operation,
                                                               const std::string& url,
                                                               const std::string& nodeId,
                                                               const std::string& version,
                                                               const std::string& hash ) {
    auto updatePlan = replaceTokensInString( hubCustomNodesInstallUpgrade,
                                             { { "__OPERATION__", quotes( operation ) },
                                               { "__SOURCE__", quotes( url ) },
                                               { "__TYPE__", "\"custom_plugin\"" },
                                               { "__ID__", quotes( nodeId ) },
                                               { "__VERSION__", quotes( version ) },
                                               { "__HASH__", quotes( hash ) } } );
    return updatePlan;
}

std::string CustomNodesTest::fillSoftwareUpdatePluginUninstallTemplate( const std::string& nodeId ) {
    auto updatePlan = replaceTokensInString( hubCustomNodesRemove,
                                             { { "__OPERATION__", "\"uninstall\"" },
                                               { "__TYPE__", "\"custom_plugin\"" },
                                               { "__ID__", "\"" + nodeId + "\"" } } );
    return updatePlan;
}

std::string CustomNodesTest::makeWebUrl( const std::string& archiveName ) {
    return "http://localhost:" + std::to_string( getWebServerPort() ) + "/" + archiveName;
}

CustomNodesTest::CustomNodesTest() {}

TEST_F( CustomNodesTest, InstallationUpgradeSuccess ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    const std::string pluginPrefix = "ohtkj94hjk45h-05h5i";
    const std::string pluginShortName = "siren";

    std::string nodeId = pluginPrefix + "_" + pluginShortName;
    if( pluginPrefix.empty() )
        nodeId = pluginShortName;
    const std::string url = makeWebUrl( "data_node.tgz" );
    const std::string archiveHash = "F0A9977AB8AB8474BB06D4E3E97D2066";

    auto installUpgradePlugin = [this]( const std::string& operation,
                                        const std::string& url,
                                        const std::string& nodeId,
                                        const std::string& version,
                                        const std::string& archiveHash,
                                        int line ) {
        ASSERT_TRUE( operation == "install" || operation == "upgrade" ) << ", at: " << __FILE__ << ":" << line;
        std::string updateAction = fillSoftwareUpdatePluginTemplate( operation, url, nodeId, version, archiveHash );
        Json request;
        ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) ) << ", at: " << __FILE__ << ":" << line;

        auto answer = nma().sendtoHubAndGetAnswer( request );
        Json error = answer[ "error" ];
        ASSERT_TRUE( error.empty() ) << error << ", at: " << __FILE__ << ":" << line;

        const char* uiBroadcastMessage = "Wassup cruel world!";
        if( operation == "install" ) {
            checkUpdateProgressBroadcast( 0, line );
            checkUpdateProgressBroadcast( 20, line );
            checkUpdateProgressBroadcast( 80, line );

            checkGatewayAddedBroadcast( nodeId, line );
            checkGatewayUpdatedBroadcast( line );
            checkNodeStartedBroadcast( uiBroadcastMessage, line );
            checkGatewayUpdatedBroadcast( line );
        }
        else if( operation == "upgrade" ) {
            checkUpdateProgressBroadcast( 0, line );
            checkUpdateProgressBroadcast( 20, line );

            checkUpdateProgressBroadcast( 60, line );
            checkGatewayUpdatedBroadcast( line );

            checkUpdateProgressBroadcast( 80, line );

            checkGatewayUpdatedBroadcast( line );
            checkNodeStartedBroadcast( uiBroadcastMessage, line );
            checkGatewayUpdatedBroadcast( line );
        }

        checkUpdateProgressBroadcast( 100, line );

        checkNodeInstalled( nodeId, line );
    };

    installUpgradePlugin( "install", url, nodeId, "1.23", archiveHash, __LINE__ );
    installUpgradePlugin( "upgrade", url, nodeId, "1.24", archiveHash, __LINE__ );

    checkNodeList( nodeId, pluginShortName, pluginPrefix, false, false, false, __LINE__ );
    checkNodeList( nodeId, pluginShortName, pluginPrefix, true, true, true, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationAlreadyInstalledFail ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string url = makeWebUrl( "data_node.tgz" );
    std::string nodeId = "ohtkj94hjk45h-05h5i_siren";
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.23", "F0A9977AB8AB8474BB06D4E3E97D2066" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateAborted( __LINE__ );
}

TEST_F( CustomNodesTest, InstallationInvalidParametersFail ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "ohtkj94hjk45h-05h5i_sirenA";
    std::string url = makeWebUrl( "data_node.tgz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "InvalidOperation", url, nodeId, "1.23", "F0A9977AB8AB8474BB06D4E3E97D2066" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_FALSE( error.empty() ) << error;
    EXPECT_EQ( json_get_int( error, "code" ), -32600 );
    EXPECT_EQ( json_get_str( error, "message" ), "Bad params" );

    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationInvalidParametersFailMetadata ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "eptest1.ezlo_wol";
    std::string url = makeWebUrl( "modified_eptest1.ezlo_wol.tgz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "C71EBD541F0FEA8DDD348EE2B72F9746" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );

    for( int i = 0; i < 100; i++ ) {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.software.update.progress" );
        Json result = broadcast[ "result" ];
        int completed = json_get_int( result, "completed" );
        Json error = result[ "error" ];
        if( completed == 100 ) {
            ASSERT_FALSE( error.empty() ) << error;
            EXPECT_EQ( json_get_int( error, "code" ), -32500 );
            EXPECT_EQ( json_get_str( error, "data" ), "custom.plugin.invalid.metadata" );
            EXPECT_EQ( json_get_str( error, "message" ), "Invalid custom plugin metadata!" );
            EXPECT_EQ( json_get_str( error, "reason" ),
                       R"(/"config.json"/"meta"/"type": 'nod' type is not supported)" );
            break;
        }
        else {
            ASSERT_TRUE( error.empty() );
        }
    }
    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationZeroLengthFileDeploy ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "luiz.minimal";
    std::string url = makeWebUrl( "minimal.tar.gz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "b6a5517b2558e52da38251e41aeab957" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );

    for( int i = 0; i < 100; i++ ) {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.software.update.progress" );
        Json result = broadcast[ "result" ];
        int completed = json_get_int( result, "completed" );
        if( completed == 100 ) {
            Json error = result[ "error" ];
            ASSERT_TRUE( error.empty() ) << error;
            break;
        }
    }
    checkNodeInstalled( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationScriptsSuccess ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "luiz.mqtt";
    std::string url = makeWebUrl( "mqtt_plugin.tar.gz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "3f0fe4baf71c458cb296c7a48b2a6198" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );

    for( int i = 0; i < 100; i++ ) {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.software.update.progress" );
        Json result = broadcast[ "result" ];
        int completed = json_get_int( result, "completed" );
        if( completed == 100 ) {
            Json error = result[ "error" ];
            ASSERT_TRUE( error.empty() );
            break;
        }
    }
    checkNodeInstalled( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationScriptsFailScriptPath ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "luiz.mqtt.fail";
    std::string url = makeWebUrl( "mqtt_plugin_fail_path.tar.gz" );

    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "f63b30be47ca382af811a71bd50e5d2d" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );

    for( int i = 0; i < 100; i++ ) {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.software.update.progress" );
        Json result = broadcast[ "result" ];
        int completed = json_get_int( result, "completed" );
        Json error = result[ "error" ];
        if( completed == 100 ) {
            ASSERT_FALSE( error.empty() ) << error;
            EXPECT_EQ( json_get_int( error, "code" ), -32500 );
            EXPECT_EQ( json_get_str( error, "data" ), "custom.plugin.gateway.invalid" );
            EXPECT_EQ( json_get_str( error, "message" ), "Custom plugin gateway invalid" );
            auto reason = json_get_str( error, "reason" );
            EXPECT_TRUE( !reason.empty() );
            EXPECT_NE( reason.find( "script: HUB:luiz.mqtt.fail/scripts/set_item_value_fail.lua Does not exist" ),
                       std::string::npos );
            break;
        }
        else {
            ASSERT_TRUE( error.empty() );
        }
    }
    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationBadId ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "ohtkj94hjk45h-05h5i_sirenA";
    std::string url = makeWebUrl( "minimal.tar.gz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "b6a5517b2558e52da38251e41aeab957" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );
    auto answer = nma().sendtoHubAndGetAnswer( request );

    for( int i = 0; i < 100; i++ ) {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.software.update.progress" );
        Json result = broadcast[ "result" ];
        int completed = json_get_int( result, "completed" );
        Json error = result[ "error" ];
        if( completed == 100 ) {
            ASSERT_FALSE( error.empty() ) << error;
            EXPECT_EQ( json_get_int( error, "code" ), -32500 );
            EXPECT_EQ( json_get_str( error, "data" ), "custom.plugin.invalid.metadata" );
            EXPECT_EQ( json_get_str( error, "message" ), "Invalid custom plugin metadata!" );
            auto reason = json_get_str( error, "reason" );
            EXPECT_TRUE( reason.size() > 0 );
            EXPECT_TRUE( reason.find( "Plugin config 'id' doesn't match plugin name" ) != std::string::npos );
            break;
        }
        else {
            ASSERT_TRUE( error.empty() );
        }
    }

    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationEmptyPackage ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "ohtkj94hjk45h-05h5i_sirenA";
    std::string url = makeWebUrl( "empty.tgz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "0a6b8e2dba5489fd3c1da80d0f0d6408" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    for( int i = 0; i < 100; i++ ) {
        auto broadcast = NMA_TEST_WAIT_FOR_BROADCAST( "hub.software.update.progress" );
        Json result = broadcast[ "result" ];
        int completed = json_get_int( result, "completed" );
        Json error = result[ "error" ];
        if( completed == 100 ) {
            ASSERT_FALSE( error.empty() ) << error;
            EXPECT_EQ( json_get_int( error, "code" ), -32500 );
            EXPECT_EQ( json_get_str( error, "data" ), "custom.plugin.invalid.metadata" );
            EXPECT_EQ( json_get_str( error, "message" ), "Invalid custom plugin metadata!" );
            auto reason = json_get_str( error, "reason" );
            EXPECT_TRUE( reason.size() > 0 );
            EXPECT_TRUE( reason.find( "Plugin config is bad or not exist: parse error - unexpected '" ) !=
                         std::string::npos );
            break;
        }
        else {
            ASSERT_TRUE( error.empty() );
        }
    }

    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, UpdateBadExecuteRequestParamsTest ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );
    auto sendRequestAndCheckError = [this]( const std::string& actions,
                                            const std::string& message,
                                            const std::string& field,
                                            const std::string& reason,
                                            int codeLine ) {
        auto request =
            JsonObject::parse( replaceTokensInString( hubCustomNodesRequest, { { "__ACTIONS__", actions } } ) );
        auto answer = nma().sendtoHubAndGetAnswer( request );

        Json error = answer[ "error" ];
        ASSERT_FALSE( error.empty() ) << error << ", at: " << getCodeLineString( codeLine );
        EXPECT_EQ( json_get_str( error, "field" ), field ) << getCodeLineString( codeLine );
        EXPECT_EQ( json_get_str( error, "message" ), message ) << getCodeLineString( codeLine );
        EXPECT_EQ( json_get_str( error, "reason" ), reason ) << getCodeLineString( codeLine );
    };

    // "actions" is not an array
    sendRequestAndCheckError(
        R"("actions" : {"operation": "install", "type": "custom_plugin", "id": "id"})",
        "Bad params",
        "actions",
        "field has incorrect format",
        __LINE__ );
    // "actions" array  is missing
    sendRequestAndCheckError( R"("actions_mistake" : [{"operation": "install", "type": "custom_plugin", "id": "id"}])",
                              "Bad params",
                              "actions",
                              "field doesn't exist",
                              __LINE__ );
    // no required "type" field
    sendRequestAndCheckError( R"("actions" : [{"operation": "install", "type": "custom_plugin", "id": "id"}])",
                              "Bad params",
                              "archive_hash_md5",
                              "field doesn't exist",
                              __LINE__ );
    // "archive_hash_md5" invalid format
    sendRequestAndCheckError(
        R"("actions" : [{"operation": "install", "type": "custom_plugin", "id": "id", "archive_hash_md5": 123}])",
        "Bad params",
        "archive_hash_md5",
        "field has incorrect format",
        __LINE__ );
    // no "source" field
    sendRequestAndCheckError(
        R"("actions" : [{"operation": "upgrade", "type": "custom_plugin", "id": "id", "archive_hash_md5": "hash"}])",
        "Bad params",
        "source",
        "field doesn't exist",
        __LINE__ );
    // invalid operations: update
    sendRequestAndCheckError(
        R"("actions" : [{"operation": "update", "type": "custom_plugin", "id": "id", "archive_hash_md5": "hash", "source":
        "source"}])",
        "Bad params",
        "operation",
        "incorrect value: update",
        __LINE__ );
    // no "id" field
    sendRequestAndCheckError(
        R"("actions" : [{"operation": "uninstall", "type": "custom_plugin"}])",
        "Bad params",
        "id",
        "field doesn't exist",
        __LINE__ );
}

TEST_F( CustomNodesTest, DISABLED_CustomNodeInstallationInvalidSourceFail ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "data_node";
    std::string url = makeWebUrl( "data_node.tgz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "F0A9977AB8AB8474BB06D4E3E97D2066" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateProgressBroadcast( 20, __LINE__ );
    checkUpdateAborted( __LINE__ );
    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, InstallationInvalidCheckSumFail ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "data_node_checksum";
    std::string url = makeWebUrl( "data_node.tgz" );

    std::string updateAction = fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "invalidCheckSum" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateProgressBroadcast( 20, __LINE__ );
    checkUpdateAborted( __LINE__ );
    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, DISABLED_CustomNodeInstallationInvalidMetaDataFail ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "ohtkj94hjk45h-05h5i_sirenA";
    std::string url = "http://localhost:" + std::to_string( getWebServerPort() ) + "/invalid_config_data_node.tgz";
    auto request =
        JsonObject::parse( replaceTokensInString( hubCustomNodesInstallUpgrade,
                                                  { { "__OPERATION__", "\"install\"" },
                                                    { "__SOURCE__", "\"" + url + "\"" },
                                                    { "__TYPE__", "\"custom_plugin\"" },
                                                    { "__ID__", "\"" + nodeId + "\"" },
                                                    { "__VERSION__", "\"1.23\"" },
                                                    { "__HASH__", "\"13c2c09a3f18f49c27372d65289eac89\"" } } ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateProgressBroadcast( 20, __LINE__ );
    checkUpdateAborted( __LINE__ );
    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, UninstallSuccess ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    std::string nodeId = "ohtkj94hjk45h-05h5i_siren";
    auto request = JsonObject::parse( replaceTokensInString( hubCustomNodesRemove,
                                                             { { "__OPERATION__", "\"uninstall\"" },
                                                               { "__TYPE__", "\"custom_plugin\"" },
                                                               { "__ID__", "\"" + nodeId + "\"" } } ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateProgressBroadcast( 20, __LINE__ );
    checkGatewayUpdatedBroadcast( __LINE__ );
    checkUpdateProgressBroadcast( 100, __LINE__ );
    checkGatewayRemovedBroadcast( __LINE__ );

    checkNodeRemoved( nodeId, __LINE__ );
}

TEST_F( CustomNodesTest, DISABLED_CustomNodeRemoveInexistentFail ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    auto request = JsonObject::parse( replaceTokensInString( hubCustomNodesRemove,
                                                             { { "__OPERATION__", "\"uninstall\"" },
                                                               { "__TYPE__", "\"custom_plugin\"" },
                                                               { "__ID__", "\"ohtkj94hjk45h-05h5i_siren\"" } } ) );

    auto answer = nma().sendtoHubAndGetAnswer( request );
    Json error = answer[ "error" ];
    ASSERT_TRUE( error.empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateAborted( __LINE__ );
}

TEST_F( CustomNodesTest, checkStartupTeardownScript ) {
    const auto testName = test_info_->name();
    Log::trace( cc::trace( "# test name: " ) + testName );

    // Test startup script:
    std::string nodeId = "startup_teardown";
    std::string url = makeWebUrl( "startup_teardown.tgz" );
    std::string updateAction =
        fillSoftwareUpdatePluginTemplate( "install", url, nodeId, "1.0.0", "2991805bf2b6a0714ff6fc599db20959" );
    Json request;
    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    ASSERT_TRUE( answer[ "error" ].empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateProgressBroadcast( 20, __LINE__ );
    checkUpdateProgressBroadcast( 80, __LINE__ );

    checkGatewayAddedBroadcast( nodeId, __LINE__ );
    checkGatewayUpdatedBroadcast( __LINE__ );
    checkNodeStartedBroadcast( "Startup script execute", __LINE__ );
    checkGatewayUpdatedBroadcast( __LINE__ );
    checkUpdateProgressBroadcast( 100, __LINE__ );
    checkNodeInstalled( nodeId, __LINE__ );

    // Test teardown script:
    updateAction = fillSoftwareUpdatePluginUninstallTemplate( nodeId );

    ASSERT_NO_THROW( request = JsonObject::parse( updateAction ) );

    answer = nma().sendtoHubAndGetAnswer( request );
    ASSERT_TRUE( answer[ "error" ].empty() );

    checkUpdateProgressBroadcast( 0, __LINE__ );
    checkUpdateProgressBroadcast( 20, __LINE__ );
    checkGatewayUpdatedBroadcast( __LINE__ );
    checkNodeStartedBroadcast( "Teardown script execute", __LINE__ );
    checkUpdateProgressBroadcast( 100, __LINE__ );
    checkGatewayRemovedBroadcast( __LINE__ );

    checkNodeRemoved( nodeId, __LINE__ );
}

}  // namespace Tests
}  // namespace HubPlatform
