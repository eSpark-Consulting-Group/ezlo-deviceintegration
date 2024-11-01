#include "common_test_resources/resources/uid_resources.h"
#include "common_test_resources/fixtures/scene_manager_tests_fixture.h"
#include "common_test_resources/resources/scene_manager_resources.h"

#include "cored/model/cored_item.h"
#include "cored/model/values/value_factory.h"

#include <gtest/gtest.h>

namespace HubPlatform {
namespace Tests {

class DeviceSettingsListTests : virtual public SceneManagerTestsFixture {
public:
    void SetUp() override;
    void TearDown() override;

    void createDeviceSettings();

    static void checkNoError( const JsonObject& answer, const std::string& message );

    static void checkResultSize( const Json& answer, int expectedFullResults, const std::string& message );

    static void checkError( const Json& answer, int errorCode, const std::string& data, const std::string& message );

    static std::string makeCommand( std::vector< std::string > ids );

protected:
    Json m_device;
    std::string m_deviceId;
    using SettingIds = std::vector< std::string >;
    SettingIds m_settingIds;
};

void DeviceSettingsListTests::SetUp() {
    CommonNmaTests::SetUp();

    m_deviceId =
        addDevice( makeRemote(), "testDevice1", "smoke", "134_3_24", "security_sensor", "smoke", false, __LINE__ );

    createDeviceSettings();
}

void DeviceSettingsListTests::TearDown() {
    clearExistingDevices( __LINE__ );
    SceneManagerTestsFixture::TearDown();
}

void DeviceSettingsListTests::createDeviceSettings() {
    auto& remote = makeRemote( nma() );

    std::string id;
    id = addDeviceSetting( remote, m_deviceId, CoreD::Model::ValueType::Enum::Int, __LINE__ );
    m_settingIds.push_back( id );
    id = addDeviceSetting( remote, m_deviceId, CoreD::Model::ValueType::Enum::Bool, __LINE__ );
    m_settingIds.push_back( id );
    id = addDeviceSetting( remote, m_deviceId, CoreD::Model::ValueType::Enum::Temperature, __LINE__ );
    m_settingIds.push_back( id );
}

void DeviceSettingsListTests::checkNoError( const JsonObject& answer, const std::string& message ) {
    Json error;
    EXPECT_FALSE( answer.empty() ) << message;
    EXPECT_TRUE( json_get( answer, "error", error ) ) << message;
    EXPECT_TRUE( error.is_null() ) << message;
}

void DeviceSettingsListTests::checkResultSize( const Json& answer,
                                               int expectedFullResults,
                                               const std::string& message ) {
    Json settings;
    EXPECT_TRUE( json_get( answer, { "result", "settings" }, settings ) ) << message;
    EXPECT_TRUE( settings.is_array() ) << message;
    EXPECT_EQ( settings.size(), expectedFullResults ) << message;
}

std::string DeviceSettingsListTests::makeCommand( std::vector< std::string > ids ) {
    std::string idsStr = R"("ids": [)";
    for( int i = 0; i < ids.size(); ++i ) {
        const auto& id = ids[ i ];
        if( i > 0 )
            idsStr += ',';
        idsStr += "\"" + id + "\"";
    }
    idsStr += "]";

    return idsStr;
}

void DeviceSettingsListTests::checkError( const Json& answer,
                                          int errorCode,
                                          const std::string& data,
                                          const std::string& message ) {
    Json error;
    LOGGER_TRACE( cc::j( answer ) );
    EXPECT_FALSE( answer.empty() ) << message;
    EXPECT_TRUE( json_get( answer, "error", error ) ) << message;
    EXPECT_TRUE( error.is_object() ) << message;

    EXPECT_EQ( error[ "code" ], errorCode ) << message;
    if( !data.empty() )
        EXPECT_EQ( error[ "data" ], data ) << message;
}

namespace Resources {
std::string hubDeviceSettingsList = R"({
        "method": "hub.device.settings.list",
        "id": "_ID_",
        "params": {
            __OPTIONAL_IDS__
        }
    })";
const char optionalIdsPattern[] = "__OPTIONAL_IDS__";
}  // namespace Resources

TEST_F( DeviceSettingsListTests, getSettingsList ) {
    Json request = replaceTokens( Resources::hubDeviceSettingsList, { { Resources::optionalIdsPattern, "" } } );
    auto answer = nma().sendtoHubAndGetAnswer( request );
    checkNoError( answer, NMA_TEST_MAKE_MESSAGE( "get whole list" ) );

    checkResultSize( answer, m_settingIds.size(), NMA_TEST_MAKE_MESSAGE( "get whole list" ) );
}

TEST_F( DeviceSettingsListTests, getSettingsListById ) {
    for( int i = 1; i < m_settingIds.size(); i++ ) {
        SettingIds slice( m_settingIds.begin(), m_settingIds.begin() + i );
        auto idsStr = makeCommand( slice );

        Json request = replaceTokens( Resources::hubDeviceSettingsList, { { Resources::optionalIdsPattern, idsStr } } );
        LOGGER_TRACE( cc::j( request ) );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkNoError( answer, NMA_TEST_MAKE_MESSAGE( "get slice: index: " + std::to_string( i ) ) );

        checkResultSize( answer, slice.size(), NMA_TEST_MAKE_MESSAGE( "get slice: index: " + std::to_string( i ) ) );
    }
}

TEST_F( DeviceSettingsListTests, getSettingsListErrors ) {
    {
        Json request =
            replaceTokens( Resources::hubDeviceSettingsList, { { Resources::optionalIdsPattern, R"("ids": [])" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32600, "rpc.params.empty.ids", NMA_TEST_MAKE_MESSAGE( "empty ids array" ) );
    }
    {
        Json request = replaceTokens( Resources::hubDeviceSettingsList,
                                      { { Resources::optionalIdsPattern, R"("ids": "some string")" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32602, "rpc.params.invalid.ids", NMA_TEST_MAKE_MESSAGE( "not an array" ) );
    }
    {
        Json request = replaceTokens( Resources::hubDeviceSettingsList,
                                      { { Resources::optionalIdsPattern, R"("ids": ["a", 11, {"a":true}])" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32602, "rpc.params.invalid.ids", NMA_TEST_MAKE_MESSAGE( "array with not a string" ) );
    }
    {
        Json request = replaceTokens( Resources::hubDeviceSettingsList,
                                      { { Resources::optionalIdsPattern, R"("ids": [ "" ])" } } );
        auto answer = nma().sendtoHubAndGetAnswer( request );
        checkError( answer, -32602, "rpc.params.invalid.ids", NMA_TEST_MAKE_MESSAGE( "array with empty string" ) );
    }
}

}  // namespace Tests
}  // namespace HubPlatform