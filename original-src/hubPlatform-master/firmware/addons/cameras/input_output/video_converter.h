#pragma once

#include "cameras/packet/packet.h"
#include "cameras/frame/frame.h"
#include "cameras/codec/codec_context.h"
#include "cameras/defs.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

namespace HubPlatform {

class Resolution;

class VideoConverter {
public:
    VideoConverter( const AVCodecParameters& inputCodecParams,
                    AVRational inputTimeBase,
                    AVFormatContext& outputFormatContext,
                    Resolution outputResolution );
    ~VideoConverter() = default;

    int fillOutputStreamCodecParams( AVStream& stream ) const;

    int process( AVPacket& inputPacket );
    bool flush();

private:
    struct SwsContextDeleter {
        void operator()( SwsContext* ptr ) {
            sws_freeContext( ptr );
        }
    };
    using SwsContextPtr = std::unique_ptr< SwsContext, SwsContextDeleter >;

    struct DecodeResult {
        int code = 0;
        bool gotResult = false;
        AVFrameWrapper frame;
    };
    struct EncodeResult {
        int code = 0;
        bool gotResult = false;
        AVPacketWrapper packet;
    };
    struct ScaleResult {
        int code = 0;
        AVFrameWrapper frame;
    };

    DecodeResult decode( const AVPacket& packet );
    EncodeResult encode( const AVFrame& frame );
    ScaleResult scale( const AVFrame& frame ) const;
    int processDecodedFrame( const AVFrame& frame );
    int write( AVPacket& packet );

    bool flushDecoder();
    bool flushEncoder();

    int initEmptyFrame( AVFrame& frame ) const;

    static AVCodecContextWrapper createDecoderContext( const AVCodecParameters& inputCodecParams );
    static AVCodecContextWrapper createEncoderContext( const AVCodecParameters& inputCodecParams,
                                                       AVRational inputTimeBase,
                                                       Resolution outputResolution );
    static SwsContextPtr createSwsContext( const AVCodecContext& decoderContext, const AVCodecContext& encoderContext );

private:
    AVRational m_inputTimeBase;
    AVFormatContext& m_outputFormatContext;

    AVCodecContextWrapper m_decoderContextWrapper;
    AVCodecContextWrapper m_encoderContextWrapper;
    SwsContextPtr m_swsContext;
};

}  // namespace HubPlatform
