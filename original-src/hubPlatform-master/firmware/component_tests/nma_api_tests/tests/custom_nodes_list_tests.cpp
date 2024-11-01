#include "extensions_manager/plugin_model/model_resources.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/uid_resources.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include <gtest/gtest.h>
#include <string>

using namespace ExtensionsManager::Resources::ConfigFields;

namespace HubPlatform {
namespace Tests {

class HubCustomNodesFixture : public CoreNmaTest {
public:
    HubCustomNodesFixture();

    void SetUp() override;
    void TearDown() override;

    JsonObject createRequestAndGetAnwer( const std::string ids,
                                         const std::string name,
                                         const std::string auth,
                                         const std::string showMeta,
                                         const std::string showAdvanced,
                                         const std::string showInterface );

protected:
    CoreDRemoteInterface::CoreDInterface& remoteInterface;
};

HubCustomNodesFixture::HubCustomNodesFixture()
: remoteInterface( makeRemote() ) {}

void HubCustomNodesFixture::SetUp() {
    CoreNmaTest::SetUp();
}

void HubCustomNodesFixture::TearDown() {
    CoreNmaTest::TearDown();
}

JsonObject HubCustomNodesFixture::createRequestAndGetAnwer( const std::string ids,
                                                            const std::string name,
                                                            const std::string auth,
                                                            const std::string showMeta,
                                                            const std::string showAdvanced,
                                                            const std::string showInterface ) {
    auto reqest = JsonObject::parse( replaceTokensInString( hubCustomNodesList,
                                                            { { "__IDS__", ids },
                                                              { "__NAME__", name },
                                                              { "__AUTH__", auth },
                                                              { "__SHOW_META__", showMeta },
                                                              { "__SHOW_ADV__", showAdvanced },
                                                              { "__SHOW_INT__", showInterface } } ) );

    return nma().sendtoHubAndGetAnswer( reqest );
}

class HubCustomNodesTests : public HubCustomNodesFixture {};

TEST_F( HubCustomNodesTests, CustomNodesList ) {
    auto showMeta = "true";
    auto showAdvanced = "true";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "[]", R"("name")", R"("author")", showMeta, showAdvanced, showInterface );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    EXPECT_EQ( res[ "error" ], JSON_NULL_VAL );
    EXPECT_EQ( res[ "result" ], JSON( "nodes" << JSON_EMPTY_ARR ) );
}

TEST_F( HubCustomNodesTests, CustomNodesListSuscess ) {
    auto showMeta = "true";
    auto showAdvanced = "true";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "[]", R"("name")", R"("author")", showMeta, showAdvanced, showInterface );
    JsonObject error;
    json_get( res, "error", error );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( error, JSON_NULL_VAL );
}

TEST_F( HubCustomNodesTests, CustomNodesListFailIds ) {
    auto showMeta = "true";
    auto showAdvanced = "true";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "123", R"("name")", R"("author")", showMeta, showAdvanced, showInterface );
    JsonObject error;
    json_get( res, "error", error );

    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( json_get_int( error, "code" ), -32602 );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.ids" );
}

TEST_F( HubCustomNodesTests, CustomNodesListFailName ) {
    auto showMeta = "true";
    auto showAdvanced = "true";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "[]", "123", R"("author")", showMeta, showAdvanced, showInterface );
    JsonObject error;
    json_get( res, "error", error );

    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( json_get_int( error, "code" ), -32602 );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.name" );
}

TEST_F( HubCustomNodesTests, CustomNodesListFailAuthor ) {
    auto showMeta = "true";
    auto showAdvanced = "true";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "[]", R"("name")", "123", showMeta, showAdvanced, showInterface );
    JsonObject error;
    json_get( res, "error", error );

    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( json_get_int( error, "code" ), -32602 );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.author" );
}

TEST_F( HubCustomNodesTests, CustomNodesListFailShowMeta ) {
    auto showMeta = "123";
    auto showAdvanced = "true";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "[]", R"("name")", R"("author")", showMeta, showAdvanced, showInterface );
    JsonObject error;
    json_get( res, "error", error );

    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( json_get_int( error, "code" ), -32602 );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.showMeta" );
}

TEST_F( HubCustomNodesTests, CustomNodesListFailShowAdvanced ) {
    auto showMeta = "true";
    auto showAdvanced = "123";
    auto showInterface = "true";
    auto res = createRequestAndGetAnwer( "[]", R"("name")", R"("author")", showMeta, showAdvanced, showInterface );
    JsonObject error;
    json_get( res, "error", error );

    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( json_get_int( error, "code" ), -32602 );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.showAdvanced" );
}

TEST_F( HubCustomNodesTests, CustomNodesListFailShowInterface ) {
    auto showMeta = "true";
    auto showAdvanced = "true";
    auto showInterface = "123";
    auto res = createRequestAndGetAnwer( "[]", R"("name")", R"("author")", "true", "true", "123" );
    JsonObject error;
    json_get( res, "error", error );

    ASSERT_EQ( json_get_str( res, "method" ), "hub.custom.nodes.list" );
    ASSERT_EQ( json_get_int( error, "code" ), -32602 );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.showInterface" );
}

}  // namespace Tests
}  // namespace HubPlatform
