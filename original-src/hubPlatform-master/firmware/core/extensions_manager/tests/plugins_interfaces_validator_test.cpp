#include "plugins_validator_fixture.h"
#include "database_client/dbtools.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ExtensionsManager {
namespace UT {

using ::testing::AtLeast;
using ::testing::Return;

using namespace HubPlatform::Tests;

class PluginsInterfacesValidatorSuite : public PluginsValidatorFixture {};

#define VALIDATE( x )                                                                                       \
    std::istringstream is( x );                                                                             \
    getPluginsMetadataValidator().loadInterfacesJsonString( is );                                           \
    std::function< bool( std::string ) > ifExist = [this]( std::string scriptId ) -> bool { return true; }; \
    getPluginsMetadataValidator().validateInterfaces( ifExist );

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesConfigFailed ) {
    const auto meta = createInterfaces( "xxx",
                                        R"("xxx")",
                                        R"("xxx")",
                                        "",
                                        R"("boolean")",
                                        R"("name")",
                                        R"("desc")",
                                        R"("node")",
                                        R"("true")" );

    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesSucceeded ) {
    const auto meta = createInterfaces( "configuration",
                                        R"("xxx")",
                                        R"("xxx")",
                                        R"("float")",
                                        R"("boolean")",
                                        R"("name")",
                                        R"("desc")",
                                        R"("none")",
                                        "true" );
    try {
        std::istringstream is( meta );
        getPluginsMetadataValidator().loadInterfacesJsonString( is );
        std::function< bool( std::string ) > ifExist = []( std::string scriptId ) -> bool { return true; };
        getPluginsMetadataValidator().validateInterfaces( ifExist );
    }
    catch( const std::exception& e ) {
        FAIL() << e.what();
    }
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesScriptFail1 ) {
    const auto meta = createInterfaces( "configuration",
                                        "123",
                                        R"("xxx")",
                                        R"("float")",
                                        R"("boolean")",
                                        R"("name")",
                                        R"("desc")",
                                        R"("node")",
                                        "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, optionalScriptField ) {
    auto meta = createInterfaces( "configuration",
                                  "123",
                                  R"("xxx")",
                                  R"("float")",
                                  R"("boolean")",
                                  R"("name")",
                                  R"("desc")",
                                  R"("none")",
                                  "true" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"script\":123,\n", "" } } );
    SHOULD_NOT_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesFieldFail1 ) {
    const auto meta = createInterfaces( "configuration",
                                        R"("xxx")",
                                        "123",
                                        R"("float")",
                                        R"("boolean")",
                                        R"("name")",
                                        R"("desc")",
                                        R"("node")",
                                        "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesFieldFail2 ) {
    auto meta = createInterfaces( "configuration",
                                  R"("xxx")",
                                  "123",
                                  R"("float")",
                                  R"("boolean")",
                                  R"("name")",
                                  R"("desc")",
                                  R"("node")",
                                  "true" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"field\":123,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesScriptFieldType1 ) {
    const auto meta = createInterfaces(
        "configuration", R"("xxx")", R"("xxx")", R"("float")", "123", R"("name")", R"("desc")", R"("node")", "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesScriptFieldType2 ) {
    auto meta = createInterfaces(
        "configuration", R"("xxx")", R"("xxx")", R"("float")", "123", R"("name")", R"("desc")", R"("node")", "true" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"type\":123\n", R"("1": "2")" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesScriptFieldType3 ) {
    auto meta = createInterfaces( "configuration",
                                  R"("xxx")",
                                  R"("xxx")",
                                  R"("float")",
                                  R"("long long 123")",
                                  R"("name")",
                                  R"("desc")",
                                  R"("node")",
                                  "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesNameFail1 ) {
    const auto meta = createInterfaces( "configuration",
                                        R"("xxx")",
                                        R"("xxx")",
                                        R"("float")",
                                        R"("boolean")",
                                        "123",
                                        R"("desc")",
                                        R"("node")",
                                        "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesNameFail2 ) {
    auto meta = createInterfaces( "configuration",
                                  R"("xxx")",
                                  R"("xxx")",
                                  R"("float")",
                                  R"("boolean")",
                                  "123",
                                  R"("desc")",
                                  R"("node")",
                                  "true" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"name\":123,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesEmptyFormat ) {
    const auto meta = createInterfaces(
        "configuration", R"("xxx")", R"("xxx")", "", R"("boolean")", R"("xxx")", R"("desc")", R"("none")", "true" );
    SHOULD_NOT_THROW(
        VALIDATE( meta ), PluginsMetadataValidatorException, "'format' field is optional", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesFormatFail1 ) {
    const auto meta = createInterfaces( "configuration",
                                        R"("xxx")",
                                        R"("xxx")",
                                        "123",
                                        R"("boolean")",
                                        R"("xxx")",
                                        R"("desc")",
                                        R"("node")",
                                        "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesScriptType1 ) {
    const auto meta = createInterfaces( "configuration",
                                        R"("xxx")",
                                        R"("xxx")",
                                        R"("float")",
                                        R"("boolean")",
                                        R"("name")",
                                        R"("desc")",
                                        "123",
                                        "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesScriptType2 ) {
    auto meta = createInterfaces( "configuration",
                                  R"("xxx")",
                                  R"("xxx")",
                                  R"("float")",
                                  R"("boolean")",
                                  R"("name")",
                                  R"("desc")",
                                  R"("int")",
                                  "true" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, optionalScriptAndInputs ) {
    auto meta = createInterfaces( "configuration",
                                  R"("")",
                                  R"("xxx")",
                                  R"("xxx")",
                                  R"("boolean")",
                                  R"("xxx")",
                                  R"("desc")",
                                  R"("none")",
                                  "true" );
    Json metaObject = Json::parse( meta );
    // remove script and inputs fields, they are optional
    json_remove( metaObject[ "configuration" ], "script" );
    json_remove( metaObject[ "configuration" ], "inputs" );
    SHOULD_NOT_THROW( VALIDATE( metaObject.dump() ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesRequiredFail1 ) {
    const auto meta = createInterfaces( "configuration",
                                        R"("xxx")",
                                        R"("xxx")",
                                        R"("float")",
                                        R"("boolean")",
                                        R"("xxx")",
                                        R"("desc")",
                                        R"("node")",
                                        R"("xxx")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsInterfacesValidatorSuite, testValidateInterfacesRequiredFail2 ) {
    auto meta = createInterfaces( "configuration",
                                  R"("xxx")",
                                  R"("xxx")",
                                  R"("float")",
                                  R"("boolean")",
                                  "123",
                                  R"("desc")",
                                  R"("node")",
                                  "123" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"required\":123,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

}  // namespace UT
}  // namespace ExtensionsManager
