#include "luad/addon/lua_task.h"
#include "luad/addon/lua_addon.h"

#include "luad/interpreter/lua_script_params_holder.h"
#include <chrono>
#include <utility>

namespace HubPlatform {
namespace LuaAddon {

LuaTaskExecuter::LuaTaskExecuter( LuaAddon& addon,
                                  LuaScriptParamsHolderPtr paramsHolder,
                                  std::chrono::steady_clock::duration timeout )
: m_luaInterpreter( std::make_shared< LuaInterpreter >( addon, addon, timeout ) )
, m_paramsHolder( std::move( paramsHolder ) ) {}

LuaTaskExecuter::LuaTaskExecuter( std::shared_ptr< LuaInterpreter > luaInterpreter,
                                  LuaScriptParamsHolderPtr paramsHolder )
: m_luaInterpreter( std::move( luaInterpreter ) )
, m_paramsHolder( std::move( paramsHolder ) ) {}

ScriptResult LuaTaskExecuter::operator()() {
    return runTask();
}

ScriptResult LuaTaskExecuter::runTask() {
    return m_luaInterpreter->runLoadedCode( m_paramsHolder );
}

std::string LuaTaskExecuter::getExecutionState() {
    return m_luaInterpreter->getExecutionState();
}

LuaTaskExecuteFile::LuaTaskExecuteFile( LuaAddon& addon,
                                        const std::string& luaFilePath,
                                        LuaScriptParamsHolderPtr paramsHolder,
                                        std::chrono::steady_clock::duration timeout )
: LuaTaskExecuter( addon, std::move( paramsHolder ), timeout ) {
    m_luaInterpreter->loadLuaFile( luaFilePath );
}

LuaTaskExecuteString::LuaTaskExecuteString( LuaAddon& addon,
                                            const std::string& luaString,
                                            LuaScriptParamsHolderPtr paramsHolder,
                                            std::chrono::steady_clock::duration timeout )
: LuaTaskExecuter( addon, std::move( paramsHolder ), timeout ) {
    m_luaInterpreter->loadLuaString( luaString );
}

LuaTaskExecuteScript::LuaTaskExecuteScript( std::shared_ptr< LuaInterpreter > interpreter,
                                            const std::string& script,
                                            LuaScriptParamsHolderPtr paramsHolder )
: LuaTaskExecuter( std::move( interpreter ), std::move( paramsHolder ) ) {
    m_luaInterpreter->loadScript( script );
}

}  // namespace LuaAddon
}  // namespace HubPlatform