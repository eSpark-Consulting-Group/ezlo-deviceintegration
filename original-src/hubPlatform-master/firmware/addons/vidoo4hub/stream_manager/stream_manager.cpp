#include "vidoo4hub/stream_manager/stream_manager.h"
#include "vidoo4hub/vidoo4hub_exception.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace Vidoo4Hub {
namespace {
const char* const camerasAddonName = "addon.cameras";
}

StreamManager::StreamManager( Services::Service& service, std::chrono::seconds requestTimeout )
: m_service( service )
, m_requestTimeout( requestTimeout ) {}

void StreamManager::addCameraStream( const std::string& streamItemId,
                                     const std::string& streamName,
                                     const std::string& rtspURL ) {
    Json stream;
    json_add_str( stream, "streamId", streamItemId );
    json_add_str( stream, "streamName", streamName );
    json_add_str( stream, "rtspUrl", rtspURL );
    json_add_str( stream, "description", streamName + " desc" );

    auto response =
        m_service.sendRequest( camerasAddonName, std::move( stream ), CAMERAS_ADD_STREAM, m_requestTimeout );
    if( hasError( response ) ) {
        LOGGER_ERROR_RAW( "Failed to add camera stream: " + getErrorMessage( response ) );
    }
}

void StreamManager::removeCameraStream( const std::string& streamItemId ) {
    auto response = m_service.sendRequest(
        camerasAddonName, JSON( "streamId" << streamItemId ), CAMERAS_REMOVE_STREAM, m_requestTimeout );
    if( hasError( response ) ) {
        LOGGER_ERROR_RAW( "Failed to remove camera stream: " + getErrorMessage( response ) );
    }
}

void StreamManager::createLiveRTSPStream( const std::string& streamItemId ) {
    m_service.sendMethodCall( camerasAddonName, CAMERAS_CREATE_LIVE_RTSP_STREAM, JSON( "streamId" << streamItemId ) );
}

void StreamManager::deleteLiveRTSPStream( const std::string& streamItemId ) {
    m_service.sendMethodCall( camerasAddonName, CAMERAS_DELETE_LIVE_RTSP_STREAM, JSON( "streamId" << streamItemId ) );
}

Json StreamManager::getCameraStreams() {
    auto response = m_service.sendRequest( camerasAddonName, JSON_EMPTY_OBJ, REQUEST_CAMERA_STREAMS, m_requestTimeout );
    if( hasError( response ) ) {
        LOGGER_ERROR_RAW( "Failed to get camera streams: " + getErrorMessage( response ) );
        return JSON_EMPTY_ARR;
    }
    return response;
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
