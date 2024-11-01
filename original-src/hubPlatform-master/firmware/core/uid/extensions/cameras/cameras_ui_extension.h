#pragma once

#include "uid/extensions/cameras/cameras_audio_stream_manager.h"
#include "uid/extensions/cameras/cameras_ui_bus_extension.h"
#include "uid/extensions/cameras/cameras_ui_http_extension.h"

#include "uid/extensions/ui_extension.h"

namespace HubPlatform {

namespace Services {
class Service;
}

namespace Cameras {

class UiExtension : public Uid::Extensions::Extension, public StreamEventsHandler {
public:
    explicit UiExtension( Uid::Extensions::BusMessenger& messenger, Services::Service& hostService );

    const std::string& getName() const override;

    Uid::Extensions::BusExtension* getBusExtension() override;
    Uid::Extensions::HttpExtension* getHttpExtension() override;

public:
    void onStreamStarted( const std::string& streamId, const JsonObject& userData ) override;
    void onStreamStopped( const std::string& streamId, const JsonObject& userData ) override;
    void onStreamError( const std::string& streamId,
                        const JsonObject& userData,
                        const std::string& errorMessage ) override;

private:
    Uid::Extensions::BusMessenger& m_messenger;
    AudioStreamManager m_manager;
    UiBusExtension m_busExtension;
    UiHttpExtension m_httpExtension;
};

}  // namespace Cameras
}  // namespace HubPlatform
