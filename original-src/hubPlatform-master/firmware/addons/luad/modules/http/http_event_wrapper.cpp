#include "luad/modules/http/http_event_wrapper.h"
#include "http/http_events.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Luad {

namespace {

std::string formatKey( const KeyFormatter& keyFormatter, std::string str ) {
    return keyFormatter ? keyFormatter( std::move( str ) ) : std::move( str );
}

}  // namespace

class LuaPushVisitor : public CommonHttp::EventVisitor {
public:
    LuaPushVisitor( lua_State& luaState, HttpEventWrapper& wrapper );

public:
    void visit( CommonHttp::DataSendEvent& value ) override;
    void visit( CommonHttp::DataReceivedEvent& value ) override;
    void visit( CommonHttp::ConnectionClosedEvent& value ) override;

    int exitCode() const {
        return m_exitCode;
    }

private:
    lua_State& m_luaState;
    HttpEventWrapper& m_eventWrapper;
    int m_exitCode = 0;
};

LuaPushVisitor::LuaPushVisitor( lua_State& luaState, HttpEventWrapper& eventWrapper )
: m_luaState( luaState )
, m_eventWrapper( eventWrapper ) {}

void LuaPushVisitor::visit( CommonHttp::DataSendEvent& value ) {
    m_eventWrapper.pushAsObject( m_luaState, value );
}

void LuaPushVisitor::visit( CommonHttp::DataReceivedEvent& value ) {
    m_eventWrapper.pushAsObject( m_luaState, value );
}

void LuaPushVisitor::visit( CommonHttp::ConnectionClosedEvent& value ) {
    m_eventWrapper.pushAsObject( m_luaState, value );
}

HttpEventWrapper::HttpEventWrapper( EventPtr ptr, KeyFormatter keyFormatter )
: m_event( std::move( ptr ) )
, m_keyFormatter( std::move( keyFormatter ) ) {}

const HttpEventWrapper::EventPtr& HttpEventWrapper::event() const {
    return m_event;
}

int HttpEventWrapper::objectTemplate( Json injectKeys, const std::string& key ) {
    m_objectTemplate = std::move( injectKeys );
    m_key = key;

    return 0;
}

int HttpEventWrapper::pushAsObject( lua_State& luaState ) const {
    if( m_event.get() ) {
        LuaPushVisitor visitor( luaState, const_cast< HttpEventWrapper& >( *this ) );
        m_event->accept( visitor );

        return visitor.exitCode();
    }
    return 0;  // TODO: check if 0 is Ok.
}

std::string HttpEventWrapper::toString() const {
    if( !m_event.get() )
        return std::string();
    if( m_objectTemplate.is_object() && m_objectTemplate != nullptr && !m_key.empty() ) {
        // Push as a part of another object
        Json builder = m_objectTemplate;
        builder[ m_key ] = m_event->toJson();

        return builder.dump();
    }

    return m_event->toJson().dump();
}

int HttpEventWrapper::pushAsObjectDefault( lua_State& luaState, CommonHttp::Event& event ) const {
    if( m_objectTemplate.is_object() && m_objectTemplate != nullptr && !m_key.empty() ) {
        // Push as a part of another object
        Json builder = m_objectTemplate;
        builder[ m_key ] = event.toJson();

        LOGGER_TRACE( cc::trace( "HttpEventWrapper::pushAsObject(Event&) / builder: " ) + cc::j( builder ) );
        return pushJsonAsLuaObj( &luaState, builder );
    }

    return pushJsonAsLuaObj( &luaState, event.toJson() );
}

// Define FOR DEBUG PURPOSES ONLY
//# define USE_JSON_RECEIVE_DATA_EVENT

int HttpEventWrapper::pushAsObject( lua_State& luaState, CommonHttp::DataReceivedEvent& event ) const {
#ifdef USE_JSON_RECEIVE_DATA_EVENT
    return pushAsObjectDefault( luaState, event );
#else
    bool hasTemplate = m_objectTemplate.is_object() && m_objectTemplate != nullptr && !m_key.empty();
    if( hasTemplate ) {
        Json& json = const_cast< Json& >( m_objectTemplate );
        // start object template
        lua_createtable( &luaState, 0, json.size() );
        for( auto it = json.begin(); it != json.end(); ++it ) {
            pushJsonAsLuaObj( &luaState, it.value() );
            lua_setfield( &luaState, -2, formatKey( m_keyFormatter, it.key() ).c_str() );
        }
    }

    int exitCode = pushEventFields( luaState, event );

    if( hasTemplate ) {
        lua_setfield( &luaState, -2, formatKey( m_keyFormatter, m_key ).c_str() );
    }

    return exitCode;
#endif
}

/** Pushes http_data_received event to a Lua stack field by field. The event can be huge.
 * Avoid calling event.toJson() in the method. */
int HttpEventWrapper::pushEventFields( lua_State& luaState, CommonHttp::DataReceivedEvent& event ) const {
    LOGGER_TRACE( "HttpEventWrapper::pushAsObject(DataReceivedEvent&) called" );

    // Optional fields are: user_data, headers
    const int requiredFields = 5;  // id, url, code, last, data
    bool areHeadersEmpty = event.headers().empty();
    bool isUserDataEmpty = event.userData().empty();

    int numFields = requiredFields + ( areHeadersEmpty ? 0 : 1 ) + ( isUserDataEmpty ? 0 : 1 ) +
                    ( event.multipleHeaders() ? 1 : 0 );

    lua_createtable( &luaState, 0, numFields );
    // common fields: id, url, userData: string
    lua_pushstring( &luaState, event.id().c_str() );
    lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::id ).c_str() );

    lua_pushstring( &luaState, event.url().c_str() );
    lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::url ).c_str() );

    if( !isUserDataEmpty ) {
        pushJsonAsLuaObj( &luaState, event.userData() );
        lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::userData ).c_str() );
    }

    // data_received fields: code: int, data: string|binary, last: bool, headers: []
    lua_pushnumber( &luaState, event.code() );
    lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::code ).c_str() );

    lua_pushboolean( &luaState, event.last() );
    lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::last ).c_str() );

    lua_pushlstring( &luaState, event.data().data(), event.data().size() );
    lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::data ).c_str() );

    lua_pushboolean( &luaState, event.multipleHeaders() );
    lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::multipleHeaders ).c_str() );

    if( !areHeadersEmpty ) {
        Json headersFormattedJson;
        event.serializeHeaders( headersFormattedJson );
        pushJsonAsLuaObj( &luaState, headersFormattedJson );
        lua_setfield( &luaState, -2, formatKey( m_keyFormatter, CommonHttp::Fields::headers ).c_str() );
    }

    // NOTE: ! Update numFields if you are adding something here!

    return 1;
}

int HttpEventWrapper::pushAsObject( lua_State& luaState, CommonHttp::DataSendEvent& event ) const {
    return pushAsObjectDefault( luaState, event );
}

int HttpEventWrapper::pushAsObject( lua_State& luaState, CommonHttp::ConnectionClosedEvent& event ) const {
    return pushAsObjectDefault( luaState, event );
}

LuaScriptParamsHolderPtr HttpEventWrapper::make( HttpEventWrapper::EventPtr params, KeyFormatter keyFormatter ) {
    auto ptr = std::make_unique< HttpEventWrapper >( std::move( params ), std::move( keyFormatter ) );
    return LuaScriptParamsHolderPtr{ ptr.release(), ptr.get_deleter() };
}

unsigned HttpEventWrapper::payloadSize() const {
    if( m_event.get() ) {
        if( auto* dataReceived = dynamic_cast< CommonHttp::DataReceivedEvent* >( m_event.get() ) ) {
            return dataReceived->data().size();
        }
    }
    return 0;
}

}  // namespace Luad
}  // namespace HubPlatform