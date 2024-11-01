#include "plugins_validator_fixture.h"
#include "test_utils/component_tests/common_nma_test_fixture.h"
#include "database_client/dbtools.h"
#include <gtest/gtest.h>

namespace ExtensionsManager {
namespace UT {

using ::testing::AtLeast;
using ::testing::Return;

using namespace HubPlatform::Tests;

class PluginsConfigMetadataValidatorSuite : public PluginsValidatorFixture {};

#define VALIDATE( x )                                         \
    std::istringstream is( x );                               \
    getPluginsMetadataValidator().loadConfigJsonString( is ); \
    JsonObject obj;                                           \
    getPluginsMetadataValidator().validateConfigAndGetItsData( obj );

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigMetaFailed ) {
    const auto meta = createConfigMeta( "xxx",
                                        R"({ "text": "xxx" })",
                                        R"({ "text": "xxx" })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigSucceeded ) {
    const auto configJsonModify = createConfigMeta( "meta",
                                                    R"({ "text": "xxx" })",
                                                    R"({ "text": "xxx" })",
                                                    R"({ "text": "xxx" })",
                                                    R"("pref")",
                                                    "123",
                                                    R"("node")",
                                                    R"("lua")",
                                                    R"("1.0")",
                                                    R"({"static": true, "custom": true})",
                                                    R"("Siren")" );
    try {
        std::istringstream is( configJsonModify );
        PluginsMetadataValidator& validator = getPluginsMetadataValidator();

        validator.loadConfigJsonString( is );
        JsonObject obj;
        validator.validateConfigAndGetItsData( obj );
    }
    catch( const PluginsMetadataValidatorException& e ) {
        FAIL() << e.getPath().toString() + ": " + e.what();
    }
    catch( const std::exception& e ) {
        FAIL() << e.what();
    }
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailName1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": 123 })",
                                        R"({ "text": "xxx" })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailName2 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "t": 123 })",
                                        R"({ "text": "xxx" })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailName3 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "t": 123 })",
                                  R"({ "text": "xxx" })",
                                  R"({ "text": "xxx" })",
                                  R"("pref")",
                                  "123",
                                  R"("node")",
                                  R"("lua")",
                                  R"("1.0")",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"name\": { \"t\": 123 } ,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailDesc1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": 123 })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailDesc2 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "t": "123" })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailDesc3 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "t": 123 })",
                                  R"({ "text": "xxx" })",
                                  R"("pref")",
                                  "123",
                                  R"("node")",
                                  R"("lua")",
                                  R"("1.0")",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"description\": { \"t\": 123 } ,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailAuth1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": 123 })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailAuth2 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "t": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailAuth3 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "text": "123" })",
                                  R"({ "t": 123 })",
                                  R"("pref")",
                                  "123",
                                  R"("node")",
                                  R"("lua")",
                                  R"("1.0")",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"author\": { \"t\": 123 } ,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPref1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "xxx" })",
                                        "123",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPref2 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "text": "123" })",
                                  R"({ "text": 123 })",
                                  "123",
                                  "123",
                                  R"("node")",
                                  R"("lua")",
                                  R"("1.0")",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"prefix\": 123,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailSize ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "xxx" })",
                                        R"("xxx")",
                                        R"("123")",
                                        R"("node")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailType1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": 123 })",
                                        R"("pref")",
                                        "123",
                                        "123",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailType2 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("bool")",
                                        R"("lua")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailType3 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "text": "123" })",
                                  R"({ "t": 123 })",
                                  R"("pref")",
                                  "123",
                                  "123",
                                  R"("lua")",
                                  R"("1.0")",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"type\": 123,\n", "" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailLang1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        "123",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailLang2 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "xxx" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("perl")",
                                        R"("1.0")",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailLang3 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "text": "123" })",
                                  R"({ "type": 123 })",
                                  R"("pref")",
                                  "123",
                                  R"("node")",
                                  "123",
                                  R"("1.0")",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"language\": 123,\n", "\"langu\": 123,\n" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailVer1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        "123",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailVer2 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "text": "123" })",
                                  R"({ "type": 123 })",
                                  R"("pref")",
                                  "123",
                                  R"("node")",
                                  R"("lua")",
                                  "123",
                                  R"({"static": true, "custom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"version\": 123,\n", "\"langu\": 123,\n" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPlc1 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        "123",
                                        R"({"static": "true", "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPlc2 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        "123",
                                        R"({"stac": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPlc3 ) {
    auto meta = createConfigMeta( "meta",
                                  R"({ "text": "123" })",
                                  R"({ "text": "123" })",
                                  R"({ "type": 123 })",
                                  R"("pref")",
                                  "123",
                                  R"("node")",
                                  R"("lua")",
                                  "123",
                                  R"({"static": true, "ctom": true})",
                                  R"("Siren")" );
    meta = CommonNmaTests::replaceTokensInString( meta, { { "\"version\": 123,\n", "\"langu\": 123,\n" } } );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPlc4 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        "123",
                                        R"({"static": true, "custom": true})",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigFailPlc5 ) {
    const auto meta = createConfigMeta( "meta",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"({ "text": "123" })",
                                        R"("pref")",
                                        "123",
                                        R"("node")",
                                        R"("lua")",
                                        "123",
                                        R"(["static", "custom"])",
                                        R"("Siren")" );
    SHOULD_THROW( VALIDATE( meta ), PluginsMetadataValidatorException, "", __FILE__, __LINE__ )
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigParseAndSave ) {
    try {
        std::function< bool( std::string ) > ifExists = []( const std::string& scriptId ) -> bool { return true; };
        std::istringstream pluginDataJsonConfigStream( Resources::pluginDataJsonConfig );
        std::istringstream pluginDataJsonInterfacesStream( Resources::pluginDataJsonInterfaces );
        auto dbData =
            getPluginsMetadataValidator().parse( pluginDataJsonConfigStream, pluginDataJsonInterfacesStream, ifExists );
        getRemoteInterface().saveToDb( dbData );
        EXPECT_EQ( getRemoteInterface().checkId( dbData ), false );
        auto jsons = getRemoteInterface().fetchPluginsMetadataWithQuery( {} );
        EXPECT_EQ( jsons.size(), 1 );
        auto res = checkDbItem( jsons.front(), "Ezlo Innovation", __FILE__, __LINE__ );
        EXPECT_TRUE( res );
    }
    catch( const PluginsMetadataValidatorException& e ) {
        FAIL() << e.getPath().toString() + ": " + e.what();
    }
    catch( const std::exception& e ) {
        FAIL() << e.what();
    }
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigParseAndUpdate ) {
    try {
        std::function< bool( std::string ) > ifExists = []( const std::string& scriptId ) -> bool { return true; };
        std::istringstream pluginDataJsonConfigStream( Resources::pluginDataJsonConfig );
        std::istringstream pluginDataJsonInterfacesStream( Resources::pluginDataJsonInterfaces );
        auto dbData =
            getPluginsMetadataValidator().parse( pluginDataJsonConfigStream, pluginDataJsonInterfacesStream, ifExists );
        dbData[ "metaAuthor" ] = "Ezlo";
        getRemoteInterface().saveToDb( dbData );
        EXPECT_EQ( getRemoteInterface().checkId( dbData ), false );
        auto jsons = getRemoteInterface().fetchPluginsMetadataWithQuery( {} );
        EXPECT_EQ( jsons.size(), 1 );
        auto res = checkDbItem( jsons.front(), "Ezlo", __FILE__, __LINE__ );
        EXPECT_EQ( res, true );
    }
    catch( const PluginsMetadataValidatorException& e ) {
        FAIL() << e.getPath().toString() + ": " + e.what();
    }
    catch( const std::exception& e ) {
        FAIL() << e.what();
    }
}

TEST_F( PluginsConfigMetadataValidatorSuite, testValidateConfigParseAndDelete ) {
    try {
        std::function< bool( std::string ) > ifExists = []( const std::string& scriptId ) -> bool { return true; };
        std::istringstream pluginDataJsonConfigStream( Resources::pluginDataJsonConfig );
        std::istringstream pluginDataJsonInterfacesStream( Resources::pluginDataJsonInterfaces );
        auto dbData =
            getPluginsMetadataValidator().parse( pluginDataJsonConfigStream, pluginDataJsonInterfacesStream, ifExists );
        getRemoteInterface().saveToDb( dbData );
        getRemoteInterface().removePluginData( Resources::PluginPrefixedName );
        auto jsons = getRemoteInterface().fetchPluginsMetadataWithQuery( {} );
        EXPECT_EQ( jsons.size(), 0 );
    }
    catch( const std::exception& e ) {
        FAIL() << e.what();
    }
}

}  // namespace UT
}  // namespace ExtensionsManager
