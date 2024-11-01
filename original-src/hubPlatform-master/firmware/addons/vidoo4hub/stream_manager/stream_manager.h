#pragma once

#include "vidoo4hub/stream_manager/stream_manager_interface.h"
#include "service_interfaces/service.h"
#include <string>
#include <chrono>

namespace HubPlatform {
namespace Vidoo4Hub {

class StreamManager : public IStreamManager {
public:
    StreamManager( Services::Service& service, std::chrono::seconds requestTimeout = std::chrono::seconds( 5 ) );
    void addCameraStream( const std::string& streamItemId,
                          const std::string& streamName,
                          const std::string& rtspURL ) override;
    void removeCameraStream( const std::string& streamItemId ) override;
    void createLiveRTSPStream( const std::string& streamItemId ) override;
    void deleteLiveRTSPStream( const std::string& streamItemId ) override;
    Json getCameraStreams() override;

private:
    Services::Service& m_service;
    std::chrono::seconds m_requestTimeout{ 5 };
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
