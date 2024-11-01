#include "cameras/local_stream/live_streamer.h"

#include "cameras/rtsp_server/rtsp_server_controller_interface.h"
#include "cameras/rtsp_server/rtsp_server_exception.h"
#include "cameras/input_output/camera_input_interface.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/codec/video_codec.h"
#include "cameras/codec/audio_codec.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"

#include <tiny/tiny_consolecolors.h>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
}

namespace HubPlatform {
namespace Cameras {

LiveStreamer::LiveStreamer( IRtspServerController& rtspServer,
                            ICameraInput& cameraInput,
                            LiveStreamer::StreamId streamId )
: m_rtspServer( rtspServer )
, m_cameraInput( cameraInput )
, m_packetPublisher( std::make_unique< PacketPublisher >() )
, m_streamId( std::move( streamId ) ) {}

void LiveStreamer::createCameraStream() {
    auto convertCodecType = []( AVCodecID codec ) {
        switch( codec ) {
        case AVCodecID::AV_CODEC_ID_H264:
            return CodecType::VIDEO_H264;

        case AVCodecID::AV_CODEC_ID_H265:
            return CodecType::VIDEO_H265;

        case AVCodecID::AV_CODEC_ID_MPEG4:
            return CodecType::VIDEO_MPEG4;

        case AVCodecID::AV_CODEC_ID_PCM_MULAW:
            return CodecType::AUDIO_PCM_MULAW;

        case AVCodecID::AV_CODEC_ID_AAC:
            return CodecType::AUDIO_AAC;

        default:
            throw RtspServerException( "Unsupported codec: " + std::to_string( (int) codec ) );
        }
    };

    IRtspServerController::Codecs codecs;
    if( auto videoStream = m_cameraInput.getVideoStream() ) {
        auto* codecpar = videoStream->codecpar;
        codecs.emplace_back( std::make_unique< VideoCodec >( videoStream->index,
                                                             codecpar->bit_rate,
                                                             convertCodecType( codecpar->codec_id ),
                                                             avcodec_get_name( codecpar->codec_id ) ) );
    }
    if( auto audioStream = m_cameraInput.getAudioStream() ) {
        auto* codecpar = audioStream->codecpar;
        auto codec = std::make_unique< AudioCodec >( audioStream->index,
                                                     codecpar->bit_rate,
                                                     convertCodecType( codecpar->codec_id ),
                                                     avcodec_get_name( codecpar->codec_id ) );
        codec->audioChannels = codecpar->channels;
        codec->audioSampleRate = codecpar->sample_rate;
        codecs.emplace_back( std::move( codec ) );
    }
    if( codecs.empty() ) {
        Log::error( cc::error( "No input for Stream with id " ) + cc::error( m_streamId ) );
        throw CameraException( "No input for Stream with id \"" + m_streamId + "\"" );
    }

    const int videoStreamIdx = m_cameraInput.getVideoIndex();
    PacketHandlerSharedPtr packetHandler = m_cameraInput.getPacketHandler();
    packetHandler->addListener( [this, videoStreamIdx]( const AVPacketPrototype& packetPrototype ) {
        Packet outputPacket;
        const auto& inputPacket = packetPrototype.getPacket();
        outputPacket.data = std::vector< uint8_t >( inputPacket.data, inputPacket.data + inputPacket.size );
        outputPacket.streamIndex = inputPacket.stream_index;
        outputPacket.isKey = inputPacket.stream_index == videoStreamIdx && inputPacket.flags == AV_PKT_FLAG_KEY;
        m_packetPublisher->sendPacket( outputPacket );
    } );
    m_rtspServer.createCameraStream( *m_packetPublisher, m_streamId, std::move( codecs ), m_cameraInput.getSdp() );
}

void LiveStreamer::deleteCameraStream() {
    m_rtspServer.deleteCameraStream( m_streamId.c_str() );
}

}  // namespace Cameras
}  // namespace HubPlatform
