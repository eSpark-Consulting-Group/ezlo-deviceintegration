#pragma once

#include "base_utils/non_copyable.h"
#include "luad/interpreter/lua_script_params_holder.h"

namespace HubPlatform {
namespace LuaAddon {

class EventEmitter : private common::NonCopyable {
public:
    virtual ~EventEmitter() = default;

    virtual void localEmit( const std::string& eventHandler, const std::string& eventType, const Json& eventData ) = 0;
    virtual void localEmit( const std::string& eventHandler, const Json& eventData ) = 0;

    virtual void localEmit( const std::string& eventHandler,
                            const std::string& eventType,
                            LuaScriptParamsHolderPtr scriptParams ) = 0;
    virtual void localEmit( const std::string& eventHandler, LuaScriptParamsHolderPtr scriptParams ) = 0;
};

}  // namespace LuaAddon
}  // namespace HubPlatform
