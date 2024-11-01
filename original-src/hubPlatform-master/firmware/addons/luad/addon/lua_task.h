#pragma once

#include "luad/interpreter/lua_interpreter.h"
#include "luad/interpreter/lua_script_params_holder.h"
#include "base_utils/script_result.h"

#include <string>
#include <chrono>

namespace HubPlatform {
namespace LuaAddon {

class LuaAddon;

class LuaTaskExecuter {
public:
    ScriptResult operator()();
    ScriptResult runTask();
    std::string getExecutionState();

protected:
    LuaTaskExecuter( LuaAddon& addon,
                     LuaScriptParamsHolderPtr paramsHolder,
                     std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::zero() );

    LuaTaskExecuter( std::shared_ptr< LuaInterpreter > luaInterpreter, LuaScriptParamsHolderPtr paramsHolder );

    ~LuaTaskExecuter() = default;

protected:
    std::shared_ptr< LuaInterpreter > m_luaInterpreter;
    LuaScriptParamsHolderPtr m_paramsHolder;
};

class LuaTaskExecuteFile final : public LuaTaskExecuter {
public:
    LuaTaskExecuteFile( LuaAddon& addon,
                        const std::string& luaFilePath,
                        LuaScriptParamsHolderPtr paramsHolder,
                        std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::zero() );
};

class LuaTaskExecuteString final : public LuaTaskExecuter {
public:
    LuaTaskExecuteString( LuaAddon& addon,
                          const std::string& luaString,
                          LuaScriptParamsHolderPtr paramsHolder,
                          std::chrono::steady_clock::duration timeout = std::chrono::steady_clock::duration::zero() );
};

class LuaTaskExecuteScript final : public LuaTaskExecuter {
public:
    LuaTaskExecuteScript( std::shared_ptr< LuaInterpreter > interpreter,
                          const std::string& script,
                          LuaScriptParamsHolderPtr paramsHolder );
};

}  // namespace LuaAddon
}  // namespace HubPlatform
