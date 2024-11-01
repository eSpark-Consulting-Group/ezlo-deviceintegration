#include "uid/extensions/http_links/links.h"
#include "uid/extensions/http_links/http_links_bus_extension.h"
#include "uid/extensions/http_links/http_links_http_extension.h"
#include "uid/extensions/http_links/http_links_manager.h"
#include "uid/extensions/http_links/events.h"
#include "uid/extensions/http_links/errors.h"
#include "uid/extensions/http_links/resources.h"
#include "uid/extensions/ui_bus_messenger.h"
#include "common_utils/cpp14.h"
#include "service_interfaces/groups_names.h"
#include "http_bridge/resources.h"

namespace HubPlatform {
namespace HttpLinks {

HttpLinksExtension::HttpLinksExtension( Uid::Extensions::BusMessenger& messenger, Services::Service& hostService )
: m_messenger( messenger )
, m_manager( hostService, *this ) {
    m_manager.setUp();

    m_busExtension = std::make_unique< HttpLinksBusExtension >( m_manager );
    m_httpExtension = std::make_unique< HttpLinksHttpExtension >( m_manager );
}

const std::string& HttpLinksExtension::getName() const {
    // should match to a protocol name to which the endpoint is bound
    static std::string groupName = HubPlatform::HttpBridge::Resources::httpLinksProtocol;
    return groupName;
}

Uid::Extensions::BusExtension* HttpLinksExtension::getBusExtension() {
    return m_busExtension.get();
}

Uid::Extensions::HttpExtension* HttpLinksExtension::getHttpExtension() {
    return m_httpExtension.get();
}

void HttpLinksExtension::sendLinkEventToBus( const EventBase& event, bool fillOperationId ) {
    Json linkInfo = m_manager.getLinkInfoRaw( event.linkId() );
    const std::string handler = json_get_str( linkInfo, "handler" );
    auto params = JSON_EMPTY_OBJ;
    params[ "script" ] = handler;
    params[ "scriptParams" ] = JSON( "event" << event.name() << "data" << event.toJsonFull() );  // NOT toJson()
    if( fillOperationId )
        params[ "operationId" ] = event.id();  // pass connectionId as a unique id
    const bool dontProduceProgressBroadcasts = false;
    params[ "isUserRunScript" ] = dontProduceProgressBroadcasts;

    m_messenger.sendMethodCall( GroupsNames::EXTENSIONS_MANAGER, PLUGIN_RUN_REQUEST, params );
}

void HttpLinksExtension::onHttpDataReceived( const EventBase& event ) {
    sendLinkEventToBus( event, true );
}

void HttpLinksExtension::onHttpConnectionClosed( const EventBase& event ) {
    Json linkInfo = m_manager.getLinkInfoRaw( event.linkId() );
    const std::string handler = json_get_str( linkInfo, "handler" );
    LOGGER_INFO_RAW( "Send HttpLink IP connection closed: event.id=" + event.id() + ", handler: " + handler );
    sendLinkEventToBus( event, true );
}

void HttpLinksExtension::onHttpDataSend( const EventBase& event ) {
    sendLinkEventToBus( event, true );
}

void HttpLinksExtension::onHttpLinkUpdated( const EventBase& event ) {
    LOGGER_INFO( "HttpLink updated: " + cc::j( event.toJson() ) );
    sendLinkEventToBus( event, false );
}

}  // namespace HttpLinks
}  // namespace HubPlatform
