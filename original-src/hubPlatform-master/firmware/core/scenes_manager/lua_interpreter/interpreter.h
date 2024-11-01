#pragma once

extern "C" {
#include <lua.h>
}
#include "json_utils/json_fwd.h"
#include <string>

namespace HubPlatform {
namespace ScenesExpressions {

class ExpressionsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ScenesLuaInterpreter {
public:
    ScenesLuaInterpreter();
    /** @brief Calls lua script passed as an 'expression' with parameters passed via 'expressionParams' */
    bool protectedCall( std::string expression, Json expressionParams, Json& result );

private:
    std::unique_ptr< lua_State, void ( * )( lua_State* ) > m_luaState;
};

}  // namespace ScenesExpressions
}  // namespace HubPlatform