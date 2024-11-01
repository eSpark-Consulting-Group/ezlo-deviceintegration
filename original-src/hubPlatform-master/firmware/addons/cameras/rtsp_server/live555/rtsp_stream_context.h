#pragma once

#include "cameras/packet/packet_publisher_interface.h"
#include "cameras/events/event_publisher_interface.h"
#include "cameras/rtsp_server/event.h"
#include "cameras/codec/codec_interface.h"

#include "UsageEnvironment/UsageEnvironment.hh"
#include "liveMedia/RTSPServer.hh"

#include <memory>
#include <vector>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

struct RtspStreamContext {
    using Codec = std::unique_ptr< ICodec >;
    using Codecs = std::vector< Codec >;

    RtspStreamContext( std::string streamId,
                       IPacketPublisher& packetPublisher,
                       IEventPublisher< EventBase >& eventPublisher,
                       Codecs codecs,
                       std::string sdp,
                       size_t packetCountLimit )
    : packetPublisher( packetPublisher )
    , eventPublisher( eventPublisher )
    , streamId( std::move( streamId ) )
    , codecs( std::move( codecs ) )
    , sdp( std::move( sdp ) )
    , packetCountLimit( packetCountLimit ) {}

    IPacketPublisher& packetPublisher;
    IEventPublisher< EventBase >& eventPublisher;
    std::string streamId;
    Codecs codecs;
    std::string sdp;
    size_t packetCountLimit;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
