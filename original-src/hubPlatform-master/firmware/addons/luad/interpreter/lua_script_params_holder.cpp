#include "luad/interpreter/lua_script_params_holder.h"
#include "binding_interfaces/lua/lua_binding_interface.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {

LuaScriptJsonParams::LuaScriptJsonParams( Json params, bool handleKeyTypes )
: m_params( std::move( params ) )
, m_handleKeyTypes( handleKeyTypes ) {}

int LuaScriptJsonParams::objectTemplate( Json injectKeys, const std::string& key ) {
    m_injectKeys = std::move( injectKeys );
    m_key = key;

    return 0;
}

int LuaScriptJsonParams::pushAsObject( lua_State& luaState ) const {
    if( m_injectKeys != nullptr && m_injectKeys.is_object() && !m_key.empty() ) {
        // Push as a part of another object
        Json builder = m_injectKeys;
        builder[ m_key ] = m_params;

        return pushJsonAsLuaObj( &luaState, builder, nullptr, 0, m_handleKeyTypes );
    }
    return pushJsonAsLuaObj( &luaState, m_params, nullptr, 0, m_handleKeyTypes );
}

std::string LuaScriptJsonParams::toString() const {
    if( m_injectKeys != nullptr && m_injectKeys.is_object() && !m_key.empty() ) {
        // Push as a part of another object
        Json builder = m_injectKeys;
        builder[ m_key ] = m_params;

        return builder.dump();
    }
    return m_params.dump();
}

LuaScriptParamsHolderPtr LuaScriptJsonParams::make( Json params, bool handleKeyTypes ) {
    return std::make_unique< LuaScriptJsonParams >( std::move( params ), handleKeyTypes );
}

unsigned LuaScriptJsonParams::payloadSize() const {
    if( !m_params.empty() && m_params.count( "data" ) > 0 ) {
        const auto& dataField = m_params[ "data" ];
        if( dataField.is_string() ) {
            if( const auto dataPtr = dataField.get_ptr< const Json::string_t* >() ) {
                return dataPtr->size();
            }
        }
        else if( dataField.is_object() && dataField.count( "data" ) ) {
            // try case of event { event: name, data: { data: payload } }
            if( const auto dataPtr = dataField[ "data" ].get_ptr< const Json::string_t* >() ) {
                return dataPtr->size();
            }
        }
    }
    return 0;
}

}  // namespace HubPlatform
