#include "common_test_resources/fixtures/core_test_fixture.h"
#include "cored/model/resources/cored_resources.h"
#include "service_interfaces/resources.h"
#include <utility>

namespace HubPlatform {
namespace Tests {

/*

TESTPLAN:

DONE    1 hub.log.local.set command tests
DONE        1.1 hub.log.local.set level, color, indent changes
DONE        1.2 error checks
TODO    2 Tests for etc/firmware.cfg/log_level value change

*/

class HubLogLocalSetTests : public CoreNmaTest {
public:
    void SetUp() override;
    void TearDown() override;

    void resetToDefaults();
    static Json callHubLogLocalSet( Json params );

    static void changeLogSettings( const Json& params, const std::string& message );
    static void checkNoError( const Json& answer, const std::string& message );
    static void checkError( const Json& answer, const Json& expectedError, const std::string& message );
    void checkSettingUpdated( const Json& params, const std::string& message );

    static std::unique_ptr< Hub::Setting > getSetting( const std::string& name, const char* file, int line );

    static std::string getLogsLevelValue( const char* file, int line );
    static bool getLogsColorValue( const char* file, int line );
    static int getLogsIndentValue( const char* file, int line );
};

using namespace HubPlatform::Services::Resources;

void HubLogLocalSetTests::SetUp() {
    CoreNmaTest::SetUp();
    resetToDefaults();
}

void HubLogLocalSetTests::TearDown() {
    resetToDefaults();
    CoreNmaTest::TearDown();
}

void HubLogLocalSetTests::resetToDefaults() {
    auto logLevelUPtr = makeRemote().findSetting( HubSettings::LogsLevel );
    if( logLevelUPtr && logLevelUPtr->getTokenValue() != "TRACE" ) {
        changeLogSettings( { { "severity", "TRACE" }, { "color", false }, { "indent", -1 } },
                           NMA_TEST_MAKE_MESSAGE( "" ) );
        ignoreBroadcasts(
            []( const Json& broadcast ) { return json_get_str( broadcast, "msg_subclass" ) == "hub.setting.updated"; },
            __FILE__,
            __LINE__,
            std::chrono::milliseconds( 300 ) );
    }
}

Json HubLogLocalSetTests::callHubLogLocalSet( Json params ) {
    auto answer =
        nma().sendtoHubAndGetAnswer( { { "method", "hub.log.local.set" }, { "params", std::move( params ) } } );
    return answer;
}

void HubLogLocalSetTests::changeLogSettings( const Json& params, const std::string& message ) {
    auto answer = callHubLogLocalSet( params );
    checkNoError( answer, message );
}

void HubLogLocalSetTests::checkNoError( const Json& answer, const std::string& message ) {
    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    EXPECT_TRUE( answer[ "error" ].empty() ) << message << ", error: " << answer[ "error" ].dump();
}

void HubLogLocalSetTests::checkError( const Json& answer, const Json& expectedError, const std::string& message ) {
    EXPECT_TRUE( answer[ "result" ].empty() ) << message;
    Json error;
    EXPECT_TRUE( json_get_obj( answer, "error", error ) );
    EXPECT_FALSE( error.empty() ) << message << ", error: " << error.dump();

    if( !expectedError.empty() ) {
        EXPECT_EQ( json_get_int( expectedError, "code", -1 ), json_get_int( error, "code", -1 ) );
        if( !json_get_str( expectedError, "data" ).empty() )
            EXPECT_EQ( json_get_str( expectedError, "data" ), json_get_str( error, "data" ) );
        if( !json_get_str( expectedError, "reason" ).empty() )
            EXPECT_EQ( json_get_str( expectedError, "reason" ), json_get_str( error, "reason" ) );
        if( !json_get_str( expectedError, "message" ).empty() )
            EXPECT_EQ( json_get_str( expectedError, "message" ), json_get_str( error, "message" ) );
    }
}

void HubLogLocalSetTests::checkSettingUpdated( const Json& params, const std::string& message ) {
    auto broadcast = waitForBroadcast( "hub.setting.updated", message );
    EXPECT_EQ( json_get_str( broadcast, "msg_subclass" ), "hub.setting.updated" ) << message;
    auto severity = json_get_str( params, "severity" );
    if( !severity.empty() )
        EXPECT_EQ( json_get_str( broadcast, "severity" ), severity ) << message;
    if( json_contains( params, "color" ) ) {
        EXPECT_EQ( json_get_bool( broadcast, "color" ), json_get_bool( params, "color" ) ) << message;
    }
    if( json_contains( params, "indent" ) ) {
        EXPECT_EQ( json_get_int( broadcast, "indent", -2 ), json_get_int( params, "indent", -3 ) ) << message;
    }
}

std::unique_ptr< Hub::Setting > HubLogLocalSetTests::getSetting( const std::string& name, const char* file, int line ) {
    if( auto settingUPtr = makeRemote().findSetting( name ) )
        return std::move( settingUPtr );
    ADD_FAILURE_AT( __FILE__, __LINE__ ) << "Setting not found: " << name << ", at: " << file << ":" << line;
    throw std::runtime_error( std::string{ "Setting not found: " } + name );
}

std::string HubLogLocalSetTests::getLogsLevelValue( const char* file, int line ) {
    const auto name = Services::Resources::HubSettings::LogsLevel;
    if( auto settingUPtr = getSetting( name, file, line ) ) {
        auto severity = settingUPtr->getStringValue();
        LOGGER_INFO( name + ": " + severity );
        return severity;
    }
    return "";
}

bool HubLogLocalSetTests::getLogsColorValue( const char* file, int line ) {
    const auto name = Services::Resources::HubSettings::LogsColor;
    if( auto settingUPtr = getSetting( name, file, line ) ) {
        bool isColored = settingUPtr->getBoolValue();
        LOGGER_INFO( name + ": " + cc::flag( isColored ) );
        return isColored;
    }
    return false;
}

int HubLogLocalSetTests::getLogsIndentValue( const char* file, int line ) {
    const auto name = Services::Resources::HubSettings::LogsIndent;
    if( auto settingUPtr = getSetting( name, file, line ) ) {
        int indent = stoi( settingUPtr->getStringValue() );
        LOGGER_INFO( name + ": " + cc::num10( indent ) );
        return indent;
    }
    return -1;
}

TEST_F( HubLogLocalSetTests, changeLogLevel ) /* 1.1 */ {
    LOGGER_INFO( "# test case: " + test_info_->name() );
    bool isColored = false;
    int indent = -1;
    std::string severity, newSeverity = "DEBUG";

    auto setDifferentSeverity = [&]( const std::string& level ) {
        if( level != severity ) {
            Log::debug( "set new severity: " + level );
            newSeverity = level;
        }
        return level != severity;
    };

    auto levelSetting = getSetting( Services::Resources::HubSettings::LogsLevel, __FILE__, __LINE__ );
    ASSERT_TRUE( levelSetting.get() != nullptr );
    auto possibilities = levelSetting->getPossibleTokenValues();

    isColored = getLogsColorValue( __FILE__, __LINE__ );
    indent = getLogsIndentValue( __FILE__, __LINE__ );

    severity = levelSetting->getStringValue();
    ASSERT_FALSE( possibilities.empty() );
    auto it1 = std::find_if( possibilities.rbegin(), possibilities.rend(), setDifferentSeverity );
    ASSERT_NE( severity, newSeverity );

    Log::info( "Change severity from: " + severity + " to " + newSeverity );
    changeLogSettings( JSON( "severity" << newSeverity ), NMA_TEST_MAKE_MESSAGE( "" ) );
    // check broadcast logs.level change
    checkSettingUpdated( JSON( HubSettings::LogsLevel << newSeverity ), NMA_TEST_MAKE_MESSAGE( "" ) );
    severity = getLogsLevelValue( __FILE__, __LINE__ );
    LOGGER_TRACE( "Severity before sleep: " + severity );
    LOGGER_INFO( "Sleep for 500 ms..." );
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

    severity = getLogsLevelValue( __FILE__, __LINE__ );
    LOGGER_TRACE( "Severity after sleep: " + severity );

    auto it2 = std::find_if( possibilities.rbegin(), possibilities.rend(), setDifferentSeverity );
    ASSERT_NE( severity, newSeverity );

    int newIndent = 8;
    if( severity != newSeverity && indent != newIndent ) {
        changeLogSettings( JSON( "severity" << newSeverity << "indent" << newIndent ), NMA_TEST_MAKE_MESSAGE( "" ) );

        LOGGER_INFO( "Sleep for 500 ms..." );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

        // check broadcast logs.level change
        if( severity != newSeverity ) {
            checkSettingUpdated( JSON( HubSettings::LogsLevel << newSeverity ), NMA_TEST_MAKE_MESSAGE( "" ) );
            severity = getLogsLevelValue( __FILE__, __LINE__ );
        }
        if( indent != newIndent ) {
            checkSettingUpdated( JSON( HubSettings::LogsIndent << newIndent ), NMA_TEST_MAKE_MESSAGE( "" ) );
            indent = getLogsIndentValue( __FILE__, __LINE__ );
        }
    }

    auto it3 = std::find_if( possibilities.rbegin(), possibilities.rend(), setDifferentSeverity );
    ASSERT_NE( severity, newSeverity );

    bool newColor = true;
    if( severity != newSeverity && isColored != newColor ) {
        changeLogSettings( JSON( "severity" << newSeverity << "color" << newColor ), NMA_TEST_MAKE_MESSAGE( "" ) );

        LOGGER_INFO( "Sleep for 500 ms..." );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

        // check broadcast logs.level change
        if( severity != newSeverity )
            checkSettingUpdated( JSON( HubSettings::LogsLevel << newSeverity ), NMA_TEST_MAKE_MESSAGE( "" ) );
        if( newColor != isColored )
            checkSettingUpdated( JSON( HubSettings::LogsColor << newColor ), NMA_TEST_MAKE_MESSAGE( "" ) );
    }
}

TEST_F( HubLogLocalSetTests, errorHandling ) /* 1.2 */ {
    LOGGER_INFO( "# test case: " + test_info_->name() );
    auto answer = callHubLogLocalSet( JSON( "severity"
                                            << "" ) );

    const auto errorSeverityDoeNotExist = R"({"code":-32602,
                    "data":"ezlo.severity.not.exist",
                    "message":"Name of severity doesn't exist",
                    "reason":"The severity with this name does not exist"
                })"_json;
    checkError( answer, errorSeverityDoeNotExist, NMA_TEST_MAKE_MESSAGE( "" ) );

    answer = callHubLogLocalSet( JSON( "severity"
                                       << "non-existent-value" ) );
    checkError( answer, errorSeverityDoeNotExist, NMA_TEST_MAKE_MESSAGE( "" ) );

    answer = callHubLogLocalSet( JSON( "severity" << 111 ) );
    checkError( answer,
                R"({"code":-32602,"data":"rpc.params.invalid.severity","message":"Wrong params, severity"})"_json,
                NMA_TEST_MAKE_MESSAGE( "" ) );

    answer = callHubLogLocalSet( JSON( "severity"
                                       << "TRACE"
                                       << "color"
                                       << "string-instead-of-bool" ) );
    checkError( answer,
                R"({"code":-32602,"data":"rpc.params.invalid.color","message":"Wrong params, color"})"_json,
                NMA_TEST_MAKE_MESSAGE( "" ) );

    answer = callHubLogLocalSet( JSON( "severity"
                                       << "TRACE"
                                       << "indent"
                                       << "string-instead-of-int" ) );
    checkError( answer,
                R"({"code":-32602,"data":"rpc.params.invalid.indent","message":"Wrong params, indent"})"_json,
                NMA_TEST_MAKE_MESSAGE( "" ) );
}

}  // namespace Tests
}  // namespace HubPlatform
