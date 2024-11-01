#include "cameras/input_output/video_converter.h"
#include "cameras/input_output/resolution.h"
#include "cameras/codec/codec_context.h"
#include "cameras/ffmpeg_helpers.h"

extern "C" {
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace HubPlatform {

VideoConverter::VideoConverter( const AVCodecParameters& inputCodecParams,
                                AVRational inputTimeBase,
                                AVFormatContext& outputFormatContext,
                                Resolution outputResolution )
: m_inputTimeBase( inputTimeBase )
, m_outputFormatContext( outputFormatContext ) {
    m_decoderContextWrapper = createDecoderContext( inputCodecParams );
    const AVCodecContext* decoderContext = m_decoderContextWrapper.getContext();

    m_encoderContextWrapper = createEncoderContext( inputCodecParams, inputTimeBase, outputResolution );
    const AVCodecContext* encoderContext = m_encoderContextWrapper.getContext();

    m_swsContext = createSwsContext( *decoderContext, *encoderContext );
}

AVCodecContextWrapper VideoConverter::createDecoderContext( const AVCodecParameters& inputCodecParams ) {
    AVCodec* codec = avcodec_find_decoder( inputCodecParams.codec_id );
    if( !codec ) {
        throw FFmpegHelpers::Exception( "Could not find a decoder for a video converter" );
    }

    AVCodecContextWrapper decoderContextWrapper( codec );
    AVCodecContext* decoderContext = decoderContextWrapper.getContext();
    if( !decoderContext ) {
        throw FFmpegHelpers::Exception( "Could not allocate a decoder context" );
    }

    const int paramsToContextRes = avcodec_parameters_to_context( decoderContext, &inputCodecParams );
    if( paramsToContextRes < 0 ) {
        throw FFmpegHelpers::Exception( "Failed to initialize a decoder context", paramsToContextRes );
    }

    const int codecOpenRes = avcodec_open2( decoderContext, codec, nullptr );
    if( codecOpenRes < 0 ) {
        throw FFmpegHelpers::Exception( "Failed to open a decoder in video converter", codecOpenRes );
    }

    return decoderContextWrapper;
}

AVCodecContextWrapper VideoConverter::createEncoderContext( const AVCodecParameters& inputCodecParams,
                                                            AVRational inputTimeBase,
                                                            Resolution outputResolution ) {
    AVCodec* codec = avcodec_find_encoder( AV_CODEC_ID_H264 );
    if( !codec ) {
        throw FFmpegHelpers::Exception( "Could not find the encoder for a video converter" );
    }

    AVCodecContextWrapper encoderContextWrapper( codec );
    AVCodecContext* encoderContext = encoderContextWrapper.getContext();
    if( !encoderContext ) {
        throw FFmpegHelpers::Exception( "Could not allocate an encoder context" );
    }

    const int paramsToContextRes = avcodec_parameters_to_context( encoderContext, &inputCodecParams );
    if( paramsToContextRes < 0 ) {
        throw FFmpegHelpers::Exception( "Failed to initialize an encoder context in video converter",
                                        paramsToContextRes );
    }

    encoderContext->width = outputResolution.width;
    encoderContext->height = outputResolution.height;
    encoderContext->time_base = inputTimeBase;

    const int setPresetRes = av_opt_set( encoderContext->priv_data, "preset", "ultrafast", 0 );
    if( setPresetRes < 0 ) {
        FFmpegHelpers::logAvError( "Cannot set 'preset' for encoder, default value will be used", setPresetRes );
    }

    const int codecOpenRes = avcodec_open2( encoderContext, codec, nullptr );
    if( codecOpenRes < 0 ) {
        throw FFmpegHelpers::Exception( "Failed to open an encoder in video converter", codecOpenRes );
    }

    return encoderContextWrapper;
}

VideoConverter::SwsContextPtr VideoConverter::createSwsContext( const AVCodecContext& decoderContext,
                                                                const AVCodecContext& encoderContext ) {
    const int scaleAlgorithm = SWS_FAST_BILINEAR;
    SwsContextPtr swsContext( sws_getContext( decoderContext.width,
                                              decoderContext.height,
                                              decoderContext.pix_fmt,
                                              encoderContext.width,
                                              encoderContext.height,
                                              encoderContext.pix_fmt,
                                              scaleAlgorithm,
                                              nullptr,      // srcFilter
                                              nullptr,      // dstFilter
                                              nullptr ) );  // params
    if( !swsContext ) {
        throw FFmpegHelpers::Exception( "Could not initialize SwsContext" );
    }

    return swsContext;
}

int VideoConverter::fillOutputStreamCodecParams( AVStream& stream ) const {
    return avcodec_parameters_from_context( stream.codecpar, m_encoderContextWrapper.getContext() );
}

int VideoConverter::process( AVPacket& inputPacket ) {
    DecodeResult decodeResult = decode( inputPacket );
    int result = decodeResult.code;
    if( decodeResult.gotResult ) {
        result = processDecodedFrame( *decodeResult.frame.getFrame() );
    }
    return result;
}

VideoConverter::DecodeResult VideoConverter::decode( const AVPacket& packet ) {
    AVCodecContext* decoderContext = m_decoderContextWrapper.getContext();

    DecodeResult result;
    result.code = avcodec_send_packet( decoderContext, &packet );
    if( result.code < 0 ) {
        FFmpegHelpers::logAvError( "Sending frame to decoder failed", result.code );
        return result;
    }

    AVFrame* frame = result.frame.getFrame();
    result.code = avcodec_receive_frame( decoderContext, frame );
    result.gotResult = result.code == 0;
    if( result.code == AVERROR( EAGAIN ) ) {
        result.code = 0;
    }
    else if( result.code < 0 ) {
        FFmpegHelpers::logAvError( "Failed to receive decoded frame", result.code );
    }

    return result;
}

VideoConverter::EncodeResult VideoConverter::encode( const AVFrame& frame ) {
    AVCodecContext* encoderContext = m_encoderContextWrapper.getContext();

    EncodeResult result;
    result.code = avcodec_send_frame( encoderContext, &frame );
    if( result.code < 0 ) {
        FFmpegHelpers::logAvError( "Sending frame to encoder failed", result.code );
        return result;
    }

    AVPacket* packet = result.packet.getPacket();
    result.code = avcodec_receive_packet( encoderContext, packet );
    result.gotResult = result.code == 0;
    if( result.code == AVERROR( EAGAIN ) ) {
        result.code = 0;
    }
    else if( result.code < 0 ) {
        FFmpegHelpers::logAvError( "Failed to receive encoded packet", result.code );
    }

    return result;
}

VideoConverter::ScaleResult VideoConverter::scale( const AVFrame& frame ) const {
    ScaleResult result;
    result.code = initEmptyFrame( *result.frame.getFrame() );
    if( result.code < 0 ) {
        FFmpegHelpers::logAvError( "Could not init empty frame for scaling", result.code );
        return result;
    }

    AVFrame* outputFrame = result.frame.getFrame();
    outputFrame->pts = frame.pts;
    outputFrame->pkt_dts = frame.pkt_dts;

    result.code = sws_scale( m_swsContext.get(),
                             (const uint8_t* const*) frame.data,
                             frame.linesize,
                             0,
                             frame.height,
                             outputFrame->data,
                             outputFrame->linesize );
    if( result.code < 0 ) {
        FFmpegHelpers::logAvError( "Failed to scale a frame", result.code );
    }
    return result;
}

int VideoConverter::processDecodedFrame( const AVFrame& frame ) {
    ScaleResult scaleResult = scale( frame );
    int result = scaleResult.code;
    if( scaleResult.code >= 0 ) {
        AVFrame* outputFrame = scaleResult.frame.getFrame();
        EncodeResult encodeResult = encode( *outputFrame );
        result = encodeResult.code;
        if( encodeResult.gotResult ) {
            result = write( *encodeResult.packet.getPacket() );
        }
        av_freep( outputFrame->data );
    }

    return result;
}

int VideoConverter::write( AVPacket& packet ) {
    const AVRational outputTimeBase = m_outputFormatContext.streams[ packet.stream_index ]->time_base;
    av_packet_rescale_ts( &packet, m_inputTimeBase, outputTimeBase );

    const int res = av_interleaved_write_frame( &m_outputFormatContext, &packet );
    if( res < 0 ) {
        FFmpegHelpers::logAvError( "Failed to write a converted frame", res );
    }
    return res;
}

bool VideoConverter::flush() {
    return flushDecoder() && flushEncoder();
}

bool VideoConverter::flushDecoder() {
    AVCodecContext* decoderContext = m_decoderContextWrapper.getContext();

    int res = avcodec_send_packet( decoderContext, nullptr );
    if( res < 0 ) {
        FFmpegHelpers::logAvError( "Could not flush decoded packets", res );
        return false;
    }

    AVFrameWrapper frameWrapper;
    AVFrame* frame = frameWrapper.getFrame();
    while( res >= 0 ) {
        res = avcodec_receive_frame( decoderContext, frame );
        if( res >= 0 ) {
            res = processDecodedFrame( *frame );
        }
    }

    return true;
}

bool VideoConverter::flushEncoder() {
    AVCodecContext* encoderContext = m_encoderContextWrapper.getContext();

    int res = avcodec_send_frame( encoderContext, nullptr );
    if( res < 0 ) {
        FFmpegHelpers::logAvError( "Could not flush encoded frames", res );
        return false;
    }

    AVPacketWrapper packetWrapper;
    AVPacket* packet = packetWrapper.getPacket();
    while( res >= 0 ) {
        res = avcodec_receive_packet( encoderContext, packet );
        if( res >= 0 ) {
            res = write( *packet );
        }
    }

    return true;
}

int VideoConverter::initEmptyFrame( AVFrame& frame ) const {
    const AVCodecContext* encoderContext = m_encoderContextWrapper.getContext();

    frame.format = encoderContext->pix_fmt;
    frame.width = encoderContext->width;
    frame.height = encoderContext->height;
    const int res =
        av_image_alloc( frame.data, frame.linesize, frame.width, frame.height, encoderContext->pix_fmt, 32 );
    return res;
}

}  // namespace HubPlatform
