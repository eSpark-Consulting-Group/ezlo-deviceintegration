#pragma once
#include "http/http_events.h"
#include "luad/addon/lua_task.h"
#include "binding_interfaces/lua/lua_binding_interface.h"

struct lua_State;

namespace HubPlatform {
namespace Luad {

class HttpEventWrapper : public LuaScriptParamsHolder {
public:
    using EventPtr = CommonHttp::EventPtr;

public:
    HttpEventWrapper( EventPtr ptr, KeyFormatter keyFormatter = nullptr );
    const EventPtr& event() const;

    int objectTemplate( Json injectKeys, const std::string& key ) override;
    int pushAsObject( lua_State& luaState ) const override;
    std::string toString() const override;
    unsigned payloadSize() const override;

    static LuaScriptParamsHolderPtr make( EventPtr params, KeyFormatter keyFormatter = nullptr );

public:
    int pushAsObject( lua_State& luaState, CommonHttp::DataSendEvent& event ) const;
    int pushAsObject( lua_State& luaState, CommonHttp::DataReceivedEvent& event ) const;
    int pushAsObject( lua_State& luaState, CommonHttp::ConnectionClosedEvent& event ) const;

protected:
    int pushAsObjectDefault( lua_State& luaState, CommonHttp::Event& event ) const;
    int pushEventFields( lua_State& luaState, CommonHttp::DataReceivedEvent& event ) const;

private:
    EventPtr m_event;
    /** In case m_event should be pushed as a part of another object as a value of
     *  { ...<objectTemplate>, "<m_key>": event->pushAsObject() }
     */
    std::string m_key;
    Json m_objectTemplate;
    KeyFormatter m_keyFormatter = nullptr;
};

}  // namespace Luad
}  // namespace HubPlatform