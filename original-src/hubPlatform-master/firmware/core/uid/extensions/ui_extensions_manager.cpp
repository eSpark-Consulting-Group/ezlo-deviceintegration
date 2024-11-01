#include "uid/extensions/ui_extensions_manager.h"
#include "uid/extensions/cameras/cameras_ui_extension.h"
#include "uid/extensions/http_links/links.h"
#include "uid/extensions/ui_http_extension.h"
#include "uid/extensions/ui_http_utils.h"
#include "uid/handlers/hub_offline_commands_handler.h"
#include "http_bridge/resources.h"
#include "http_bridge/session.h"

#include "uid/ui_service.h"

#include "http_bridge/utils.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Uid {
namespace Extensions {
namespace {

struct HttpSessionInternal : public HttpSession {
    HttpExtension* httpExtension = nullptr;
};

}  // namespace

ExtensionsManager::ExtensionsManager( Service& service, HttpBridge::ApiBridge& httpApiBridge )
: m_service( service )
, m_httpApiBridge( httpApiBridge ) {}

void ExtensionsManager::setUp() {
    registerExtension( std::make_unique< Cameras::UiExtension >( *this, m_service ) );
    registerExtension( std::make_unique< HttpLinks::HttpLinksExtension >( *this, m_service ) );
    m_httpApiBridge.resetAddonsEndpoint( this );
}

ExtensionsManager::~ExtensionsManager() {
    m_httpApiBridge.resetAddonsEndpoint();
}

JsonObject ExtensionsManager::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    JsonObject answer = BusExtension::unhandledRequestAnswer();
    for( auto& extensionsPair : m_extensions ) {
        if( auto busExtensionPtr = extensionsPair.second->getBusExtension() ) {
            answer = busExtensionPtr->handleRequestFromBus( request, requestType );
            if( answer != BusExtension::unhandledRequestAnswer() )
                return answer;
        }
    }
    return answer;
}

void ExtensionsManager::handleMethodCall( const JsonObject& json,
                                          MessageSubClasses requestType,
                                          const std::string& sender ) {
    for( auto& extensionsPair : m_extensions ) {
        if( auto busExtensionPtr = extensionsPair.second->getBusExtension() ) {
            busExtensionPtr->handleMethodCall( json, requestType, sender );
        }
    }
}

void ExtensionsManager::handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) {
    for( auto& extensionsPair : m_extensions ) {
        if( auto busExtensionPtr = extensionsPair.second->getBusExtension() ) {
            busExtensionPtr->handleEventFromBus( event, requestType );
        }
    }
}

void ExtensionsManager::registerExtension( std::unique_ptr< Extension > extension ) {
    if( m_extensions.count( extension->getName() ) != 0 ) {
        return;
    }
    m_extensions.emplace( std::make_pair( extension->getName(), std::move( extension ) ) );
}

void ExtensionsManager::unregisterExtension( const std::string& extensionName ) {
    if( m_extensions.count( extensionName ) == 0 ) {
        return;
    }
    m_extensions.erase( extensionName );
}

void ExtensionsManager::sendMethodCall( const std::string& serviceName,
                                        MessageSubClasses requestType,
                                        const JsonObject& data ) {
    m_service.sendMethodCall( serviceName, requestType, data );
}

JsonObject ExtensionsManager::sendRequest( const std::string& serviceName,
                                           MessageSubClasses requestType,
                                           const JsonObject& request ) {
    return m_service.sendRequest( serviceName, requestType, request );
}

std::tuple< std::string, std::string > ExtensionsManager::extractAddonNameAndEndpoint( const std::string& url ) {
    if( url.empty() || url[ 0 ] != '/' )
        return std::make_tuple( "", url );

    auto pos = url.find( '/', 1 );
    if( pos == std::string::npos )
        return std::make_tuple( "", url );
    return std::make_tuple( url.substr( 1, pos - 1 ), url.substr( pos ) );
}

int ExtensionsManager::endpointCallback( struct lws* wsi,
                                         enum lws_callback_reasons reason,
                                         void* user,
                                         void* in,
                                         size_t len ) {
    auto& sessionPtr = *static_cast< HttpSessionInternal** >( user );
    static lws_usec_t Timeout = 30 * 60 * LWS_USEC_PER_SEC;
    using namespace HttpBridge::Resources;

    switch( reason ) {
    case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
        sessionPtr = new HttpSessionInternal();
        m_httpApiBridge.registerExternalSession();
        m_httpApiBridge.emit( Metrics::eventRequestStarted, EndpointHandlers::UiExtensions, nullptr );
        m_httpApiBridge.emit( Metrics::eventParallelRequests, EndpointHandlers::UiExtensions, nullptr );
        break;
    case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
        if( sessionPtr ) {
            if( sessionPtr->httpExtension ) {
                sessionPtr->httpExtension->onHttpClosed( wsi, *sessionPtr );
            }
            if( !sessionPtr->isClosed() ) {
                m_httpApiBridge.emit( Metrics::eventRequestFinished, EndpointHandlers::UiExtensions, nullptr );
            }
            delete sessionPtr;
            sessionPtr = nullptr;
        }
        m_httpApiBridge.unregisterExternalSession();
        m_httpApiBridge.emit( Metrics::eventParallelRequests, EndpointHandlers::UiExtensions, nullptr );
        break;
    case LWS_CALLBACK_HTTP: {
        lws_set_timer_usecs( wsi, Timeout );
        if( sessionPtr ) {
            if( m_httpApiBridge.sessionCount() >= HttpBridge::ApiBridge::parallelSessionsLimit() ) {
                m_httpApiBridge.emit( Metrics::eventRequestsLimit, EndpointHandlers::UiExtensions, nullptr );
                sessionPtr->closeSession();
                return Utils::writeError( wsi, 429, "Too Many Requests" );
            }

            std::string protocolName = (const char*) ( lws_get_protocol( wsi )->name );
            std::string url = std::string( static_cast< char* >( in ), len );
            constexpr const char addonsProtocolPrefix[] = "addons-";
            if( strncmp( protocolName.c_str(), addonsProtocolPrefix, sizeof( addonsProtocolPrefix ) - 1 ) != 0 ) {
                sessionPtr->addon = protocolName;
                sessionPtr->url = url;
            }
            else {
                // Do this only for protocol names started from "addons-"
                std::tie( sessionPtr->addon, sessionPtr->url ) = extractAddonNameAndEndpoint( url );
            }
            sessionPtr->method = HttpBridge::Utils::extractMethod( wsi );
            auto headers = HttpBridge::Utils::extractHeaders( wsi );
            sessionPtr->headers.insert( headers.begin(), headers.end() );
            sessionPtr->uriParams = HttpBridge::Utils::extractUriParams( wsi );

            // Cache resolved httpExtension
            if( !sessionPtr->httpExtension ) {
                auto it = m_extensions.find( sessionPtr->addon );
                if( it != m_extensions.end() ) {
                    if( auto actualHttpExtension = it->second->getHttpExtension() ) {
                        sessionPtr->httpExtension = actualHttpExtension;
                    }
                }
            }

            if( !sessionPtr->httpExtension ) {
                sessionPtr->closeSession();
                m_httpApiBridge.emit( Metrics::eventRequestFailed, EndpointHandlers::UiExtensions, nullptr );
                return Uid::Extensions::Utils::writeError( wsi, 404, "Specified addon not registered" );
            }

            if( sessionPtr->httpExtension->authorizationRequired() ) {
                if( !Uid::HubOfflineCommandsHandler::anonymousAccessEnabled() ) {
                    std::string username;
                    std::string token;
                    std::tie( username, token ) = HttpBridge::Utils::extractAuthData( wsi );
                    if( !Uid::HubOfflineCommandsHandler::authenticateUser( username, token ) ) {
                        // Return default 401 error
                        sessionPtr->closeSession();
                        m_httpApiBridge.emit(
                            Metrics::eventRequestUnauthorized, EndpointHandlers::UiExtensions, nullptr );
                        return Uid::Extensions::Utils::writeError( wsi, 401, "Not authorized" );
                    }
                }
            }

            if( sessionPtr->httpExtension->hasTimerHandler() ) {
                // Cancel timer for extensions with own timer management, because otherwise it overrides its settings
                lws_set_timer_usecs( wsi, LWS_SET_TIMER_USEC_CANCEL );
            }
            auto result = sessionPtr->httpExtension->onHttp( wsi, *sessionPtr );
            if( result != -1 )
                return result;
        }
        break;
    }
    case LWS_CALLBACK_HTTP_BODY:
        if( sessionPtr && sessionPtr->httpExtension ) {
            if( !sessionPtr->httpExtension->hasTimerHandler() )
                lws_set_timer_usecs( wsi, Timeout );

            auto result =
                sessionPtr->httpExtension->onHttpBody( wsi, *sessionPtr, static_cast< const char* >( in ), len );
            sessionPtr->addBytesRx( len );
            if( result != -1 )
                return result;
        }
        break;
    case LWS_CALLBACK_HTTP_BODY_COMPLETION:
        if( sessionPtr && sessionPtr->httpExtension ) {
            if( !sessionPtr->httpExtension->hasTimerHandler() )
                lws_set_timer_usecs( wsi, Timeout );

            auto result = sessionPtr->httpExtension->onHttpBodyCompletion( wsi, *sessionPtr );
            if( result != -1 ) {
                if( sessionPtr->bytesRx() > 0 ) {
                    // Make a session out of sessionInternal, pass bada there
                    auto session = std::make_shared< HttpBridge::Session >();
                    session->init( wsi );

                    session->getData()->addBytesWritten( sessionPtr->bytesRx() );
                    m_httpApiBridge.emit( Metrics::eventPayloadRx, EndpointHandlers::UiExtensions, session );
                }
                return result;
            }
        }
        break;
    case LWS_CALLBACK_HTTP_WRITEABLE:
        if( sessionPtr && sessionPtr->httpExtension ) {
            if( !sessionPtr->httpExtension->hasTimerHandler() )
                lws_set_timer_usecs( wsi, Timeout );
            auto result = sessionPtr->httpExtension->onHttpWriteable( wsi, *sessionPtr );
            if( result != -1 )
                return result;
        }
        break;
    case LWS_CALLBACK_TIMER: {
        if( sessionPtr ) {
            // Just drop connection
            m_httpApiBridge.emit( Metrics::eventRequestTimeOut, EndpointHandlers::UiExtensions, nullptr );

            if( sessionPtr->httpExtension->hasTimerHandler() ) {
                return sessionPtr->httpExtension->onTimer( wsi, *sessionPtr );
            }
            else if( sessionPtr->responseHeadersWritten ) {
                return 1;
            }
            else {
                return Utils::writeError( wsi, 408, "Request Timeout" );
            }
        }
    }
    default:
        break;
    }
    return lws_callback_http_dummy( wsi, reason, user, in, len );
}

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
