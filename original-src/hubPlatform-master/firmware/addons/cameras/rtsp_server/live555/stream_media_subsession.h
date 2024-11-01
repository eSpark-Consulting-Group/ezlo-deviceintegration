#pragma once

#include "cameras/codec/codec_interface.h"
#include "cameras/packet/packet_publisher_interface.h"
#include "cameras/events/event_publisher_interface.h"

#include "cameras/rtsp_server/event.h"

#include "liveMedia/OnDemandServerMediaSubsession.hh"

#include <memory>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

class StreamPacketPassthrough;

class StreamMediaSubsession : public OnDemandServerMediaSubsession {
public:
    using CodecPtr = std::unique_ptr< ICodec >;
    using StreamPacketPassthroughPtr = std::shared_ptr< StreamPacketPassthrough >;

    static StreamMediaSubsession* createNew( std::string streamId,
                                             UsageEnvironment& env,
                                             CodecPtr codecPtr,
                                             StreamPacketPassthroughPtr packetsPassthrough,
                                             int streamIndex,
                                             const std::string& sdp,
                                             size_t packetCountLimit );

protected:
    StreamMediaSubsession( std::string streamId,
                           UsageEnvironment& env,
                           CodecPtr codecPtr,
                           StreamPacketPassthroughPtr packetsPassthrough,
                           int streamIndex,
                           std::string sdp,
                           size_t packetCountLimit );

    ~StreamMediaSubsession() override;

protected:
    FramedSource* createNewStreamSource( unsigned clientSessionId, unsigned& estBitrate ) override;
    RTPSink* createNewRTPSink( Groupsock* rtpGroupsock,
                               unsigned char rtpPayloadTypeIfDynamic,
                               FramedSource* inputSource ) override;

private:
    std::string getSpsFromSdp( const std::string& sdp );
    std::string getVpsFromSdp( const std::string& sdp );
    std::string getPpsFromSdp( const std::string& sdp );
    std::string getOptionFromSdp( const std::string& sdp, const std::vector< std::string >& tags );

private:
    std::string m_streamId;
    CodecPtr m_codecPtr;
    StreamPacketPassthroughPtr m_packetsPassthrough;
    const int m_streamIndex;
    const std::string m_sps;
    const std::string m_vps;
    const std::string m_pps;
    const size_t m_packetCountLimit;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
