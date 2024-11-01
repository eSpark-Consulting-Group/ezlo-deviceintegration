#pragma once

#include "uid/extensions/cameras/cameras_audio_stream_manager.h"

#include "uid/extensions/ui_bus_extension.h"

namespace HubPlatform {
namespace Cameras {

class UiBusExtension : public Uid::Extensions::BusExtension {
public:
    explicit UiBusExtension( AudioStreamManager& manager );

private:
    void handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) override;
    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;
    void handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) override;

private:
    void handleCreateAudioStreamMessage( const JsonObject& json );
    void handleDeleteAudioStreamMessage( const JsonObject& json );
    JsonObject handleGetAudioStreamInfoRequest( const JsonObject& json );

private:
    AudioStreamManager& m_manager;
};

}  // namespace Cameras
}  // namespace HubPlatform
