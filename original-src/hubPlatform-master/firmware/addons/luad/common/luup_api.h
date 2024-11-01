#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <string>
#include <vector>
#include <map>

namespace HubPlatform {

class LuupAPI {
public:
    void registerModule( const std::string& moduleName, const luaL_Reg* functionsSet );

    /* @brief registers all existent Luup API modules for a specified @c luaState lua_State
     *
     * @param [in] luaState - a lua_State modules have to be registered for
     */
    static void registerOldAPIModules( lua_State* luaState );

protected:
    using NativeStringTable = std::map< std::string, std::string >;

    /* @brief converts a Lua table (string->string) to a native C++ table (string->string)
     *
     * @param [in] luaState - a lua env, a Lua table is placed in
     * @param [in] luaTableIdx - an index of a Lua table on the stack
     * @return a C++ representation of a Lua table
     */
    static NativeStringTable getNativeStringTable( lua_State* luaState, int luaTableIdx );

    /* @brief returns a string representation of an errno
     *
     * @note this function returns a string representation of a LAST errno, so keep it in mind
     */
    static std::string getLastErrnoDescription();

private:
    /* @brief represents an Luup API module (Luup API consists of several modules: log, io, inet,...)
     * moduleName - a name of the module
     * functionsSet - a set of functions the module provides
     */
    struct LuupAPIModule final {
        LuupAPIModule( const std::string& moduleName, const luaL_Reg* functionsSet )
        : m_moduleName( moduleName )
        , m_functionsSet( functionsSet ) {}

        std::string m_moduleName;
        const luaL_Reg* m_functionsSet;
    };

    using LuupAPIModules = std::vector< LuupAPIModule >;

    static LuupAPIModules m_APIModules;
};

}  // namespace HubPlatform
