#pragma once
#include "uid/extensions/ui_extension.h"
#include "uid/extensions/http_links/http_links_manager.h"
#include "uid/extensions/http_links/event_handler.h"
#include "events.h"
#include <memory>
#include <string>

namespace HubPlatform {
namespace HttpLinks {

class HttpLinksManager;

class HttpLinksExtension : public Uid::Extensions::Extension, public HttpLinksEventHandler {
public:
    explicit HttpLinksExtension( Uid::Extensions::BusMessenger& messenger, Services::Service& hostService );
    ~HttpLinksExtension() override = default;
    const std::string& getName() const override;
    BusExtension* getBusExtension() override;
    HttpExtension* getHttpExtension() override;

public:  // event handler
    void onHttpDataReceived( const EventBase& event ) override;
    void onHttpConnectionClosed( const EventBase& event ) override;
    void onHttpDataSend( const EventBase& event ) override;
    void onHttpLinkUpdated( const EventBase& event ) override;

private:
    Uid::Extensions::BusMessenger& m_messenger;
    HttpLinksManager m_manager;
    std::unique_ptr< Uid::Extensions::BusExtension > m_busExtension;
    std::unique_ptr< Uid::Extensions::HttpExtension > m_httpExtension;

    void sendLinkEventToBus( const EventBase& event, bool fillOperationId );
};

}  // namespace HttpLinks
}  // namespace HubPlatform
