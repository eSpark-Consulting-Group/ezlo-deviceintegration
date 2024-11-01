#pragma once

#include "cameras/codec/codec_interface.h"

#include <memory>
#include <vector>
#include <chrono>

namespace HubPlatform {
namespace Cameras {

class IPacketPublisher;

class IRtspServerController {
public:
    using Codec = std::unique_ptr< ICodec >;
    using Codecs = std::vector< Codec >;
    using StreamId = std::string;
    using StreamIds = std::vector< StreamId >;

    enum class Status { WaitingForStart, Running, Stopped };

    virtual ~IRtspServerController() = default;
    virtual void createCameraStream( IPacketPublisher& packetHandler,
                                     const std::string& streamId,
                                     Codecs&& codecs,
                                     const std::string& sdp ) = 0;
    virtual void deleteCameraStream( const StreamId& streamId ) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isStarted() const = 0;
    virtual bool waitForStart( std::chrono::seconds timeout ) = 0;
    virtual StreamIds getStreamIds() const = 0;
    virtual size_t getStreamQuantity() const = 0;
    virtual Status getStatus() const = 0;
};

}  // namespace Cameras
}  // namespace HubPlatform
