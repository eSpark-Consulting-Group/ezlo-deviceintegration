#include "scenes_manager/lua_interpreter/interpreter.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"
#include <gtest/gtest.h>

/*

 TESTPLAN

 DONE   1. Run the Lua interpreter
 DONE       1.1 Run with a simple expression in protected way;
 DONE       1.2 Run a script with a syntax error;
 DONE       1.3 Disable os.execute;
 DONE       1.4 Disable io module;
 DONE       1.5 Disable require();
 DONE       1.6 Disable loadfile();
 DONE       1.7 Run with params;

*/

namespace HubPlatform {
namespace UT {

class ScenesLuaInterpreterTests : public testing::Test {
public:
    void checkError( Json& scriptResult, const Json& scriptBody, const char* file, int line );

protected:
    HubPlatform::ScenesExpressions::ScenesLuaInterpreter m_interpreter;
};

void ScenesLuaInterpreterTests::checkError( Json& scriptResult, const Json& scriptBody, const char* file, int line ) {
    EXPECT_FALSE( m_interpreter.protectedCall( scriptBody, {}, scriptResult ) ) << file << ":" << line;
    LOGGER_TRACE( "Script execution failed as expected, result: " + cc::j( scriptResult ) );
    EXPECT_TRUE( scriptResult.count( "error" ) > 0 ) << file << ":" << line;
}

TEST_F( ScenesLuaInterpreterTests, run_1_1 ) /* 1.1 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );

    Json scriptResult;
    auto scriptBody = R"(return 2 + 2 * 2)";
    const int expected = 6;
    EXPECT_TRUE( m_interpreter.protectedCall( scriptBody, {}, scriptResult ) );
    ASSERT_EQ( scriptResult, expected );
}

TEST_F( ScenesLuaInterpreterTests, runScripWithError_1_2 ) /* 1.2 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );
    Json scriptResult;
    auto scriptBody = R"(
        local params=...
        syntax.error.here
        return 2 + 2 * 2)";
    checkError( scriptResult, scriptBody, __FILE__, __LINE__ );
}

TEST_F( ScenesLuaInterpreterTests, runScript_disableOsExecute_1_3 ) /* 1.3 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );
    HubPlatform::ScenesExpressions::ScenesLuaInterpreter interpreter;

    Json scriptResult;
    auto scriptBody = R"Bound(
        local params=...
        print('Before os.execute')
        os.execute( "sleep 1" )
        print('After os.execute')
        return result
    )Bound";
    checkError( scriptResult, scriptBody, __FILE__, __LINE__ );
}

TEST_F( ScenesLuaInterpreterTests, runScript_disableModuleIo_1_4 ) /* 1.4 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );
    Json scriptResult;
    auto scriptBody = R"(
        local params=...
        local handle = io.popen('ls -lA /')
        local result = handle:read("*a")
        print( 'Result from opened pipe: ', result )
        handle:close()
        return result)";
    checkError( scriptResult, scriptBody, __FILE__, __LINE__ );
}

TEST_F( ScenesLuaInterpreterTests, runScript_disableRequire_1_5 ) /* 1.5 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );
    Json scriptResult;
    auto scriptBody = R"(
        local params=...
        local handle = require('math')
        return 1)";
    checkError( scriptResult, scriptBody, __FILE__, __LINE__ );
}

TEST_F( ScenesLuaInterpreterTests, runScript_disableLoadFile_1_6 ) /* 1.6 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );
    Json scriptResult;
    auto scriptBody = R"(
        local params=...
        local handle = loadfile('a')
        return 1)";
    checkError( scriptResult, scriptBody, __FILE__, __LINE__ );
}

TEST_F( ScenesLuaInterpreterTests, runScriptWithParams_1_7 ) /* 1.7 */ {
    LOGGER_TRACE( "# Test started: " + cc::note( test_info_->name() ) );
    Json scriptResult;
    auto scriptBody = R"(
        local params=...
        return params.key1)";
    Json scriptParam = { { "key1", "value1" } };
    EXPECT_TRUE( m_interpreter.protectedCall( scriptBody, scriptParam, scriptResult ) );
    LOGGER_TRACE( "Script exec Ok, result: " + cc::j( scriptResult ) );
    EXPECT_EQ( scriptResult, "value1" );
}

}  // namespace UT
}  // namespace HubPlatform