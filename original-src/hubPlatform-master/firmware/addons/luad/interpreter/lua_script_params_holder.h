#pragma once

#include <string>
#include <memory>
#include "json_utils/json_fwd.h"

struct lua_State;

namespace HubPlatform {

class LuaScriptParamsHolder {
public:
    LuaScriptParamsHolder() = default;
    virtual ~LuaScriptParamsHolder() = default;

    virtual int objectTemplate( Json injectKeys, const std::string& key ) = 0;
    virtual int pushAsObject( lua_State& luaState ) const = 0;
    virtual std::string toString() const = 0;
    virtual unsigned payloadSize() const = 0;
};
using LuaScriptParamsHolderPtr = std::shared_ptr< LuaScriptParamsHolder >;

class LuaScriptJsonParams : public LuaScriptParamsHolder {
public:
    LuaScriptJsonParams( Json params, bool handleKeyTypes = false );

    int objectTemplate( Json injectKeys, const std::string& key ) override;
    int pushAsObject( lua_State& luaState ) const override;
    std::string toString() const override;
    unsigned payloadSize() const override;

    static LuaScriptParamsHolderPtr make( Json params, bool handleKeyTypes = false );

private:
    Json m_params;
    bool m_handleKeyTypes;
    Json m_injectKeys;
    std::string m_key;
};

}  // namespace HubPlatform
