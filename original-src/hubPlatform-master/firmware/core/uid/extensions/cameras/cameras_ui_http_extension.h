#pragma once

#include "uid/extensions/cameras/cameras_audio_stream_manager.h"

#include "uid/extensions/ui_http_extension.h"

namespace HubPlatform {
namespace Cameras {

class UiHttpExtension : public Uid::Extensions::HttpExtension {
public:
    explicit UiHttpExtension( AudioStreamManager& manager );

private:
    int onHttp( struct lws* wsi, Uid::Extensions::HttpSession& session ) override;
    int onHttpBody( struct lws* wsi, Uid::Extensions::HttpSession& session, const char* data, size_t size ) override;
    int onHttpBodyCompletion( struct lws* wsi, Uid::Extensions::HttpSession& session ) override;
    int onHttpWriteable( struct lws* wsi, Uid::Extensions::HttpSession& session ) override;
    int onHttpClosed( struct lws* wsi, Uid::Extensions::HttpSession& session ) override;
    int onTimer( struct lws* wsi, HubPlatform::Uid::Extensions::HttpSession& session ) override;

    bool authorizationRequired() const override;

    static void fetchOtelParams( HubPlatform::Uid::Extensions::HttpSession& session );

private:
    AudioStreamManager& m_manager;
};

}  // namespace Cameras
}  // namespace HubPlatform
