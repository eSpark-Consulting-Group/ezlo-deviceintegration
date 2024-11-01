#pragma once

#include "uid/extensions/http_links/events/event_base.h"

namespace HubPlatform {
namespace HttpLinks {

class HttpLinksEventHandler {
public:
    virtual ~HttpLinksEventHandler() = default;
    virtual void onHttpDataReceived( const EventBase& event ) = 0;
    virtual void onHttpConnectionClosed( const EventBase& event ) = 0;
    virtual void onHttpDataSend( const EventBase& event ) = 0;
    virtual void onHttpLinkUpdated( const EventBase& event ) = 0;
};

}  // namespace HttpLinks
}  // namespace HubPlatform