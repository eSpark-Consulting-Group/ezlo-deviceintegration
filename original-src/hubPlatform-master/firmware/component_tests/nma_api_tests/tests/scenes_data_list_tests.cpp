#include "extensions_manager/plugin_model/model_resources.h"
#include "common_test_resources/fixtures/core_test_fixture.h"
#include "common_test_resources/resources/uid_resources.h"
#include "service_interfaces/tests/fakes/fake_service.h"
#include <gtest/gtest.h>
#include <string>

using namespace ExtensionsManager::Resources::ConfigFields;

namespace HubPlatform {
namespace Tests {
class HubScenesDataListFixture : public CoreNmaTest {
public:
    HubScenesDataListFixture();

    void SetUp() override;
    void TearDown() override;

    JsonObject createRequestAndGetAnswer( const std::string valueTypes,
                                          const std::string scalableValueTypes,
                                          const std::string valueScales,
                                          const std::string scenesValueTypes,
                                          const std::string valueTypeFamilies,
                                          const std::string comparisonOperators,
                                          const std::string comparisonMethods,
                                          const std::string actionMethods,
                                          const std::string advancedScenesVersion );

protected:
    CoreDRemoteInterface::CoreDInterface& remoteInterface;
};

HubScenesDataListFixture::HubScenesDataListFixture()
: remoteInterface( makeRemote( nma() ) ) {}

void HubScenesDataListFixture::SetUp() {
    CoreNmaTest::SetUp();
}

void HubScenesDataListFixture::TearDown() {
    CoreNmaTest::TearDown();
}

JsonObject HubScenesDataListFixture::createRequestAndGetAnswer( const std::string valueTypes,
                                                                const std::string scalableValueTypes,
                                                                const std::string valueScales,
                                                                const std::string scenesValueTypes,
                                                                const std::string valueTypeFamilies,
                                                                const std::string comparisonOperators,
                                                                const std::string comparisonMethods,
                                                                const std::string actionMethods,
                                                                const std::string advancedScenesVersion ) {
    auto request =
        JsonObject::parse( CommonNmaTests::replaceTokensInString( hubScenesDataList,
                                                                  { { "__VAL_TYPES__", valueTypes },
                                                                    { "__SCALABLE_VAL_TYPES__", scalableValueTypes },
                                                                    { "__VAL_SCALES__", valueScales },
                                                                    { "__SCENES_VAL_TYPES__", scenesValueTypes },
                                                                    { "__VAL_TYPE_FAMILIES__", valueTypeFamilies },
                                                                    { "__COMP_OP__", comparisonOperators },
                                                                    { "__COMP__METH__", comparisonMethods },
                                                                    { "__ACTION__METH__", actionMethods },
                                                                    { "__VER__", advancedScenesVersion } } ) );
    return nma().sendtoHubAndGetAnswer( request );
}

class HubScenesDataListTests : public HubScenesDataListFixture {};

TEST_F( HubScenesDataListTests, succcsesRequest ) {
    auto res = createRequestAndGetAnswer( "true", "true", "true", "true", "true", "true", "true", "true", "true" );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    EXPECT_EQ( res[ "error" ], JSON_NULL_VAL );
    EXPECT_TRUE( res[ "result" ] != JSON_EMPTY_OBJ );
}

TEST_F( HubScenesDataListTests, failRequests ) {
    auto res = createRequestAndGetAnswer( "123", "true", "true", "true", "true", "true", "true", "true", "true" );
    JsonObject error;
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.valueTypes" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "123", "true", "true", "true", "true", "true", "true", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.scalableValueTypes" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "123", "true", "true", "true", "true", "true", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.valueScales" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "true", "123", "true", "true", "true", "true", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.scenesValueTypes" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "true", "true", "123", "true", "true", "true", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.valueTypeFamilies" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "true", "true", "true", "123", "true", "true", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.comparisonOperators" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "true", "true", "true", "true", "123", "true", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.comparisonMethods" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "true", "true", "true", "true", "true", "123", "true" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.actionMethods" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );

    res = createRequestAndGetAnswer( "true", "true", "true", "true", "true", "true", "true", "true", "123" );
    ASSERT_TRUE( json_get( res, "error", error ) );
    ASSERT_EQ( json_get_str( res, "method" ), "hub.scenes.block.data.list" );
    ASSERT_EQ( json_get_int( error, "code" ), (int) Errors::Codes::INVALID_PARAMS );
    ASSERT_EQ( json_get_str( error, "data" ), "rpc.params.invalid.advancedScenesVersion" );
    EXPECT_TRUE( res[ "result" ] == JSON_EMPTY_OBJ );
}

}  // namespace Tests
}  // namespace HubPlatform
