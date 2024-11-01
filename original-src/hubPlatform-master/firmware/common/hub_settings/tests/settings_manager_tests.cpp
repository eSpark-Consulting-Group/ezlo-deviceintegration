#include "hub_settings/hub_settings_manager.h"

#include "common_utils/cpp14.h"
#include "config_reader/config_keys.h"
#include "hub_settings/resources.h"
#include <database_client/dbqueryparser.h>

#include "config_reader/tests/config_reader_factory.h"
#include "database_client/fake_db/fake_db_client.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace UT {

namespace {
using namespace HubPlatform::Hub::Resources;

JsonObject createSettings( const JsonArray& settingsList ) {
    return JSON( "settings" << settingsList );
}

const JsonObject TOKEN_SETTING = JSON( "name"
                                       << "token_name"
                                       << "valueType"
                                       << "token"
                                       << "enum"
                                       << JSON_ARRAY( "token_value"
                                                      << "another_token_value" )
                                       << "value"
                                       << "token_value" );

const JsonObject BOOL_SETTING = JSON( "name"
                                      << "bool_name"
                                      << "valueType"
                                      << "bool"
                                      << "value" << true );

const JsonObject STRING_SETTING = JSON( "name"
                                        << "string_name"
                                        << "valueType"
                                        << "string"
                                        << "value"
                                        << "string_value" );

const JsonObject UNKNOWN_SETTING = JSON( "name"
                                         << "unknown_name"
                                         << "valueType"
                                         << "unknown"
                                         << "value"
                                         << "unknown_value" );

}  // namespace

class SettingsManagerTest : public ::testing::Test, public ConfigReaderFactory {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    ConfigReader createConfigReaderForSettings( const JsonObject& settings );
    IDbClient& getDBClient();
    JsonObject getDBSetting( const std::string& recordName );
    size_t getDBSettingsNum();

private:
    void cleanDB();

private:
    std::unique_ptr< HubPlatform::UT::FakeDbClient > m_dbClient;
};

void SettingsManagerTest::SetUp() {
    m_dbClient = std::make_unique< HubPlatform::UT::FakeDbClient >();
    cleanDB();
}

void SettingsManagerTest::TearDown() {
    cleanDB();
}

ConfigReader SettingsManagerTest::createConfigReaderForSettings( const JsonObject& settings ) {
    return createConfigReader( [&settings]( const std::string& ) { return settings.dump(); } );
}

IDbClient& SettingsManagerTest::getDBClient() {
    return *m_dbClient;
}

JsonObject SettingsManagerTest::getDBSetting( const std::string& recordName ) {
    return getDBClient().findOne( DBNames::SettingsDB, JSON( db::ID_FIELD_NAME << recordName ), nullptr );
}

size_t SettingsManagerTest::getDBSettingsNum() {
    return getDBClient().count( DBNames::SettingsDB, JSON_EMPTY_OBJ );
}

void SettingsManagerTest::cleanDB() {
    getDBClient().clean();
}

TEST_F( SettingsManagerTest, FixtureTest ) {
    auto settings = createSettings( { TOKEN_SETTING, BOOL_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );

    ConfigReader::Path path( configReader, ConfigKeys::Settings );

    EXPECT_EQ( ConfigReader::getArraySize( path ), 2 );

    EXPECT_EQ( ConfigReader::getString( path / size_t( 0 ) / "name" ), TOKEN_SETTING[ "name" ] );
    EXPECT_EQ( ConfigReader::getString( path / size_t( 0 ) / "valueType" ), TOKEN_SETTING[ "valueType" ] );
    EXPECT_EQ( ConfigReader::getString( path / size_t( 0 ) / "value" ), TOKEN_SETTING[ "value" ] );
    EXPECT_EQ( ConfigReader::getStringArray( path / size_t( 0 ) / "enum" ), TOKEN_SETTING[ "enum" ] );

    EXPECT_EQ( ConfigReader::getString( path / 1 / "name" ), BOOL_SETTING[ "name" ] );
    EXPECT_EQ( ConfigReader::getString( path / 1 / "valueType" ), BOOL_SETTING[ "valueType" ] );
    EXPECT_EQ( ConfigReader::getBool( path / 1 / "value" ), BOOL_SETTING[ "value" ] );

    EXPECT_EQ( getDBSettingsNum(), 0 );

    const auto setting = JSON( db::ID_FIELD_NAME << "setting_id"
                                                 << "value"
                                                 << "setting_value" );
    getDBClient().insert( DBNames::SettingsDB, setting );
    EXPECT_EQ( getDBSettingsNum(), 1 );

    auto dbSetting = getDBSetting( "setting_id" );
    EXPECT_EQ( dbSetting, setting );
    EXPECT_EQ( getDBSettingsNum(), 1 );
}

TEST_F( SettingsManagerTest, AddSettingsToEmptyDBTest ) {
    auto settings = createSettings( { TOKEN_SETTING, BOOL_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );

    EXPECT_EQ( getDBSettingsNum(), 0 );

    Hub::SettingsManager manager( getDBClient(), configReader );
    EXPECT_EQ( getDBSettingsNum(), 2 );

    auto dbToken = getDBSetting( "token_name" );
    EXPECT_EQ( dbToken[ db::ID_FIELD_NAME ], TOKEN_SETTING[ "name" ] );
    EXPECT_EQ( dbToken[ "valueType" ], TOKEN_SETTING[ "valueType" ] );
    EXPECT_EQ( dbToken[ "value" ], TOKEN_SETTING[ "value" ] );
    EXPECT_EQ( dbToken[ "enum" ], TOKEN_SETTING[ "enum" ] );

    auto dbBool = getDBSetting( "bool_name" );
    EXPECT_EQ( dbBool[ db::ID_FIELD_NAME ], BOOL_SETTING[ "name" ] );
    EXPECT_EQ( dbBool[ "valueType" ], BOOL_SETTING[ "valueType" ] );
    EXPECT_EQ( dbBool[ "value" ], BOOL_SETTING[ "value" ] );
}

TEST_F( SettingsManagerTest, IgnoreExistingSettingsInDBTest ) {
    {
        auto settings = createSettings( { TOKEN_SETTING, BOOL_SETTING } );
        const auto configReader = createConfigReaderForSettings( settings );
        Hub::SettingsManager manager( getDBClient(), configReader );
    }

    EXPECT_EQ( getDBSettingsNum(), 2 );

    {
        auto updatedTokenSetting = TOKEN_SETTING;
        updatedTokenSetting[ "value" ] = "another_token_value";

        auto updatedBoolSetting = BOOL_SETTING;
        updatedBoolSetting[ "value" ] = false;

        auto updatedSettings = createSettings( { updatedBoolSetting, updatedTokenSetting } );

        const auto configReader = createConfigReaderForSettings( updatedSettings );

        Hub::SettingsManager manager( getDBClient(), configReader );
        EXPECT_EQ( getDBSettingsNum(), 2 );

        auto dbToken = getDBSetting( "token_name" );
        EXPECT_EQ( dbToken[ "value" ], TOKEN_SETTING[ "value" ] );

        auto dbBool = getDBSetting( "bool_name" );
        EXPECT_EQ( dbBool[ "value" ], BOOL_SETTING[ "value" ] );
    }
}

TEST_F( SettingsManagerTest, AddNewSettingsToDBTest ) {
    {
        auto settings = createSettings( { TOKEN_SETTING } );
        const auto configReader = createConfigReaderForSettings( settings );
        Hub::SettingsManager manager( getDBClient(), configReader );
    }

    EXPECT_EQ( getDBSettingsNum(), 1 );

    {
        const auto& NEW_SETTING = BOOL_SETTING;
        auto settings = createSettings( { TOKEN_SETTING, NEW_SETTING } );
        const auto configReader = createConfigReaderForSettings( settings );

        Hub::SettingsManager manager( getDBClient(), configReader );
        EXPECT_EQ( getDBSettingsNum(), 2 );

        auto dbToken = getDBSetting( "token_name" );
        EXPECT_EQ( dbToken[ "value" ], TOKEN_SETTING[ "value" ] );

        auto dbBool = getDBSetting( "bool_name" );
        EXPECT_EQ( dbBool[ "value" ], BOOL_SETTING[ "value" ] );
    }
}

TEST_F( SettingsManagerTest, SettingTypeHasChangedTest ) {
    auto originalSetting = STRING_SETTING;
    originalSetting[ "name" ] = "setting_name";

    auto newSetting = TOKEN_SETTING;
    newSetting[ "name" ] = "setting_name";

    {
        auto settings = createSettings( { originalSetting } );
        const auto configReader = createConfigReaderForSettings( settings );
        Hub::SettingsManager manager( getDBClient(), configReader );
    }

    EXPECT_EQ( getDBSettingsNum(), 1 );

    {
        auto settings = createSettings( { newSetting } );
        const auto configReader = createConfigReaderForSettings( settings );

        Hub::SettingsManager manager( getDBClient(), configReader );
        EXPECT_EQ( getDBSettingsNum(), 1 );

        auto dbToken = getDBSetting( "setting_name" );
        EXPECT_EQ( dbToken[ "value" ], newSetting[ "value" ] );
        EXPECT_EQ( dbToken[ "valueType" ], newSetting[ "valueType" ] );
    }
}

TEST_F( SettingsManagerTest, TokenRangeHasExtendedTest ) {
    const std::string settingName = "setting_name";

    auto originalSetting = TOKEN_SETTING;
    originalSetting[ "name" ] = settingName;
    originalSetting[ "value" ] = "token_value";
    originalSetting[ "enum" ] = JSON_ARRAY( "token_value"
                                            << "second_token_value" );

    auto newSetting = originalSetting;
    newSetting[ "enum" ] = JSON_ARRAY( "token_value"
                                       << "second_token_value"
                                       << "third_token_value" );
    newSetting[ "value" ] = "second_token_value";

    {
        auto settings = createSettings( { originalSetting } );
        const auto configReader = createConfigReaderForSettings( settings );
        Hub::SettingsManager manager( getDBClient(), configReader );
    }

    EXPECT_EQ( getDBSettingsNum(), 1 );

    {
        auto settings = createSettings( { newSetting } );
        const auto configReader = createConfigReaderForSettings( settings );

        Hub::SettingsManager manager( getDBClient(), configReader );
        EXPECT_EQ( getDBSettingsNum(), 1 );

        auto dbToken = getDBSetting( settingName );
        EXPECT_EQ( dbToken[ "enum" ], newSetting[ "enum" ] );
        EXPECT_EQ( dbToken[ "value" ], originalSetting[ "value" ] ) << "Original value should kept";
    }
}

TEST_F( SettingsManagerTest, NewTokenRangeInvalidateOriginalValueTest ) {
    const std::string settingName = "setting_name";
    const std::string defaultTokenValue = "default_token_value";

    auto originalSetting = TOKEN_SETTING;
    originalSetting[ "name" ] = settingName;
    originalSetting[ "value" ] = "token_value";
    originalSetting[ "enum" ] = JSON_ARRAY( "token_value"
                                            << "second_token_value" );

    auto newSetting = originalSetting;
    newSetting[ "enum" ] = JSON_ARRAY( "new_token_value" << defaultTokenValue );
    newSetting[ "value" ] = defaultTokenValue;

    {
        auto settings = createSettings( { originalSetting } );
        const auto configReader = createConfigReaderForSettings( settings );
        Hub::SettingsManager manager( getDBClient(), configReader );
    }

    EXPECT_EQ( getDBSettingsNum(), 1 );

    {
        auto settings = createSettings( { newSetting } );
        const auto configReader = createConfigReaderForSettings( settings );

        Hub::SettingsManager manager( getDBClient(), configReader );
        EXPECT_EQ( getDBSettingsNum(), 1 );

        auto dbToken = getDBSetting( settingName );
        EXPECT_EQ( dbToken[ "enum" ], newSetting[ "enum" ] );
        EXPECT_EQ( dbToken[ "value" ], defaultTokenValue );
    }
}

TEST_F( SettingsManagerTest, SettingsWithDublicatesTest ) {
    auto settings = createSettings( { TOKEN_SETTING, TOKEN_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );

    Hub::SettingsManager manager( getDBClient(), configReader );
    EXPECT_EQ( getDBSettingsNum(), 1 );
}

TEST_F( SettingsManagerTest, IgnoreInvalidSettingsTest ) {
    auto invalidSetting = TOKEN_SETTING;
    invalidSetting[ "value" ] = "invalid_token";

    auto settings = createSettings( { BOOL_SETTING, invalidSetting } );
    const auto configReader = createConfigReaderForSettings( settings );

    Hub::SettingsManager manager( getDBClient(), configReader );
    EXPECT_EQ( getDBSettingsNum(), 1 );

    auto dbBool = getDBSetting( "bool_name" );
    EXPECT_EQ( dbBool[ "value" ], BOOL_SETTING[ "value" ] );
}

TEST_F( SettingsManagerTest, AcceptKnownSettingsTest ) {
    auto settings = createSettings( { TOKEN_SETTING, BOOL_SETTING, UNKNOWN_SETTING, STRING_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );

    Hub::SettingsManager manager( getDBClient(), configReader );
    EXPECT_EQ( getDBSettingsNum(), 3 );

    auto dbToken = getDBSetting( "token_name" );
    EXPECT_EQ( dbToken[ "value" ], TOKEN_SETTING[ "value" ] );

    auto dbBool = getDBSetting( "bool_name" );
    EXPECT_EQ( dbBool[ "value" ], BOOL_SETTING[ "value" ] );

    auto dbString = getDBSetting( "string_name" );
    EXPECT_EQ( dbString[ "value" ], STRING_SETTING[ "value" ] );
}

TEST_F( SettingsManagerTest, setSettingValueTest ) {
    auto settings = createSettings( { TOKEN_SETTING, BOOL_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );
    Hub::SettingsManager manager( getDBClient(), configReader );

    auto status = manager.setSettingValue( "token_name", "another_token_value" );
    EXPECT_TRUE( status );

    auto dbToken = getDBSetting( "token_name" );
    EXPECT_EQ( dbToken[ db::ID_FIELD_NAME ], TOKEN_SETTING[ "name" ] );
    EXPECT_EQ( dbToken[ "valueType" ], TOKEN_SETTING[ "valueType" ] );
    EXPECT_EQ( dbToken[ "enum" ], TOKEN_SETTING[ "enum" ] );
    EXPECT_EQ( dbToken[ "value" ], "another_token_value" );

    auto dbBool = getDBSetting( "bool_name" );
    EXPECT_EQ( dbBool[ db::ID_FIELD_NAME ], BOOL_SETTING[ "name" ] );
    EXPECT_EQ( dbBool[ "valueType" ], BOOL_SETTING[ "valueType" ] );
    EXPECT_EQ( dbBool[ "value" ], BOOL_SETTING[ "value" ] );
}

TEST_F( SettingsManagerTest, setSameSettingValueTest ) {
    auto settings = createSettings( { TOKEN_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );
    Hub::SettingsManager manager( getDBClient(), configReader );

    auto status = manager.setSettingValue( "token_name", TOKEN_SETTING[ "value" ] );
    EXPECT_FALSE( status );
    EXPECT_EQ( getDBSettingsNum(), 1 );
}

TEST_F( SettingsManagerTest, setSettingWithNonexistentNameTest ) {
    auto settings = createSettings( { TOKEN_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );
    Hub::SettingsManager manager( getDBClient(), configReader );

    auto status = manager.setSettingValue( "nonexistent_name", "another_token_value" );
    EXPECT_FALSE( status );
    EXPECT_EQ( getDBSettingsNum(), 1 );

    auto dbToken = getDBSetting( "token_name" );
    EXPECT_EQ( dbToken[ "value" ], TOKEN_SETTING[ "value" ] );

    auto dbNewToken = getDBSetting( "nonexistent_name" );
    EXPECT_TRUE( dbNewToken.empty() );
}

TEST_F( SettingsManagerTest, setSettingWithInvalidValueTest ) {
    auto settings = createSettings( { TOKEN_SETTING } );
    const auto configReader = createConfigReaderForSettings( settings );
    Hub::SettingsManager manager( getDBClient(), configReader );

    auto status = manager.setSettingValue( "token_name", "invalid_token_value" );
    EXPECT_FALSE( status );
    EXPECT_EQ( getDBSettingsNum(), 1 );

    auto dbToken = getDBSetting( "token_name" );
    EXPECT_EQ( dbToken[ "value" ], TOKEN_SETTING[ "value" ] );
}

}  // namespace UT
}  // namespace HubPlatform
