#include "luad/modules/http/http_dispatch.h"
#include "luad/modules/http/http_event_wrapper.h"
#include "luad/addon/lua_event_emitter.h"
#include "luad/interpreter/module_loader/lua_module_loader.h"

#include "binding_interfaces/common/plugin_status_events_subscription.h"

#include <curl/curl.h>
#include <openssl/ssl.h>
#include <sys/stat.h>

namespace HubPlatform {
namespace Luad {

namespace {
void globalInit() {
    LOGGER_TRACE_RAW( cc::trace( "http module globalInit()" ) );
    ::SSL_library_init();
    ::SSL_load_error_strings();
    ::ERR_load_BIO_strings();
    ::OpenSSL_add_all_algorithms();
    ::OpenSSL_add_all_digests();
    curl_global_init( CURL_GLOBAL_DEFAULT );
}
}  // namespace

HttpDispatch::HttpDispatch( LuaPluginResourcesProvider& luaPluginResourceProvider,
                            LuaAddon::EventEmitter& eventsEmitter,
                            IPluginStatusEventsSubscription& pluginStatusEventsSubscription )
: m_luaPluginResourceProvider( luaPluginResourceProvider )
, m_eventsEmitter( eventsEmitter ) {
    globalInit();
    using EventHandler = HubPlatform::CommonHttp::Http::EventHandler;
    using namespace CommonHttp::Resources;

    auto onDataReceived = [this]( const std::string& handler, const EventPtr& event ) {
        m_eventsEmitter.localEmit( handler, HttpEvents::HttpDataReceived, HttpEventWrapper::make( event ) );
    };
    auto onDataSend = [this]( const std::string& handler, const EventPtr& event ) {
        m_eventsEmitter.localEmit( handler, HttpEvents::HttpDataSend, LuaScriptJsonParams::make( event->toJson() ) );
    };
    auto onConnectionClosed = [this]( const std::string& handler, const EventPtr& event ) {
        m_eventsEmitter.localEmit(
            handler, HttpEvents::HttpConnectionClosed, LuaScriptJsonParams::make( event->toJson() ) );
    };

    m_http.on( HttpEvents::HttpDataReceived, std::move( onDataReceived ) );
    m_http.on( HttpEvents::HttpDataSend, std::move( onDataSend ) );
    m_http.on( HttpEvents::HttpConnectionClosed, std::move( onConnectionClosed ) );

    m_http.exitDelayInterval( 90 );  // set exit delay interval to avoid exit on every http request end

    pluginStatusEventsSubscription.subscribeOnPluginRestarted(
        [this]( const std::string& pluginId ) { closePluginConnections( pluginId ); } );
    pluginStatusEventsSubscription.subscribeOnPluginTeardown(
        [this]( const std::string& pluginId ) { closePluginConnections( pluginId ); } );
    pluginStatusEventsSubscription.subscribeOnPluginUninstall(
        [this]( const std::string& pluginId ) { closePluginConnections( pluginId ); } );
}

HttpDispatch::~HttpDispatch() {
    LOGGER_TRACE_RAW( cc::trace( "HttpDispatch: Stopping HTTP-server..." ) );
    m_http.stop();
    if( m_http.joinable() ) {
        m_http.join();
        LOGGER_TRACE_RAW( cc::trace( "HttpDispatch: Exit from join: " ) );
    }
}

void HttpDispatch::closePluginConnections( const std::string& pluginId ) {
    try {
        m_http.closePluginConnections( pluginId );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to free http resources for plugin " ) + cc::note( pluginId ) +
                      cc::error( ", reason: " ) + cc::note( ex.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Failed to free http resources for plugin " ) + cc::note( pluginId ) );
    }
}

std::string HttpDispatch::request( const std::string& pluginId, const Json& request ) {
    HttpOptions options;
    try {
        from_json( request, options );
        if( options.verbose )
            LOGGER_TRACE( cc::trace( "http.request called: " ) + cc::j( request ) );
    }
    catch( const std::exception& e ) {
        Log::error( cc::error( "Parsing error: " ) + cc::error( e.what() ) + " on request: " + cc::j( request ) );
        throw e;
    }

    checkScriptFileAndMakeFullPath( pluginId, options.handler );  // do not rewrite handler with the full path

    options.certs.ca = checkDataFileAndMakeFullPath( pluginId, options.certs.ca );
    options.certs.privateKey = checkDataFileAndMakeFullPath( pluginId, options.certs.privateKey );
    options.certs.publicKey = checkDataFileAndMakeFullPath( pluginId, options.certs.publicKey );

    return m_http.request( pluginId, options );
}

void HttpDispatch::repeatRequest( const std::string& pluginId, const Json& request ) {
    Http::RequestId connectionId = json_get_str( request, "id" );
    HttpOptions options;
    try {
        from_json( request, options );
        if( options.verbose )
            LOGGER_TRACE( cc::trace( "http.repeatRequest called: " ) + cc::j( request ) );
    }
    catch( const std::exception& e ) {
        Log::error( cc::error( "Parsing error" ) + cc::j( request ) );
        throw e;
    }

    checkScriptFileAndMakeFullPath( pluginId, options.handler );

    m_http.repeatRequest( pluginId, connectionId, options );
}

bool HttpDispatch::closeConnection( const std::string& pluginId, const std::string& connectionId ) {
    return m_http.closeConnection( pluginId, connectionId );
}

int HttpDispatch::sendData( const std::string& pluginId, Json request ) {
    if( Log::getLogLevel() >= Log::PriorityLevel::TRACE ) {
        auto data = json_get_str( request, "data" );
        if( data.size() > 500 ) {
            Json tmp = request;
            tmp[ "data" ] = data.substr( 0, 200 ) + " (truncated, size: " + std::to_string( data.size() ) + ")...";
            LOGGER_TRACE_RAW( "http.sendData called: " );
        }
        else {
            LOGGER_TRACE_RAW( "http.sendData called: " + cc::j( request ) );
        }
    }

    std::string id, data;
    bool last = true;
    try {
        id = json_get_str( request, "id" );
        data = json_get_str( request, "data" );
        last = json_get_bool( request, "last", true );
    }
    catch( const std::exception& e ) {
        Log::error( cc::error( "Parsing error" ) + cc::j( request ) );
        throw e;
    }

    if( data.size() >= 16 * 1024 ) {
        request[ "data" ] = "(truncated)";
    }
    return m_http.sendData( pluginId, id, std::move( data ), last );
}

/** throws HttpConnectionDoesntExistError if no connection found */
Json HttpDispatch::getConnectionInfo( const std::string& pluginId, const std::string& connectionId ) {
    const auto& info = m_http.getConnectionInfo( pluginId, connectionId );
    return info.toJson();
}

void HttpDispatch::checkPluginPermissions( const std::string& pluginId, const std::string& script ) {
    if( script.empty() )
        return;

    auto foundPluginId = m_luaPluginResourceProvider.getPluginIdByScriptId( script );

    if( foundPluginId != pluginId ) {
        std::string message = Resources::HTTP_ERROR_PLUGIN_PERMISSIONS;
        throw HttpPluginPermissionsError( message, script );
    }
}

void HttpDispatch::checkFullPathExists( const std::string& script, const std::string& nameForException ) {
    if( script.empty() )
        return;

    struct stat fileStat = {};
    if( -1 == stat( script.c_str(), &fileStat ) ) {
        const auto& toReport = nameForException.empty() ? script : nameForException;
        throw HttpFileDoesntExist( std::string( strerror( errno ) ), toReport );
    }
}

std::string HttpDispatch::makeScriptFullPath( const std::string& script ) {
    return m_luaPluginResourceProvider.getScriptFilePath( script );
}

std::string HttpDispatch::makeDataFileFullPath( const std::string& fileName ) {
    return m_luaPluginResourceProvider.getDataFilePath( fileName );
}

std::string HttpDispatch::checkScriptFileAndMakeFullPath( const std::string& pluginId, const std::string& script ) {
    if( !script.empty() ) {
        checkPluginPermissions( pluginId, script );

        auto fullPath = makeScriptFullPath( script );
        checkFullPathExists( fullPath, script );

        return fullPath;
    }
    return script;
}

std::string HttpDispatch::checkDataFileAndMakeFullPath( const std::string& pluginId, const std::string& fileName ) {
    if( !fileName.empty() ) {
        checkPluginPermissions( pluginId, fileName );

        auto fullPath = makeDataFileFullPath( fileName );
        checkFullPathExists( fullPath, fileName );

        return fullPath;
    }
    return fileName;
}

}  // namespace Luad
}  // namespace HubPlatform
