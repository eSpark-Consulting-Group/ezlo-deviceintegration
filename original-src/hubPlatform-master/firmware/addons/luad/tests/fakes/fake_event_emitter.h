#pragma once

#include "luad/addon/lua_event_emitter.h"

#include <exception>

namespace HubPlatform {
namespace Luad {
namespace UT {

class FakeEventEmitter : public LuaAddon::EventEmitter {
public:
    using EmitHandler = std::function< void( const std::string&, const Json& ) >;

public:
    void localEmit( const std::string& eventHandler, const std::string& eventType, const Json& eventData ) override {
        throw std::logic_error( "Unexpected call" );
    }

    void localEmit( const std::string& eventHandler, const Json& eventData ) override {
        if( !m_emitHandler )
            return;

        m_emitHandler( eventHandler, eventData );
    }

    void localEmit( const std::string&, const std::string&, HubPlatform::LuaScriptParamsHolderPtr ) override {
        throw std::logic_error( "Unexpected call" );
    }
    void localEmit( const std::string&, HubPlatform::LuaScriptParamsHolderPtr ) override {
        throw std::logic_error( "Unexpected call" );
    }

    void setEmitHandler( EmitHandler _callback ) {
        m_emitHandler = std::move( _callback );
    }

    void setEmitHandler( std::function< void() > _callback ) {
        m_emitHandler = std::bind( std::move( _callback ) );
    }

private:
    EmitHandler m_emitHandler;
};

}  // namespace UT
}  // namespace Luad
}  // namespace HubPlatform
