#include "uid/extensions/cameras/cameras_ui_bus_extension.h"

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Cameras {

UiBusExtension::UiBusExtension( AudioStreamManager& manager )
: m_manager( manager ) {}

JsonObject UiBusExtension::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    using MethodHandler = JsonObject ( UiBusExtension::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { CAMERAS_GET_AUDIO_STREAM_INFO, &UiBusExtension::handleGetAudioStreamInfoRequest },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        return ( this->*handler )( request );
    }

    return unhandledRequestAnswer();
}

void UiBusExtension::handleMethodCall( const JsonObject& json,
                                       MessageSubClasses requestType,
                                       const std::string& /*sender*/ ) {
    using MethodHandler = void ( UiBusExtension::* )( const JsonObject& json );
    using MethodHandlersMap = std::unordered_map< MessageSubClasses, MethodHandler, MessageSubClassesHash >;
    static const MethodHandlersMap methodHandlers = {
        { CAMERAS_CREATE_AUDIO_STREAM, &UiBusExtension::handleCreateAudioStreamMessage },
        { CAMERAS_DELETE_AUDIO_STREAM, &UiBusExtension::handleDeleteAudioStreamMessage },
    };
    const auto it = methodHandlers.find( requestType );
    if( it != methodHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( json );
    }
}

// json = {
//      ...
// }
void UiBusExtension::handleCreateAudioStreamMessage( const JsonObject& json ) {
    m_manager.createInputAudioStream( AudioStreamInfo::fromJson( json ) );
}

// json = {
//      "streamId": "<id_string>"
// }
void UiBusExtension::handleDeleteAudioStreamMessage( const JsonObject& json ) {
    auto streamId = json_get_str( json, "streamId" );

    m_manager.deleteInputAudioStream( streamId );
}

// json = {
//      "streamId": "<id_string>"
// }
JsonObject UiBusExtension::handleGetAudioStreamInfoRequest( const JsonObject& json ) {
    auto streamId = json_get_str( json, "streamId" );

    auto streamPtr = m_manager.getInputAudioStream( streamId );
    if( streamPtr ) {
        return streamPtr->getInfo().toJson();
    }
    return {};
}

void UiBusExtension::handleEventFromBus( const JsonObject& event, MessageSubClasses requestType ) {}

}  // namespace Cameras
}  // namespace HubPlatform
