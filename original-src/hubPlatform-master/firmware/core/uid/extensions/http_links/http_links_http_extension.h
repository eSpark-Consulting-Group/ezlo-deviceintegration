#pragma once

#include "uid/extensions/ui_extension.h"
#include "uid/extensions/ui_bus_extension.h"
#include "uid/extensions/ui_http_extension.h"
#include "bus/messagesclasses.h"
#include "uid/extensions/http_links/http_links_manager.h"
#include "uid/extensions/http_links/connection_closed_reason.h"
#include "uid/extensions/http_links/post_user_data.h"
#include "json_utils/json_fwd.h"
#include <string>
#include <utility>

namespace HubPlatform {
namespace HttpLinks {

class HttpLinksHttpExtension : public HubPlatform::Uid::Extensions::HttpExtension {
public:
    using HttpSession = HubPlatform::Uid::Extensions::HttpSession;
    using PostUserData = HubPlatform::HttpLinks::PostUserData;
    explicit HttpLinksHttpExtension( HttpLinksManager& manager );

private:
    int onHttp( struct lws* wsi, HttpSession& session ) override;

    int onHttpBody( struct lws* wsi, HttpSession& session, const char* data, size_t size ) override;

    int onHttpBodyCompletion( struct lws* wsi, HttpSession& session ) override;

    int onHttpWriteable( struct lws* wsi, HttpSession& session ) override;
    int onHttpClosed( struct lws* wsi, HttpSession& session ) override;
    int onTimer( struct lws* wsi, HttpSession& session ) override;

    bool authorizationRequired() const override;
    bool hasTimerHandler() const override;

private:
    void emitConnectionCreated( lws* wsi, HttpSession& session );
    void emitConnectionClosed( lws* wsi, HttpSession& session, int line );
    void emitDataReceived( lws* wsi, HttpSession& session, bool last );
    void emitDataSend( lws* wsi, HttpSession& session );
    static PostUserData* sessionUserData( const HttpSession& session );

private:
    HubPlatform::HttpLinks::HttpLinksManager& m_manager;
};

}  // namespace HttpLinks
}  // namespace HubPlatform