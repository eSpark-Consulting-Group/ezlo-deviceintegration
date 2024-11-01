#pragma once

#include "cameras/codec/codec_interface.h"
#include "cameras/packet/packet_publisher_interface.h"

#include <memory>
#include <vector>

namespace HubPlatform {
namespace Cameras {

class IRtspServer {
public:
    using Codec = std::unique_ptr< ICodec >;
    using Codecs = std::vector< Codec >;
    using StreamId = std::string;
    using StreamIds = std::vector< StreamId >;

    virtual ~IRtspServer() = default;
    virtual void createCameraStream( IPacketPublisher& packetHandler,
                                     const std::string& streamId,
                                     Codecs&& codecs,
                                     const std::string& sdp ) = 0;
    virtual void deleteCameraStream( const StreamId& streamId ) = 0;
    virtual void handle() = 0;
    virtual void stop() = 0;
};

}  // namespace Cameras
}  // namespace HubPlatform
