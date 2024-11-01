#include "luad/interpreter/lua_interpreter.h"
#include "luad/interpreter/module_loader/lua_module_loader.h"
#include "luad/addon/lua_addon.h"
#include "luad/addon/lua_status_data.h"
#include "luad/interpreter/lua_functions.h"
#include "luad/tests/fakes/fake_lua_plugin_resources_provider.h"

#include "binding_interfaces/lua/lua_binding_interface.h"

#include "database_client/fake_db/fake_db_client.h"
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "cored/model/resources/cored_resources.h"
#include "bus/fake_bus.h"
#include "base_utils/script_result.h"

#include "common_utils/cpp14.h"
#include "base_utils/utils.h"
#include "system_status/ram_usage.h"
#include "json_utils/json_approximate_size.h"

#include <gtest/gtest.h>
#include <malloc.h>

#include <ctime>

/*

TESTPLAN

DONE    1. Lua interpreter code loading
DONE        1.1 Check a script execution from a stream
DONE        1.2 Check no throwing when fail to require a plugin's script
DONE        1.3 Check throwing an exception when fail to load a plugin's script
DONE        1.4 Check throwing an exception when fail to load a lua string
DONE        1.5 Check throwing an exception when fail to load a standalone script
DONE        1.6 Check require() return nil on module that doesn't exist
DONE    2. Push json as a lua object
DONE        ...
DONE    3. Pull json from a lua object
DONE        ...
DONE    3. Lookup lua table
DONE        3.1 Simple table
DONE            3.1.1 Empty
DONE            3.1.2 Not empty
DONE        3.2 With metatable
DONE            3.2.1 Empty
DONE            3.2.1 Not empty
DONE    4. Anonymous plugins
DONE        4.1 Mark lua_state as running anonymous plugins
DONE        4.2 Set NO_ASYNC Lua api restriction running anonymous plugins
DONE        4.3 Set NO_RESTRICTIONS Lua api restriction running regular plugins
DONE        4.4 Check throwing an exception when fail to load an anonymous plugin

*/

namespace HubPlatform {
namespace Luad {
namespace UT {

class LuaTestSuite : public testing::Test {
public:
    LuaTestSuite()
    : m_luaAddon( std::make_unique< Services::BusWrapper >(
                      Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
                  std::make_unique< HubPlatform::UT::FakeDbClient >() )
    , m_luaInterpreter( m_luaAddon, m_luaPluginResourcesProvider ) {}

    void SetUp() override {
        m_luaAddon.setUp();
    }

    void TearDown() override {
        m_luaAddon.tearDown();
    }

    void testPushJsonAsLuaObj( const char* script,
                               KeyFormatter keyFormatter,
                               size_t keyFormatDepth,
                               const Json& json ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        pushJsonAsLuaObj( luaInterp.m_luaState.get(), json, keyFormatter, keyFormatDepth );
        ASSERT_EQ( LuaFunctions::protectedCall( luaInterp.m_luaState.get(), 1, LUA_MULTRET ), 0 );
    }

    void testPushJsonAsLuaObj( const char* script, Json json, bool handleKeyTypes = false ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );

        ASSERT_FALSE(
            luaInterp.runLoadedCode( LuaScriptJsonParams::make( std::move( json ), handleKeyTypes ) ).hasError() );
    }

    void testPullJsonFromLuaObj( const char* script,
                                 const Json& json,
                                 bool handleKeyTypes = false,
                                 Json* retVal = nullptr ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        ASSERT_FALSE( luaInterp.runLoadedCode( nullptr, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );
        Json loaded = pullJson( luaInterp.m_luaState.get(), 1, nullptr, 0, handleKeyTypes );
        if( retVal ) {
            *retVal = loaded;
        }
        if( !json.empty() ) {
            EXPECT_EQ( json, loaded );
        }
    }

    void testPullJsonFromLuaObjWithErrorAndTimeout( const char* script, const std::string& errorString, int timeout ) {
        auto interpreter = createLuaInterpreter( timeout );
        interpreter.loadLuaString( script );
        ASSERT_TRUE( interpreter.runLoadedCode( nullptr, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );

        Json loaded = pullJson( interpreter.m_luaState.get(), 1, nullptr, 0, false );
        LOGGER_ERROR( "loaded: " + cc::j( loaded ) );
        std::string str = loaded.get< std::string >();
        ASSERT_TRUE( str.find( errorString ) != std::string::npos );
    }

    void checkTimeout( int timeout ) {
        auto interpreterTimeout = m_luaAddon.m_scriptTimeout;
        ASSERT_EQ( interpreterTimeout, timeout );
    }

    void initTimeoutSettings() {
        m_luaAddon.getDBClient().insert( CoreD::Model::Resources::DBNames::SettingsDB,
                                         JSON( "_id"
                                               << "lua.default_timeout"
                                               << "value" << 60 ) );

        m_luaAddon.getDBClient().insert( CoreD::Model::Resources::DBNames::SettingsDB,
                                         JSON( "_id"
                                               << "lua.min_timeout"
                                               << "value" << 30 ) );
    }

    void setTimeoutSettings( int minTimeout, int defaultTimeout ) {
        m_luaAddon.getDBClient().update( CoreD::Model::Resources::DBNames::SettingsDB,
                                         JSON_EMPTY_OBJ,
                                         JSON( "_id"
                                               << "lua.default_timeout"
                                               << "value" << defaultTimeout ),
                                         true,
                                         false );

        m_luaAddon.getDBClient().update( CoreD::Model::Resources::DBNames::SettingsDB,
                                         JSON_EMPTY_OBJ,
                                         JSON( "_id"
                                               << "lua.min_timeout"
                                               << "value" << minTimeout ),
                                         true,
                                         false );
        m_luaAddon.setUpTimeouts();
    }

    void testPushAndPullJson( const char* script,
                              const Json& json,
                              const Json* pushParams = nullptr,
                              Json* retVal = nullptr ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        ASSERT_FALSE( luaInterp.runLoadedCode( pushParams, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );
        Json loaded = pullJson( luaInterp.m_luaState.get(), 1 );
        if( retVal ) {
            *retVal = loaded;
        }
        EXPECT_EQ( json, loaded );
    }

    void testPullJsonFromLuaObj( const char* script,
                                 KeyFormatter keyFormatter,
                                 size_t keyFormatDepth,
                                 const Json& json ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        ASSERT_FALSE( luaInterp.runLoadedCode( nullptr, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );
        EXPECT_EQ( json, pullJson( luaInterp.m_luaState.get(), 1, keyFormatter, keyFormatDepth ) );
    }

    template < typename NativeObject >
    void testPullFromInvalidLuaObj( const char* script ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        ASSERT_FALSE( luaInterp.runLoadedCode( nullptr, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );

        LuaToNative< NativeObject > luaToNative;
        ASSERT_FALSE( luaToNative( luaInterp.m_luaState.get(), 1 ).isValid() );
    }

    template < typename NativeObject >
    void testPullFromLuaObj( const char* script, const NativeObject& expectedNativeObject ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        ASSERT_FALSE( luaInterp.runLoadedCode( nullptr, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );

        LuaToNative< NativeObject > luaToNative;
        EXPECT_EQ( expectedNativeObject, luaToNative( luaInterp.m_luaState.get(), 1 ).value() );
    }

    template < typename T >
    LuaLookupRes< T > lookupTable( const char* script, const std::string& key ) {
        auto luaInterp = createLuaInterpreter();
        luaInterp.loadLuaString( script );
        EXPECT_FALSE( luaInterp.runLoadedCode( nullptr, LuaInterpreter::RunCodeType::SAVE_RESULTS ).hasError() );
        return lookupLuaTable< T >( luaInterp.m_luaState.get(), key );
    }

    LuaInterpreter createLuaInterpreter( int timeout = 0 ) {
        return timeout > 0 ?
                   LuaInterpreter( m_luaAddon, m_luaPluginResourcesProvider, std::chrono::seconds( timeout ) ) :
                   LuaInterpreter( m_luaAddon, m_luaPluginResourcesProvider );
    }

    LuaAddon::LuaAddon& getLuaAddon() {
        return m_luaAddon;
    }

    LuaInterpreter& getLuaInterpreter() {
        return m_luaInterpreter;
    }

    LuaModuleLoader& getLuaModuleLoader() {
        return *m_luaInterpreter.m_moduleLoader;
    }

    const Services::UT::FakeResponseBus& getTestBus() {
        return *dynamic_cast< Services::UT::FakeResponseBus* >( getLuaAddon().getBus() );
    }

protected:
    LuaAddon::LuaAddon m_luaAddon;
    LuaInterpreter m_luaInterpreter;
    FakeLuaPluginResourcesProvider m_luaPluginResourcesProvider;
};

// Note: It is a manual test. It is needed to investigate time of LuaInterpreter startup.
TEST_F( LuaTestSuite, DISABLED_testCreationDestroyingTime ) {
    const int tryAmount = 5000;
    auto startTime = clock();

    for( auto i = 0; i < tryAmount; i++ ) {
        auto luaInterp = createLuaInterpreter();
    }

    auto elapsedTime = double( clock() - startTime ) / CLOCKS_PER_SEC;
    auto averageTime = elapsedTime / tryAmount;

    std::cout << "An average time for LuaInterpreter object creation & destroying: " << averageTime << "s\n";
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjInteger ) {
    testPushJsonAsLuaObj( "local t = ... "
                          "assert(t.id == 1, 'Invalid id')\n"
                          "assert(tostring(t.id) == '1', 'Invalid id to str')\n",
                          JSON( "id" << 1 ) );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjFloat ) {
    testPushJsonAsLuaObj( "local t = ... "
                          "assert(t.id == 3.14, 'Invalid id')\n",
                          JSON( "id" << 3.14 ) );

    // Can be reimplemented to tostring(t.id) == '1.0'; but in case of conversion json->string->json float becomes int
    // automatically. So actually it doesn't matter

    // related check for json:
    EXPECT_EQ( JSON( "id" << 1.0f ).dump(), JSON( "id" << 1 ).dump() );

    testPushJsonAsLuaObj( "local t = ... "
                          "assert(t.id == 1, 'Invalid id')\n"
                          "assert(tostring(t.id) == '1', 'Invalid id to str')\n",
                          JSON( "id" << 1.0f ) );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjBool ) {
    testPushJsonAsLuaObj( "local t = ... "
                          "if (t.bool ~= true) then error('Invalid t') end\n",
                          JSON( "bool" << true ) );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjNull ) {
    testPushJsonAsLuaObj( "local t = ... "
                          "if (t.null ~= nil) then error('Invalid t') end\n",
                          JSON( "null" << nullptr ) );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjArray ) {
    Json array = { 5, 6, 3 };
    testPushJsonAsLuaObj( "local t = ... "
                          "if (t[1] ~= 5) then error('Invalid t') end\n"
                          "if (t[2] ~= 6) then error('Invalid t') end\n"
                          "if (t[3] ~= 3) then error('Invalid t') end\n"
                          "if (#t ~= 3) then error('Invalid t') end\n",
                          array );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjWithKeysTypeHandling ) {
    testPushJsonAsLuaObj( "local t = ...\n"
                          "assert(t['\\\\'] == 1, 'field \\\\ not found')\n"
                          "assert(t['\\\\key_with_slash'] == 2, 'field \\\\key_with_slash not found')\n"
                          "assert(t[1.1] == 3, 'field 1.1 not found')\n"
                          "assert(t[23] == 4, 'field 23 not found')\n"
                          "assert(t[true] == 5, 'field true not found')\n"
                          "assert(t[false] == 6, 'field false not found')\n"
                          "assert(t[100] == 7, 'field 100 not found')\n"
                          "assert(t[-0.01] == 8, 'field 0.01 not found')\n"
                          "assert(t[-7] == 9, 'field -7 not found')\n",
                          JSON( "\\\\" << 1 << "\\\\key_with_slash" << 2 << "\\f1.1" << 3 << "\\i23" << 4 << "\\btrue"
                                       << 5 << "\\bfalse" << 6 << "\\f1e+2" << 7 << "\\f-1E-2" << 8 << "\\i-7" << 9 ),
                          true );

    testPushJsonAsLuaObj( "local t = ...\n"
                          "assert(t['\\\\'] == 1, 'field \\\\ not found')\n"
                          "assert(t['\\\\btruee'] == 2, 'field \\\\btruee not found')\n"
                          "assert(t['\\\\f1.1x'] == 3, 'field \\\\f1.1x not found')\n"
                          "assert(t['\\\\n1'] == 4, 'field \\\\n1 not found')\n"
                          "assert(t['\\\\t1'] == 5, 'field \\\\t1 not found')\n",
                          JSON( "\\" << 1 << "\\btruee" << 2 << "\\f1.1x" << 3 << "\\n1" << 4 << "\\t1" << 5 ),
                          true );

    testPushJsonAsLuaObj( "local t = ...\n"
                          "assert(t['\\\\\\\\'] == 1, 'field \\\\\\\\ not found')\n"
                          "assert(t['\\\\\\\\key_with_slash'] == 2, 'field \\\\\\\\key_with_slash not found')\n"
                          "assert(t['\\\\f1.1'] == 3, 'field \\\\f1.1 not found')\n"
                          "assert(t['\\\\i23'] == 4, 'field \\\\f23 not found')\n"
                          "assert(t['\\\\btrue'] == 5, 'field \\\\btrue not found')\n"
                          "assert(t['\\\\bfalse'] == 6, 'field \\\\bfalse not found')\n"
                          "assert(t['\\\\f1e+2'] == 7, 'field \\\\f1e+2 not found')\n"
                          "assert(t['\\\\f1E-2'] == 8, 'field \\\\f1E-2 not found')\n",
                          JSON( "\\\\" << 1 << "\\\\key_with_slash" << 2 << "\\f1.1" << 3 << "\\i23" << 4 << "\\btrue"
                                       << 5 << "\\bfalse" << 6 << "\\f1e+2" << 7 << "\\f1E-2" << 8 ),
                          false );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjEmptyArray ) {
    testPushJsonAsLuaObj( "local t = ...\n"
                          "if (type(t) ~= 'table') then error('Invalid type: ' .. type(t)) end\n"
                          "if (next(t) ~= nil) then error('Invalid value: ' .. next(t)) end\n",
                          Json::array() );
}

TEST_F( LuaTestSuite, testStorageBindings ) {
    constexpr const char luaCode[] =
        R"(
local fails_number = 0
local function expect_eq(actual, expected, description)
    if actual ~= expected then
        local info = debug.getinfo(2, "Sl")
        print(('Storage Test Fail. %g: %s. AR: %s, ER: %s'):format(info.currentline, description, tostring(actual), tostring(expected)))
        fails_number = fails_number + 1
    end

    return true
end

local storage = require 'storage'

storage.set_string('\\!@#$%^&*()_+-=~`/<>:;|', '\\\\')
storage.set_string('string_1', '1')
storage.set_string('string_1.0', '1.0')

storage.set_number('number_1', 1)
storage.set_number('number_1.0', 1.0)
storage.set_number('number_1.9', 1.9)
storage.set_number('number_2000000123', 2000000123)
storage.set_number('number_-2000000123', -2000000123)

storage.set_number('number -1', -1)
storage.set_number('number -1.9', -1.9)

storage.set_bool('bool true', true)
storage.set_bool('bool false', false)

storage.set_table('empty_table', { })
storage.set_table('array', { 1, 5, 9 })
storage.set_table('simple_sparse_array', { [1] = 'x', [2] = true, [4] = 'y' })
storage.set_table('simple_table', { a = 'x', b = 2.0, c = true, d = 5.6 })
storage.set_table('simple_table_with_string_key_like_int', { ["123123"] = 'x' })
storage.set_table('mixed_table', {
    [0] = -2010000567,
    [1] = false,
    [3] = '2',
    [-3.1] = true,
    ['1'] = 'x',
    ['false'] = -1.4,
    [false] = {
        [1] = 2,
        [2010000981] = 0,
        [-2010000981] = 1,
        [true] = { }
    } })

storage.set_table('array with not supported types', { 'a', function() end, 'b' } )

storage.set_table('table_with_not_supported_types_and_metatable',
    setmetatable(
        {
            a = 'b',
            [{}] = 'c',
            [function() end] = 'd',
            d = function() end
        },
        { __index = { c = 'e' } }
    )
)

local function make_table_with_recurse( )
    local result = { }
    result.a = 'b'
    result.c = { d = result }
    result.e = result
    return result
end

storage.set_table('table_with_recurse', make_table_with_recurse())

do
    expect_eq(storage.get_string('\\!@#$%^&*()_+-=~`/<>:;|'), '\\\\', 'check string value with special key')
    expect_eq(storage.get_string('string_1'), '1', 'check value string_1')
    expect_eq(storage.get_string('string_1.0'), '1.0', 'check value string_1.0')
end

do
    expect_eq(storage.get_number('number_1'), 1, 'check value number_1')
    expect_eq(tostring(storage.get_number('number_1')), '1', 'check value number_1 tostring')

    expect_eq(storage.get_number('number_1.0'), 1, 'check value number_1.0')
    expect_eq(tostring(storage.get_number('number_1.0')), '1' --[[1.0?]], 'check value number_1.0 tostring (?)')

    expect_eq(storage.get_number('number_1.9'), 1.9, 'check value number_1.9')

    expect_eq(storage.get_number('number -1'), -1, 'check value number -1')
    expect_eq(tostring(storage.get_number('number -1')), '-1', 'check value number -1 tostring')

    expect_eq(storage.get_number('number_2000000123'), 2000000123, 'check value number_2000000123')
    expect_eq(tostring(storage.get_number('number_2000000123')), '2000000123', 'check value 2000000123 tostring')

    expect_eq(storage.get_number('number_-2000000123'), -2000000123, 'check value number_-2000000123')
    expect_eq(tostring(storage.get_number('number_-2000000123')), '-2000000123', 'check value -2000000123 tostring')
end

do
    expect_eq(storage.get_bool('bool true'), true, 'check value bool true')
    expect_eq(storage.get_bool('bool false'), false, 'check value bool false')
end

do
    expect_eq(next(storage.get_table('empty_table')), nil, 'check value empty_table')
end

do
    local array = storage.get_table('array')

    if expect_eq(type(array), 'table', 'check array type') then
        expect_eq(array[3], 9, 'check value array.3')
        expect_eq(array[2], 5, 'check value array.2')
        expect_eq(array[1], 1, 'check value array.3')

        array[3] = nil; array[2] = nil; array[1] = nil
        expect_eq(next(array), nil, 'check no extra elements in array')
    end
end

do
    local array = storage.get_table('simple_sparse_array')

    if expect_eq(type(array), 'table', 'check simple_sparse_array type') then
        expect_eq(array[1], 'x', 'check value simple_sparse_array.1')
        expect_eq(array[2], true, 'check value simple_sparse_array.2')
        expect_eq(array[4], 'y', 'check value simple_sparse_array.4')

        array[4] = nil; array[2] = nil; array[1] = nil
        expect_eq(next(array), nil, 'check no extra elements in simple_sparse_array')
    end
end

do
    local simple_table = storage.get_table('simple_table')

    if expect_eq(type(simple_table), 'table', 'check simple_table type') then
        expect_eq(getmetatable(simple_table), nil, 'check existing simple_table metatable is nil')

        expect_eq(simple_table.a, 'x', 'check value simple_table.a')
        expect_eq(simple_table.b, 2, 'check value simple_table.b')
        expect_eq(tostring(simple_table.b), '2', 'check value simple_table.b tostring')
        expect_eq(simple_table.c, true, 'check value simple_table.c')
        expect_eq(simple_table.d, 5.6, 'check value simple_table.d')

        simple_table.a = nil; simple_table.b = nil; simple_table.c = nil; simple_table.d = nil
        expect_eq(next(simple_table), nil, 'check no extra elements in simple_table')
    end
end

do
    local simple_table = storage.get_table('simple_table_with_string_key_like_int')

    if expect_eq(type(simple_table), 'table', 'check simple_table_with_string_key_like_int type') then
        expect_eq(getmetatable(simple_table), nil, 'check existing simple_table_with_string_key_like_int metatable is nil')

        expect_eq(simple_table["123123"], 'x', 'check value simple_table_with_string_key_like_int["123123"]')

        simple_table["123123"] = nil
        expect_eq(next(simple_table), nil, 'check no extra elements in simple_table_with_string_key_like_int')
    end
end

do
    local mixed_table = storage.get_table('mixed_table')

    if expect_eq(type(mixed_table), 'table', 'check value mixed_table type') then
        expect_eq(getmetatable(mixed_table), nil, 'check existing mixed_table metatable is nil')

        expect_eq(mixed_table[0], -2010000567, 'check value mixed_table[0]')
        expect_eq(tostring(mixed_table[0]), '-2010000567', 'check value mixed_table[0] tostring')
        expect_eq(mixed_table[1], false, 'check value mixed_table[1]')
        expect_eq(mixed_table[3], '2', 'check value mixed_table[3]')
        expect_eq(mixed_table[-3.1], true, 'check value mixed_table[-3.1]')
        expect_eq(mixed_table['1'], 'x', 'check value mixed_table["1"]')
        expect_eq(mixed_table['false'], -1.4, 'check value mixed_table["false"]')

        local sub_table = mixed_table[false]

        expect_eq(type(sub_table), 'table', 'check value mixed_table[false] type')

        if sub_table then
            expect_eq(sub_table[1], 2, 'check value mixed_table[false][1]')
            expect_eq(tostring(sub_table[1]), '2' --[['2.0'?]], 'check value mixed_table[false][1] tostring (?)')

            expect_eq(sub_table[2010000981], 0, 'check value mixed_table[false][2010000981]')
            expect_eq(sub_table[-2010000981], 1, 'check value mixed_table[false][-2010000981]')

            if expect_eq( type(sub_table[true]), 'table', 'check value mixed_table[false][true] type') then
                expect_eq( next(sub_table[true]), nil, 'check value mixed_table[false][true] empty')
            end
        end

        mixed_table[0] = nil; mixed_table[1] = nil; mixed_table[3] = nil; mixed_table[-3.1] = nil;
        mixed_table['1'] = nil; mixed_table['false'] = nil; mixed_table[false] = nil
        expect_eq(next(mixed_table), nil, 'check no extra elements in mixed_table')
    end
end

do
    local array = storage.get_table('array with not supported types')

    if expect_eq(type(array), 'table', 'check value array type') then
        expect_eq(array[1], 'a', 'check value array.1')
        expect_eq(array[3], 'b', 'check value array.3')

        array[3] = nil; array[1] = nil
        expect_eq(next(array), nil, 'check no extra elements in array')
    end
end

do
    local t = storage.get_table('table_with_not_supported_types_and_metatable')

    if expect_eq(type(t), 'table', 'check value table_with_not_supported_types_and_metatable type') then
        expect_eq(getmetatable(t), nil, 'check table_with_not_supported_types_and_metatable metatable is nil')

        expect_eq(t.a, 'b', 'check value table_with_not_supported_types_and_metatable.a')
        t.a = nil
        expect_eq(next(t), nil, 'check no extra elements in table_with_not_supported_types_and_metatable')
    end
end

do
    local table_with_recurse = storage.get_table('table_with_recurse')

    if expect_eq(type(table_with_recurse), 'table', 'check value table_with_recurse type') then
        expect_eq(getmetatable(table_with_recurse), nil, 'check table_with_recurse metatable is nil')

        expect_eq(table_with_recurse.a, 'b', 'check value table_with_not_supported_types.a')
        if (expect_eq(type(table_with_recurse.c), 'table', 'check value table_with_recurse.c')) then
            expect_eq(next(table_with_recurse.c), nil, 'check table_with_recurse.c empty (no recurse)')
        end

        table_with_recurse.a = nil; table_with_recurse.c = nil
        expect_eq(next(table_with_recurse), nil, 'check no extra elements in table_with_recurse')
    end
end

do
    local to_be_removed = {
        '\\!@#$%^&*()_+-=~`/<>:;|',
        'string_1',
        'string_1.0',
        'number_1',
        'number_1.0',
        'number_1.9',
        'number -1',
        'number -1.9',
        'number_2000000123',
        'number_-2000000123',
        'bool true',
        'bool false',
        'empty_table',
        'array',
        'simple_sparse_array',
        'simple_table',
        'simple_table_with_string_key_like_int',
        'mixed_table',
        'array with not supported types',
        'table_with_not_supported_types_and_metatable',
        'table_with_recurse'
    }

    for _, key in ipairs(to_be_removed) do
        expect_eq(storage.exists(key), true, 'check element ' .. key .. ' exists before removing')
        storage.delete(key)
        expect_eq(storage.exists(key), false, 'check element ' .. key .. ' is removed')
    end
end

if fails_number ~= 0 then
    error("Storage test fails: " .. fails_number)
else
    print("Storage test passed.")
end
)";

    auto luaInterp = createLuaInterpreter();
    luaInterp.loadLuaString( luaCode );
    ASSERT_FALSE( luaInterp.runLoadedCode().hasError() );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjEmptyObject ) {
    testPushJsonAsLuaObj( "local t = ...\n"
                          "if (type(t) ~= 'table') then error('Invalid type: ' .. type(t)) end\n"
                          "if (next(t) ~= nil) then error('Invalid value: ' .. next(t)) end\n",
                          JSON_EMPTY_OBJ );
}

TEST_F( LuaTestSuite, testPushJsonAsLuaObjWithKeyFormatter ) {
    testPushJsonAsLuaObj( "local t = ...\n"
                          "assert(t.abc_de == 1, 'field abc_de not found')\n"
                          "assert(type(t.abc_def) == 'table', 'field abc_def not found')\n"
                          "assert(t.abc_def.xYz == 2, 'field abc_def.xYz not found')\n",
                          camelToSnakeCase,
                          0,
                          JSON( "abcDe" << 1 << "abcDef" << JSON( "xYz" << 2 ) ) );

    testPushJsonAsLuaObj( "local t = ...\n"
                          "assert(t.abc_de == 1, 'field abc_de not found')\n"
                          "assert(t.abc_def.x_yz.zzX == 2, 'field abc_def.x_yz.zzX not found')\n",
                          camelToSnakeCase,
                          1,
                          JSON( "abcDe" << 1 << "abcDef" << JSON( "xYz" << JSON( "zzX" << 2 ) ) ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjString ) {
    testPullJsonFromLuaObj( "local obj = 'str'\n"
                            "return obj",
                            Json( "str" ) );
}

namespace {
std::string makeTempFileName() {
    char tempFileName[] = "/tmp/fw_tests_luad.XXXXXX";
    close( mkstemp( tempFileName ) );
    return tempFileName;
}
}  // namespace

TEST_F( LuaTestSuite, testPullJsonFromLuaObjBinaryString ) {
    char binaryPattern[] = "Binary string\t\0Hidden part\r\n";
    std::string binaryString( binaryPattern, sizeof( binaryPattern ) - 1 );
    ASSERT_GT( binaryString.size(), std::string{ binaryString.c_str() }.size() );

    std::string tmpFileName;
    using FileUPtr = std::unique_ptr< FILE, int ( * )( FILE* ) >;
    using TmpFileNameUPtr = std::unique_ptr< const char, int ( * )( const char* ) >;
    TmpFileNameUPtr unlinkTempFile{ nullptr, unlink };
    FileUPtr os{ nullptr, fclose };
    do {
        tmpFileName = makeTempFileName();
        LOGGER_TRACE( tmpFileName );
        FileUPtr tmpFile( fopen( tmpFileName.c_str(), "wb" ), fclose );
        if( tmpFile ) {
            //            unlink( tmpFileName.c_str() );  // make it temporary, exists till fclose() called
            os = std::move( tmpFile );
            TmpFileNameUPtr removeFile{ tmpFileName.c_str(), &::unlink };
            unlinkTempFile = std::move( removeFile );
            ASSERT_EQ( fwrite( binaryString.data(), 1, binaryString.size(), os.get() ), binaryString.size() );
            fflush( os.get() );
            break;
        }
    } while( true );

    std::string script = R"(
        local inp = assert(io.open('$FILE', "rb"))
        local data = inp:read("*all")
        return data;
    )";
    replaceAll( script, "$FILE", tmpFileName );
    Json loaded;
    testPullJsonFromLuaObj( script.c_str(), Json( binaryString ), false, &loaded );

    std::string str = loaded.get< std::string >();
    ASSERT_EQ( binaryString.size(), str.size() );
}

TEST_F( LuaTestSuite, testPushAndPullJsonBinary ) {
    char binaryPattern[] = "Binary string\t\0Hidden part\r\n";
    std::string binaryString( binaryPattern, sizeof( binaryPattern ) - 1 );
    ASSERT_GT( binaryString.size(), std::string{ binaryString.c_str() }.size() );

    std::string script = "local data=...\n  return data";

    {
        Json pulled;
        Json pushParams = Json( binaryString );
        testPushAndPullJson( script.c_str(), pushParams, &pushParams, &pulled );
        LOGGER_TRACE( pulled.dump() );
        ASSERT_EQ( pulled.get< std::string >().size(), binaryString.size() );
    }
    {
        Json pulled;
        Json pushParams = { { "key", binaryString } };
        testPushAndPullJson( script.c_str(), pushParams, &pushParams, &pulled );
        LOGGER_TRACE( pulled.dump() );
        ASSERT_EQ( pulled.at( "key" ).get< std::string >().size(), binaryString.size() );
    }
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjInt ) {
    testPullJsonFromLuaObj( "local obj = 10\n"
                            "return obj",
                            Json( 10 ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjFloat ) {
    testPullJsonFromLuaObj( "local obj = 1.25\n"
                            "return obj",
                            Json( 1.25 ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjBool ) {
    testPullJsonFromLuaObj( "local obj = true \n"
                            "return obj",
                            Json( true ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjNil ) {
    testPullJsonFromLuaObj( "local obj = nil\n"
                            "return obj",
                            Json() );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjArray ) {
    testPullJsonFromLuaObj( "local obj = {11, 1, 2, 3, 10}\n"
                            "return obj",
                            Json::array( { 11, 1, 2, 3, 10 } ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjWithStringKeysLikeInt ) {
    testPullJsonFromLuaObj( "return { ['1']='x' }",
                            JSON( "1"
                                  << "x" ) );

    testPullJsonFromLuaObj( "return { ['123123123']='x' }",
                            JSON( "123123123"
                                  << "x" ) );

    testPullJsonFromLuaObj( "return { ['0']='x' }",
                            JSON( "0"
                                  << "x" ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjEmptyTable ) {
    testPullJsonFromLuaObj( "local obj = {}\n"
                            "return obj",
                            JSON_EMPTY_OBJ );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjTable ) {
    testPullJsonFromLuaObj( "local obj = {}\n"
                            "obj.id = 10\n"
                            "obj.str = 'str'\n"
                            "return obj",
                            JSON( "id" << 10 << "str"
                                       << "str" ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjTableWithNumberKey ) {
    {
        testPullJsonFromLuaObj( "return { [1] = 1, [11] = 11 }\n", JSON( "1" << 1 << "11" << 11 ) );
        testPullJsonFromLuaObj( "return { [2] = 20, [3] = 30 }\n", JSON( "2" << 20 << "3" << 30 ) );
        testPullJsonFromLuaObj( "return { [0] = 0, [1] = 1 }\n", JSON( "0" << 0 << "1" << 1 ) );
        testPullJsonFromLuaObj( "return { [1.1] = 1, [2] = 2 }\n", JSON( "1.1" << 1 << "2" << 2 ) );
        testPullJsonFromLuaObj( "return { [0.0] = 0, [-1] = 1, [-2000000976] = -2000000976 }\n",
                                JSON( "0" << 0 << "-1" << 1 << "-2000000976" << -2000000976 ) );
    }

    {
        testPullJsonFromLuaObj( "return { [1] = 1, [11] = 11 }\n", JSON( "\\i1" << 1 << "\\i11" << 11 ), true );
        testPullJsonFromLuaObj( "return { [2] = 20, [3.0] = 30 }\n", JSON( "\\i2" << 20 << "\\i3" << 30 ), true );
        testPullJsonFromLuaObj( "return { [0] = 0, [1] = 1 }\n", JSON( "\\i0" << 0 << "\\i1" << 1 ), true );
        testPullJsonFromLuaObj( "return { [1.1] = 1, [2] = 2 }\n", JSON( "\\f1.1" << 1 << "\\i2" << 2 ), true );
        testPullJsonFromLuaObj( "return { [0.0] = 0, [-1] = 1, [-2000000976] = -2000000976 }\n",
                                JSON( "\\i0" << 0 << "\\i-1" << 1 << "\\i-2000000976" << -2000000976 ),
                                true );
    }
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjTableWithStringKey ) {
    testPullJsonFromLuaObj( R"(
                                local obj = {}
                                obj["1"] = 1
                                obj["11"] = 11
                                obj["str"] = "str"
                                return obj
                            )",
                            JSON( "1" << 1 << "11" << 11 << "str"
                                      << "str" ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjMixedArrayAndTable ) {
    testPullJsonFromLuaObj( "local obj = {1, 2}\n"
                            "obj.id = 10\n"
                            "obj.str = 'str'\n"
                            "obj[true]=false\n"
                            "return obj",
                            JSON( "1" << 1 << "2" << 2 << "id" << 10 << "str"
                                      << "str"
                                      << "true" << false ),
                            false );

    testPullJsonFromLuaObj( "local obj = {1, 2}\n"
                            "obj.id = 10\n"
                            "obj.str = 'str'\n"
                            "obj[false]=true\n"
                            "return obj",
                            JSON( "\\i1" << 1 << "\\i2" << 2 << "id" << 10 << "str"
                                         << "str"
                                         << "\\bfalse" << true ),
                            true );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjComplexTableWithArray ) {
    auto longArray = Json::array();
    for( int i = 1; i <= 10000; ++i ) {
        longArray.push_back( i );
    }

    {
        testPullJsonFromLuaObj( "local obj = {}\n"
                                "obj.id = 10\n"
                                "obj.array = {1, 2, 4}\n"
                                "return obj, 'some extra result to test stack index'",
                                JSON( "id" << 10 << "array" << Json::array( { 1, 2, 4 } ) ),
                                false );

        testPullJsonFromLuaObj( "local obj = {}\n"
                                "obj.id = 10\n"
                                "obj.array = {}\n"
                                "for i=1,10000 do obj.array[i]=i end\n"
                                "return obj",
                                JSON( "id" << 10 << "array" << longArray ),
                                false );
    }

    {
        testPullJsonFromLuaObj( "local obj = {}\n"
                                "obj.id = 10\n"
                                "obj.array = {}\n"
                                "for i=1,10000 do obj.array[i]=i end\n"
                                "return obj",
                                JSON( "id" << 10 << "array" << longArray ),
                                true );
    }
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjComplexTable ) {
    testPullJsonFromLuaObj( "local obj = {id = 10, obj = {id = 1, str = 'abc', obj = { id = 2}}}\n"
                            "return obj",
                            JSON( "id" << 10 << "obj"
                                       << JSON( "id" << 1 << "str"
                                                     << "abc"
                                                     << "obj" << JSON( "id" << 2 ) ) ) );
}

TEST_F( LuaTestSuite, testPullJsonFromLuaObjWithKeyFormatter ) {
    testPullJsonFromLuaObj( "local obj = {id = 10, first_object = {}, second_object = { third_object = {} } }\n"
                            "return obj",
                            snakeToCamelCase,
                            0,
                            JSON( "id" << 10 << "firstObject" << Json::object() << "secondObject"
                                       << JSON( "third_object" << Json::object() ) ) );

    testPullJsonFromLuaObj(
        "local obj = {id = 10, first_object = {}, second_object = { third_object = { fourth_object = {} } } }\n"
        "return obj",
        snakeToCamelCase,
        1,
        JSON( "id" << 10 << "firstObject" << Json::object() << "secondObject"
                   << JSON( "thirdObject" << JSON( "fourth_object" << Json::object() ) ) ) );
}

TEST_F( LuaTestSuite, testPullStringVectorFromEmptyTable ) {
    testPullFromLuaObj( "local obj = {}\n"
                        "return obj",
                        NativeStringVector{} );
}

TEST_F( LuaTestSuite, testPullStringVectorFromNonEmptyTable ) {
    testPullFromLuaObj( R"(
                                     local obj = { "a", "str", "2" }
                                     return obj
                                    )",
                        NativeStringVector{ { "a", "str", "2" } } );
}

TEST_F( LuaTestSuite, testPullStringVectorFromIncorrectLuaTable ) {
    testPullFromInvalidLuaObj< NativeStringVector >( R"(
                                     local obj = { 2 }
                                     return obj
                                    )" );

    testPullFromInvalidLuaObj< NativeStringVector >( R"(
                                     local obj = nil
                                     return obj
                                    )" );
}

TEST_F( LuaTestSuite, testLookupLuaTableSimpleTableEmpty /*3.1.1*/ ) {
    const char* script = "return {}";

    auto result = lookupTable< NativeString >( script, "key" );
    EXPECT_FALSE( result );
}

TEST_F( LuaTestSuite, testLookupLuaTableSimpleTableNotEmpty /*3.1.2*/ ) {
    const char* script = "return { key = \"value\" }";

    auto result = lookupTable< NativeString >( script, "key" );
    ASSERT_TRUE( result );
    EXPECT_EQ( result.value(), "value" );
}

TEST_F( LuaTestSuite, testLookupLuaTableWithMetatableEmpty /*3.2.1*/ ) {
    const char* script = R"(
                                local metatable = { key = "value" }
                                return setmetatable({}, { __index = metatable })
                            )";

    auto result = lookupTable< NativeString >( script, "key" );
    ASSERT_TRUE( result );
    EXPECT_EQ( result.value(), "value" );
}

TEST_F( LuaTestSuite, testLookupLuaTableWithMetatableNotEmpty /*3.2.2*/ ) {
    const char* script = R"(
                                local metatable = { key = "value" }
                                local obj = { key = "new_value" }
                                return setmetatable(obj, { __index = metatable })
                            )";

    auto result = lookupTable< NativeString >( script, "key" );
    ASSERT_TRUE( result );
    EXPECT_EQ( result.value(), "new_value" );
}

TEST_F( LuaTestSuite, testCheckExecuteScriptsFromStream ) {
    auto interpreter = createLuaInterpreter();

    interpreter.loadScript( "HUB:ordinary_require_aware_script" );
    ASSERT_FALSE( interpreter.runLoadedCode().hasError() );
}

TEST_F( LuaTestSuite, testCheckNoThrowingWhehFailToRequirePluginScript ) {
    auto interpreter = createLuaInterpreter();

    interpreter.loadScript( "HUB:try_to_make_failing_require" );
    ASSERT_NO_THROW( interpreter.runLoadedCode() );
}

#define TEST_LUA_INTERPRETER_EXCEPTIONS( callable, callableName, exceptionName, exceptionMsg ) \
    try {                                                                                      \
        callable();                                                                            \
        FAIL() << callableName + " has to throw an exception in case of an invalid argument";  \
    }                                                                                          \
    catch( const exceptionName& error ) {                                                      \
        ASSERT_EQ( std::string( error.what() ), exceptionMsg );                                \
    }                                                                                          \
    catch( ... ) {                                                                             \
        FAIL() << "incorrect type of an exception";                                            \
    }

TEST_F( LuaTestSuite, testCheckThrowExceptionWhenFailToLoadPluginScript ) {
    auto interpreter = createLuaInterpreter();
    const std::string nonexistentFileName = "C:/Program Files";
    const std::string errorMsg = "LuaInterpreter: Couldn't load and/or compile a '" + nonexistentFileName +
                                 +"' file: cannot open " + nonexistentFileName + ": No such file or directory";

    auto body = [&interpreter, &nonexistentFileName]() { interpreter.loadLuaFile( nonexistentFileName ); };

    TEST_LUA_INTERPRETER_EXCEPTIONS( body, std::string{ "interpreter.loadLuaFile" }, LuaInterpreterError, errorMsg );
}

TEST_F( LuaTestSuite, testCheckThrowExceptionWhenFailToLoadLuaString ) {
    auto interpreter = createLuaInterpreter();
    const std::string invalidLuaString = "ifthen";
    const std::string errorMsg = "LuaInterpreter: Couldn't load and/or compile a '" + invalidLuaString +
                                 "' string: [string \"ifthen\"]:1: syntax error near <eof>";

    auto body = [&interpreter, &invalidLuaString]() { interpreter.loadLuaString( invalidLuaString ); };

    TEST_LUA_INTERPRETER_EXCEPTIONS( body, std::string{ "interpreter.loadLuaString" }, LuaInterpreterError, errorMsg );
}

TEST_F( LuaTestSuite, testCheckThrowExceptionWhenFailToLoadStandaloneScript ) {
    auto interpreter = createLuaInterpreter();

    const std::string incorrectScript = "HUB:trash";
    const std::string errorMsg = "LuaModuleLoader: Couldn't find a plugin's script '" + incorrectScript + "'";

    auto body = [&interpreter, &incorrectScript]() { interpreter.loadScript( incorrectScript ); };

    TEST_LUA_INTERPRETER_EXCEPTIONS( body, std::string{ "interpreter.loadScript" }, LuaModuleLoaderError, errorMsg );

    {
        const std::string incorrectScript = "trash";
        const std::string errorMsg =
            "LuaModuleLoader: Script '" + incorrectScript + "' requested to load isn't a plugin's script";

        auto body = [&interpreter, &incorrectScript]() { interpreter.loadScript( incorrectScript ); };

        TEST_LUA_INTERPRETER_EXCEPTIONS(
            body, std::string{ "interpreter.loadScript" }, LuaModuleLoaderError, errorMsg );
    }
}

TEST_F( LuaTestSuite, testCustomRequireOnInvalidSharedModule ) /* 1.6 */ {
    auto interpreter = createLuaInterpreter();

    const std::string script = "HUB:dynamic_module_that_does_not_exist";

    ASSERT_NO_THROW( interpreter.loadScript( script ) );
    try {
        ScriptResult result = interpreter.runLoadedCode();

        ASSERT_FALSE( result.hasError() ) << "Should not produce errors";
        ASSERT_EQ( result.getValuesCount(), 1 );
        ASSERT_EQ( result.getValue( 0 ), Json( nullptr ) ) << "Lua script should return nil if module does not exist";
    }
    catch( const std::exception& error ) {
        FAIL() << std::string{ "LuaInterpreter has thrown an exception: " } + error.what();
    }
}

TEST_F( LuaTestSuite, sendProgressStatus ) {
    LuaAddon::StatusData statusData{ "start", "_ID_", 12 };
    getLuaAddon().sendProgressStatus( statusData );
    auto message = getTestBus().getMessage();
    auto data = statusData.toJson();
    json_add_null( data, "error" );
    EXPECT_EQ( message.data.dump(), data.dump() );

    statusData = { "finished", "_ID_", 100, true };
    getLuaAddon().sendProgressStatus( statusData );
    message = getTestBus().getMessage();
    data = statusData.toJson();
    json_add_null( data, "error" );
    EXPECT_EQ( message.data.dump(), data.dump() );

    statusData = { "failed", "11111", 0, true };
    getLuaAddon().sendProgressStatus( statusData );
    message = getTestBus().getMessage();
    data = statusData.toJson();
    json_add_null( data, "error" );
    EXPECT_EQ( message.data.dump(), data.dump() );

    statusData = { "start", "_ID_", 12, false };
    getLuaAddon().sendProgressStatus( statusData );
    message = getTestBus().getMessage();
    data = statusData.toJson();
    json_add_null( data, "error" );
    EXPECT_NE( message.data.dump(), data.dump() );
}

TEST_F( LuaTestSuite, checkMarkLuaStateAsRunningAnonymousPlugins ) { /* 4.1 */
    auto& luaInterpreter = getLuaInterpreter();
    luaInterpreter.loadScript( "anonymous:a" );
    EXPECT_TRUE( getLuaModuleLoader().isRunningAnonymousPlugin() );
}

TEST_F( LuaTestSuite, checkSetNoAsyncAPIRestrictionRunningAnonymousPlugins ) { /* 4.2 */
    auto& luaInterpreter = getLuaInterpreter();
    luaInterpreter.loadScript( "anonymous:a" );
    EXPECT_EQ( getLuaModuleLoader().getLuaModuleAPIRestrictions(), HubAPIRestrictions::NO_ASYNC_RESTRICTION );
}

TEST_F( LuaTestSuite, checkSetNoRestrictionsAPIRestrictionRunningNotAnonymousPlugins ) { /* 4.3 */
    auto& luaInterpreter = getLuaInterpreter();
    luaInterpreter.loadScript( "HUB:a" );
    EXPECT_EQ( getLuaModuleLoader().getLuaModuleAPIRestrictions(), HubAPIRestrictions::NO_RESTRICTIONS );
}

TEST_F( LuaTestSuite, checkThrowingExceptionWhenFailedToLoadAnonymousPlugin ) { /* 4.4 */
    auto& luaInterpreter = getLuaInterpreter();
    EXPECT_THROW( luaInterpreter.loadScript( "trash:trash" ), LuaModuleLoaderError );
}

TEST_F( LuaTestSuite, snakeToCamelCase ) {
    EXPECT_EQ( snakeToCamelCase( "" ), "" );
    EXPECT_EQ( snakeToCamelCase( "_a" ), "A" );
    EXPECT_EQ( snakeToCamelCase( "long_snake_case_variable" ), "longSnakeCaseVariable" );
    EXPECT_EQ( snakeToCamelCase( "a_b_c_d" ), "aBCD" );

    EXPECT_EQ( snakeToCamelCase( "_" ), "" );
    EXPECT_EQ( snakeToCamelCase( "Word_" ), "Word" );
}

TEST_F( LuaTestSuite, camelToSnakeCase ) {
    EXPECT_EQ( camelToSnakeCase( "" ), "" );
    EXPECT_EQ( camelToSnakeCase( "abraCadabra" ), "abra_cadabra" );
    EXPECT_EQ( camelToSnakeCase( "longCamelCaseWord" ), "long_camel_case_word" );
    EXPECT_EQ( camelToSnakeCase( "ABCD" ), "_a_b_c_d" );

    EXPECT_EQ( camelToSnakeCase( "_a_b_c_d" ), "_a_b_c_d" );
    EXPECT_EQ( camelToSnakeCase( "_A_B_C_D" ), "__a__b__c__d" );
}

TEST_F( LuaTestSuite, mappingKeyConverter ) {
    const KeyMap map{ { "first_word", "mapped_first_word" } };

    {
        auto formatter = createMappingKeyFormatter( map, snakeToCamelCase );

        EXPECT_EQ( formatter( "first_word" ), "mapped_first_word" );
        EXPECT_EQ( formatter( "second_word" ), "secondWord" );
    }

    {
        auto formatter = createMappingKeyFormatter( map );

        EXPECT_EQ( formatter( "first_word" ), "mapped_first_word" );
        EXPECT_EQ( formatter( "second_word" ), "second_word" );
    }
}

TEST_F( LuaTestSuite, testRunScriptTimeoutError ) {
    testPullJsonFromLuaObjWithErrorAndTimeout( "while true do\n"
                                               "end\n",
                                               "Script execution stopped due to timeout limitation!",
                                               1 );
}

TEST_F( LuaTestSuite, testRunScriptTimeout ) {
    initTimeoutSettings();

    int minTimeout = 20;
    int defaultTimeout = 50;

    setTimeoutSettings( minTimeout, defaultTimeout );
    checkTimeout( 50 );

    minTimeout = 120;
    defaultTimeout = 50;
    setTimeoutSettings( minTimeout, defaultTimeout );
    checkTimeout( 120 );

    minTimeout = 120;
    defaultTimeout = -1;
    setTimeoutSettings( minTimeout, defaultTimeout );
    checkTimeout( 0 );
}

class MemoryLeakLuadTest : public LuaTestSuite {
public:
    void pullJsonFromLuaObjVerbose( const char* script,
                                    const Json& params,
                                    const Json& json,
                                    bool handleKeyTypes,
                                    Json* retVal );
    void pullJsonFromLuaObjVerboseReuseInterpreter( LuaInterpreter& interpreter,
                                                    const char* script,
                                                    const Json& params,
                                                    const Json& json,
                                                    bool handleKeyTypes,
                                                    Json* retVal );

    void printHeapUsage( const std::string& message, const Json& conditions, int codeLine );
    static JsonObject makeProcessStatusCondition( const std::string& metric,
                                                  int64_t minValueKb,
                                                  int64_t maxValueKb,
                                                  const std::string& label );

protected:
    using ProcessStatus = SystemUtils::ProcFS::MemoryInfo::ProcessStatus;
    ProcessStatus m_processStatus;
    bool m_callStatus = false;
};

/** Fetches process status record. Saves it in a class member. Prints memory usage.
 *
 * Optionally, checks memory conditions.
 * If conditions are passed, checks memory usage in boundaries [min, max]. default values are -1.
 * If condition is empty, no checks are made.
 *
 * @params message - lifetime phase of a process to which memory usage relates to.
 * @params conditions - json object with process memory usage limits: RssAnon: { min: number, max: number }. */
void MemoryLeakLuadTest::printHeapUsage( const std::string& message, const Json& conditions, int codeLine ) {
    SystemUtils::ProcFS::MemoryInfo memoryInfo;
    m_callStatus = false;
    std::tie( m_callStatus, m_processStatus ) = memoryInfo.fetchProcessMemoryUsage( "self" );

    LOGGER_INFO( "Process status: " + message + ": " + cc::num10( m_processStatus.rssAnon ) );
    if( !conditions.empty() ) {
        Json condition;
        if( json_get( conditions, "RssAnon", condition ) ) {
            auto heapAllocatedMin = json_get_int( condition, "min", -1 );
            auto heapAllocatedMax = json_get_int( condition, "max", -1 );
            auto label = json_get_str( condition, "label" );

            if( heapAllocatedMin > -1 && m_callStatus ) {
                EXPECT_GE( m_processStatus.rssAnon, heapAllocatedMin )
                    << label << "; at: " << __FILE__ << ":" << codeLine;
                ;
            }
            if( heapAllocatedMax > -1 && m_callStatus ) {
                EXPECT_LE( m_processStatus.rssAnon, heapAllocatedMax )
                    << label << "; at: " << __FILE__ << ":" << codeLine;
                ;
            }
        }
    }
}

void MemoryLeakLuadTest::pullJsonFromLuaObjVerbose( const char* script,
                                                    const Json& params,
                                                    const Json& json,
                                                    bool handleKeyTypes,
                                                    Json* retVal ) {
    printHeapUsage( "before lua intp ctor", {}, __LINE__ );

    auto luaInterp = createLuaInterpreter();

    return pullJsonFromLuaObjVerboseReuseInterpreter( luaInterp, script, params, json, handleKeyTypes, retVal );
}

void MemoryLeakLuadTest::pullJsonFromLuaObjVerboseReuseInterpreter( LuaInterpreter& interpreter,
                                                                    const char* script,
                                                                    const Json& params,
                                                                    const Json& json,
                                                                    bool handleKeyTypes,
                                                                    Json* retVal ) {
    printHeapUsage( "before script load", {}, __LINE__ );
    interpreter.loadLuaString( script );

    printHeapUsage( "before runLoadedCode", {}, __LINE__ );
    ProcessStatus beforeRunLuaCode = m_processStatus;
    ScriptResult scriptResults = interpreter.runLoadedCode( &params );

    ASSERT_FALSE( scriptResults.hasError() );

    printHeapUsage( "before pullJson", {}, __LINE__ );
    auto& loaded = scriptResults.getValue( 0 );

    printHeapUsage( "before moving result", {}, __LINE__ );
    if( !json.empty() ) {
        EXPECT_EQ( json, loaded );
    }
    // reorder and use move
    if( retVal ) {
        *retVal = loaded;
    }

    Log::info( "Lua memory used: " + cc::num10( interpreter.getMemoryUsed() ) );
    printHeapUsage( "heap allocated in lua machine plus retval", {}, __LINE__ );
    Log::info( "collect garbage:" );
    interpreter.collectGarbage();
    printHeapUsage( "after garbage collected", {}, __LINE__ );
}

JsonObject MemoryLeakLuadTest::makeProcessStatusCondition( const std::string& metric,
                                                           int64_t minValueKb,
                                                           int64_t maxValueKb,
                                                           const std::string& label ) {
    return JSON( metric << JSON( "min" << minValueKb << "max" << maxValueKb ) << "label" << label );
}

namespace Resources {

const char luaScriptGenerateAndReturnHugeResult[] = R"(
local function generateZeroString(length)
    return string.rep('0', length)
end
local params = ... or { string_limit = 10000000 }
local obj = {str = generateZeroString(params.string_limit)}
return obj
)";
}

TEST_F( MemoryLeakLuadTest, returnedResults_pullJson_hugePayload ) {
    printHeapUsage( "test: heap allocated (before)", {}, __LINE__ );
    ProcessStatus memoryUsedStart = m_processStatus;
    const bool checkJson = false;
    Json result;

    auto luaProgram = Resources::luaScriptGenerateAndReturnHugeResult;

    JsonObject conditions = makeProcessStatusCondition( "RssAnon",
                                                        memoryUsedStart.rssAnon * 0.8,  // sometimes it goes down
                                                        memoryUsedStart.rssAnon * 2,
                                                        "Return values should be popped out from stack" );

    Json params = { { "string_limit", 30000000 } };

    for( int i = 0; i < 3; i++ ) {
        Log::info( "Attempt: " + cc::num10( i ) );
        Log::info( "Malloc stats 0:" );
        malloc_stats();

        pullJsonFromLuaObjVerbose( luaProgram, params, JSON_EMPTY_OBJ, false, nullptr );

        printHeapUsage( "test: after pullJson", {}, __LINE__ );

        // glibc from amd64 glibc toolchain:
        {
            size_t pad = 100 * 1024;
            malloc_trim( pad );

            printHeapUsage( "test: after malloc_trim", conditions, __LINE__ );

            Log::info( "Print malloc stats 3 to stderr:" );
            malloc_stats();
        }
    }
}

TEST_F( MemoryLeakLuadTest, returnedResults_pullJson_hugePayload_luaRestoreState ) {
    printHeapUsage( "test: heap allocated (before)", {}, __LINE__ );
    ProcessStatus memoryUsedStart = m_processStatus;

    const bool checkJson = false;

    auto luaProgram = Resources::luaScriptGenerateAndReturnHugeResult;

    auto luaInterp = createLuaInterpreter();

    JsonObject conditions = makeProcessStatusCondition( "RssAnon",
                                                        memoryUsedStart.rssAnon * 0.8,
                                                        memoryUsedStart.rssAnon * 2,
                                                        "Return values should be popped out from stack" );

    Json params = { { "string_limit", 30000000 } };

    for( int i = 0; i < 3; i++ ) {
        Log::info( "Attempt: " + cc::num10( i ) );
        Log::info( "Malloc stats 0:" );
        malloc_stats();

        pullJsonFromLuaObjVerboseReuseInterpreter( luaInterp, luaProgram, params, JSON_EMPTY_OBJ, false, nullptr );

        Log::info( "Malloc stats 1:" );
        malloc_stats();

        // glibc from amd64 glibc toolchain:
        {
            const size_t pad = 100 * 1024;
            malloc_trim( pad );
            printHeapUsage( "test: after exit from testPullJsonFromLuaObjVerbose", conditions, __LINE__ );

            Log::info( "Print malloc stats 3 to stderr:" );
            malloc_stats();
        }
    }

    printHeapUsage( "test: after exit from testPullJsonFromLuaObjVerbose", conditions, __LINE__ );

    // clean-up: remove interpreter
    luaInterp = createLuaInterpreter();
    json_add_str( conditions, "label", "Memory should be freed after interpreter destroyed" );
    printHeapUsage( "test: after deleting lua interpreter", conditions, __LINE__ );
}

}  // namespace UT
}  // namespace Luad
}  // namespace HubPlatform
