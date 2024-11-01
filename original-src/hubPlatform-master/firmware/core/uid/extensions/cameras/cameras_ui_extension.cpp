#include "uid/extensions/cameras/cameras_ui_extension.h"
#include "uid/extensions/ui_bus_messenger.h"

#include <fstream>

namespace HubPlatform {
namespace Cameras {

UiExtension::UiExtension( Uid::Extensions::BusMessenger& messenger, Services::Service& hostService )
: m_messenger( messenger )
, m_manager( *this, hostService )
, m_busExtension( m_manager )
, m_httpExtension( m_manager ) {}

const std::string& UiExtension::getName() const {
    static std::string groupName = "cameras";
    return groupName;
}

Uid::Extensions::BusExtension* UiExtension::getBusExtension() {
    return &m_busExtension;
}

Uid::Extensions::HttpExtension* UiExtension::getHttpExtension() {
    return &m_httpExtension;
}

void UiExtension::onStreamStarted( const std::string& streamId, const JsonObject& userData ) {
    Log::info( cc::info( "Input audio stream started: " ) + cc::note( streamId ) );
    m_messenger.sendMethodCall( "addon.cameras",
                                EXTERNAL_EVENT,
                                JSON( "event"
                                      << "audio_stream_started"
                                      << "streamId" << streamId << "user_data" << userData ) );
}

void UiExtension::onStreamStopped( const std::string& streamId, const JsonObject& userData ) {
    Log::info( cc::info( "Input audio stream stopped: " ) + cc::note( streamId ) );
    m_messenger.sendMethodCall( "addon.cameras",
                                EXTERNAL_EVENT,
                                JSON( "event"
                                      << "audio_stream_stopped"
                                      << "streamId" << streamId << "user_data" << userData ) );
}

void UiExtension::onStreamError( const std::string& streamId,
                                 const JsonObject& userData,
                                 const std::string& errorMessage ) {
    Log::error( cc::error( "Input audio stream error: " ) + cc::note( streamId ) + cc::error( " - " ) +
                cc::note( errorMessage ) );
    m_messenger.sendMethodCall( "addon.cameras",
                                EXTERNAL_EVENT,
                                JSON( "event"
                                      << "audio_stream_error"
                                      << "streamId" << streamId << "errorMessage" << errorMessage << "user_data"
                                      << userData ) );
}

}  // namespace Cameras
}  // namespace HubPlatform
