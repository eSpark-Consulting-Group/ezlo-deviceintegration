#pragma once

#include "uid/extensions/ui_http_session.h"

namespace HubPlatform {
namespace Uid {
namespace Extensions {

// Provide HttpExtension if you want to handle http requests
class HttpExtension {
public:
    virtual ~HttpExtension() = default;

    // TODO: get rid of lws* parameter, and provide some interface to sending data
    virtual int onHttp( struct lws* wsi, HttpSession& session ) = 0;
    virtual int onHttpBody( struct lws* wsi, HttpSession& session, const char* data, size_t size ) = 0;
    virtual int onHttpBodyCompletion( struct lws* wsi, HttpSession& session ) = 0;
    virtual int onHttpClosed( struct lws* wsi, HttpSession& session ) = 0;
    virtual int onHttpWriteable( struct lws* wsi, HttpSession& session ) = 0;
    virtual int onTimer( struct lws* wsi, HubPlatform::Uid::Extensions::HttpSession& session ) = 0;

    virtual bool authorizationRequired() const = 0;
    virtual bool hasTimerHandler() const {
        return false;
    }
};

}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
