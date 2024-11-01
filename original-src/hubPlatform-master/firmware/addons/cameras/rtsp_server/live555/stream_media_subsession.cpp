#include "cameras/rtsp_server/live555/stream_media_subsession.h"
#include "cameras/rtsp_server/live555/stream_framed_source.h"
#include "cameras/rtsp_server/event.h"
#include "cameras/packet/packet_buffer.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include "liveMedia.hh"

#include <tiny/tiny_consolecolors.h>
#include <cassert>
#include <utility>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

StreamMediaSubsession* StreamMediaSubsession::createNew( std::string streamId,
                                                         UsageEnvironment& env,
                                                         CodecPtr codecPtr,
                                                         StreamPacketPassthroughPtr packetsPassthrough,
                                                         int streamIndex,
                                                         const std::string& sdp,
                                                         size_t packetCountLimit ) {
    return new StreamMediaSubsession( std::move( streamId ),
                                      env,
                                      std::move( codecPtr ),
                                      std::move( packetsPassthrough ),
                                      streamIndex,
                                      sdp,
                                      packetCountLimit );
}

StreamMediaSubsession::StreamMediaSubsession( std::string streamId,
                                              UsageEnvironment& env,
                                              CodecPtr codecPtr,
                                              StreamPacketPassthroughPtr packetsPassthrough,
                                              int streamIndex,
                                              std::string sdp,
                                              size_t packetCountLimit )
: OnDemandServerMediaSubsession( env, False /*reuseFirstSource, ...*/ )  // @todo: find out other params
, m_streamId( std::move( streamId ) )
, m_codecPtr( std::move( codecPtr ) )
, m_packetsPassthrough( std::move( packetsPassthrough ) )
, m_streamIndex( streamIndex )
, m_sps( getSpsFromSdp( sdp ) )
, m_vps( getVpsFromSdp( sdp ) )
, m_pps( getPpsFromSdp( sdp ) )
, m_packetCountLimit( packetCountLimit ) {
    LOGGER_TRACE_RAW( cc::trace( "Created stream media subsession : " ) + cc::note( m_streamId ) );
}

StreamMediaSubsession::~StreamMediaSubsession() {
    LOGGER_TRACE_RAW( cc::trace( "Deleted stream media subsession: " ) + cc::note( m_streamId ) );
}

FramedSource* StreamMediaSubsession::createNewStreamSource( unsigned /*clientSessionId*/, unsigned& estBitrate ) {
    LOGGER_DEBUG_RAW( cc::debug( "Creating new stream source for " ) +
                      cc::note( m_streamId + "_" + std::to_string( m_streamIndex ) ) );
    assert( m_codecPtr );
    assert( m_packetsPassthrough );

    // You should also set the "estBitrate" result parameter to be the estimated bit rate (in kbps) of the stream.
    // This estimate is used to determine the frequency of RTCP packets; it is not essential that it be accurate.
    estBitrate = ( m_codecPtr->getBitrate() == 0 ? 500 : m_codecPtr->getBitrate() );  // kbps, estimate

    auto packetBuffer = std::make_unique< PacketBuffer >( m_packetCountLimit );

    auto isVideoStream = m_codecPtr->getMediaType() == MediaType::VIDEO;
    auto source = StreamFramedSource::createNew(
        envir(), *m_packetsPassthrough, std::move( packetBuffer ), m_streamId, m_streamIndex, isVideoStream );

    if( m_codecPtr->getCodecType() == CodecType::VIDEO_H264 ) {
        return H264VideoStreamDiscreteFramer::createNew( envir(), source );
    }
    else if( m_codecPtr->getCodecType() == CodecType::VIDEO_H265 ) {
        return H265VideoStreamDiscreteFramer::createNew( envir(), source );
    }
    else if( m_codecPtr->getCodecType() == CodecType::VIDEO_MPEG4 ) {
        return MPEG4VideoStreamDiscreteFramer::createNew( envir(), source );
    }

    return source;
}

RTPSink* StreamMediaSubsession::createNewRTPSink( Groupsock* rtpGroupsock,
                                                  unsigned char rtpPayloadTypeIfDynamic,
                                                  FramedSource* /*inputSource*/ ) {
    Log::debug( cc::debug( "Creating new RTP sink for " ) +
                cc::note( m_streamId + "_" + std::to_string( m_streamIndex ) ) );
    assert( m_codecPtr );

    RTPSink* rtpSink = nullptr;

    if( m_codecPtr->getMediaType() == MediaType::VIDEO ) {  // Video codecs.
        if( m_codecPtr->getCodecType() == CodecType::VIDEO_H264 ) {
            char const* sps = m_sps.empty() ? nullptr : m_sps.c_str();
            if( m_sps.empty() ) {
                Log::warn( cc::warn( "Empty SPS from SDP for " + cc::note( m_codecPtr->getCodecName() ) ) );
            }
            rtpSink = H264VideoRTPSink::createNew( envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, sps );
        }
        else if( m_codecPtr->getCodecType() == CodecType::VIDEO_H265 ) {
            char const* sps = m_sps.empty() ? nullptr : m_sps.c_str();
            char const* vps = m_vps.empty() ? nullptr : m_vps.c_str();
            char const* pps = m_pps.empty() ? nullptr : m_pps.c_str();
            if( m_sps.empty() ) {
                Log::warn( cc::warn( "Empty SPS from SDP for " + cc::note( m_codecPtr->getCodecName() ) ) );
            }
            if( m_sps.empty() ) {
                Log::warn( cc::warn( "Empty VPS from SDP for " + cc::note( m_codecPtr->getCodecName() ) ) );
            }
            if( m_sps.empty() ) {
                Log::warn( cc::warn( "Empty PPS from SDP for " + cc::note( m_codecPtr->getCodecName() ) ) );
            }
            rtpSink = H265VideoRTPSink::createNew( envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, vps, sps, pps );
        }
        else if( m_codecPtr->getCodecType() == CodecType::VIDEO_MPEG4 ) {
            rtpSink = MPEG4ESVideoRTPSink::createNew( envir(), rtpGroupsock, rtpPayloadTypeIfDynamic );
        }
    }
    else if( m_codecPtr->getMediaType() == MediaType::AUDIO ) {  // Audio codecs
        if( m_codecPtr->getCodecType() == CodecType::AUDIO_PCM_ALAW ) {
            rtpSink = SimpleRTPSink::createNew( envir(),
                                                rtpGroupsock,
                                                rtpPayloadTypeIfDynamic,
                                                m_codecPtr->audioSampleRate,
                                                "audio",
                                                "PCMA",
                                                m_codecPtr->audioChannels );
        }
        else if( m_codecPtr->getCodecType() == CodecType::AUDIO_PCM_MULAW ) {
            rtpSink = SimpleRTPSink::createNew( envir(),
                                                rtpGroupsock,
                                                rtpPayloadTypeIfDynamic,
                                                m_codecPtr->audioSampleRate,
                                                "audio",
                                                "PCMU",
                                                m_codecPtr->audioChannels );
        }
        else if( m_codecPtr->getCodecType() == CodecType::AUDIO_AAC ) {
            rtpSink = MPEG4GenericRTPSink::createNew( envir(),
                                                      rtpGroupsock,
                                                      rtpPayloadTypeIfDynamic,
                                                      m_codecPtr->audioSampleRate,
                                                      "audio",
                                                      "aac-hbr",
                                                      "" /*configStr()*/,
                                                      m_codecPtr->audioChannels );
        }
    }
    else {
        Log::error( cc::error( "Not supported media type " ) +
                    cc::note( std::to_string( (int) m_codecPtr->getMediaType() ) ) );
    }

    if( rtpSink ) {
        const std::string codecName = m_codecPtr->getCodecName();
        Log::info( cc::info( "Created RTP sink for codec " ) +
                   cc::note( codecName.empty() ? std::to_string( (int) m_codecPtr->getCodecType() ) : codecName ) );
    }
    else {
        const std::string codecName = m_codecPtr->getCodecName();
        if( codecName.empty() ) {
            Log::warn( cc::warn( "Unknown codec" ) );
        }
        else {
            Log::warn( cc::warn( "Currently codec " ) + cc::note( codecName ) + cc::warn( " is not supported" ) );
        }
    }

    return rtpSink;
}

std::string StreamMediaSubsession::getSpsFromSdp( const std::string& sdp ) {
    return getOptionFromSdp( sdp, { "sprop-parameter-sets=", "sprop-sps=" } );
}

std::string StreamMediaSubsession::getVpsFromSdp( const std::string& sdp ) {
    return getOptionFromSdp( sdp, { "sprop-vps=" } );
}

std::string StreamMediaSubsession::getPpsFromSdp( const std::string& sdp ) {
    return getOptionFromSdp( sdp, { "sprop-pps=" } );
}

std::string StreamMediaSubsession::getOptionFromSdp( const std::string& sdp, const std::vector< std::string >& tags ) {
    std::size_t startPos = std::string::npos;
    std::size_t tagSize = 0;
    for( const auto& tag : tags ) {
        startPos = sdp.find( tag );
        if( startPos != std::string::npos ) {
            tagSize = tag.size();
            break;
        }
    }
    if( startPos == std::string::npos ) {
        return {};
    }
    startPos += tagSize;
    auto endPos = sdp.find_first_of( ";\n", startPos );
    if( endPos == std::string::npos ) {
        return {};
    }
    return sdp.substr( startPos, endPos - startPos );
}

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
