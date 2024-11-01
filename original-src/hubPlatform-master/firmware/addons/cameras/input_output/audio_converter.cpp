#include "cameras/input_output/audio_converter.h"
#include "cameras/codec/codec_context.h"
#include "cameras/ffmpeg_helpers.h"

extern "C" {
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

/// @brief useful links:
/// https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcode_aac.c

namespace HubPlatform {

/* The output bit rate in bit/s */
#define OUTPUT_BIT_RATE 96000
/* The number of output channels */
#define OUTPUT_CHANNELS 2

AudioConverter::AudioConverter( const AVCodecParameters& inputCodecParams,
                                AVFormatContext& outputFormatContext,
                                int streamIndex )
: m_outputFormatContext( outputFormatContext )
, m_streamIndex( streamIndex ) {
    m_decoderContextWrapper = createDecoderContext( inputCodecParams );
    const AVCodecContext* decoderContext = m_decoderContextWrapper.getContext();

    m_encoderContextWrapper = createEncoderContext( inputCodecParams, m_outputFormatContext );
    const AVCodecContext* encoderContext = m_encoderContextWrapper.getContext();

    m_resamplerContext = createResamplerContext( *decoderContext, *encoderContext );
    m_audioFifo = AVAudioFifoWrapper( encoderContext->sample_fmt, encoderContext->channels, 1 );
}

AVCodecContextWrapper AudioConverter::createDecoderContext( const AVCodecParameters& inputCodecParams ) {
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

AVCodecContextWrapper AudioConverter::createEncoderContext( const AVCodecParameters& inputCodecParams,
                                                            const AVFormatContext& outputFormatContext ) {
    AVCodec* codec = avcodec_find_encoder( AV_CODEC_ID_AAC );
    if( !codec ) {
        throw FFmpegHelpers::Exception( "Could not find the encoder for a audio converter" );
    }

    AVCodecContextWrapper encoderContextWrapper( codec );
    AVCodecContext* encoderContext = encoderContextWrapper.getContext();
    if( !encoderContext ) {
        throw FFmpegHelpers::Exception( "Could not allocate an encoder context" );
    }

    /* Set the basic encoder parameters.
     * The input file's sample rate is used to avoid a sample rate conversion. */
    encoderContext->channels = OUTPUT_CHANNELS;
    encoderContext->channel_layout = av_get_default_channel_layout( OUTPUT_CHANNELS );
    encoderContext->sample_rate = inputCodecParams.sample_rate;
    encoderContext->sample_fmt = codec->sample_fmts[ 0 ];
    encoderContext->bit_rate = OUTPUT_BIT_RATE;

    /* Allow the use of the experimental AAC encoder. */
    encoderContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Some container formats (like MP4) require global headers to be present.
     * Mark the encoder so that it behaves accordingly. */
    if( outputFormatContext.oformat->flags & AVFMT_GLOBALHEADER ) {
        encoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    /* Open the encoder for the audio stream to use it later. */
    auto error = avcodec_open2( encoderContext, codec, nullptr );
    if( error < 0 ) {
        throw FFmpegHelpers::Exception( "Could not open codec: " + FFmpegHelpers::avStringError( error ) );
    }

    return encoderContextWrapper;
}

int AudioConverter::decodeAudioFrame( const AVPacket& inputPacket,
                                      AVFrame& frame,
                                      AVCodecContext& inputCodecContext,
                                      int& data_present,
                                      int& finished ) {
    int error = 0;

    /* Send the audio frame stored in the temporary packet to the decoder.
     * The input audio stream decoder is used to do this. */
    if( ( error = avcodec_send_packet( &inputCodecContext, &inputPacket ) ) < 0 ) {
        FFmpegHelpers::logAvError( "Could not send packet for decoding", error );
        return error;
    }

    /* Receive one frame from the decoder. */
    error = avcodec_receive_frame( &inputCodecContext, &frame );
    /* If the decoder asks for more data to be able to decode a frame,
     * return indicating that no data is present. */
    if( error == AVERROR( EAGAIN ) ) {
        error = 0;
    }
    else if( error == AVERROR_EOF ) { /* If the end of the input file is reached, stop decoding. */
        finished = 1;
        error = 0;
    }
    else if( error < 0 ) {
        FFmpegHelpers::logAvError( "Could not decode frame", error );
    }
    else { /* Default case: Return decoded data. */
        data_present = 1;
    }
    return error;
}

int AudioConverter::convertSamples( const uint8_t** inputData,
                                    uint8_t** convertedData,
                                    const int frameSize,
                                    SwrContext& resampleContext ) {
    int error = 0;

    /* Convert the samples using the resampler. */
    error = swr_convert( &resampleContext, convertedData, frameSize, inputData, frameSize );
    if( error < 0 ) {
        FFmpegHelpers::logAvError( "Could not convert input samples", error );
    }
    return 0;
}

int AudioConverter::addSamplesToFifo( AVAudioFifo& fifo, uint8_t** convertedInputSamples, const int frameSize ) {
    int error;

    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    if( ( error = av_audio_fifo_realloc( &fifo, av_audio_fifo_size( &fifo ) + frameSize ) ) < 0 ) {
        FFmpegHelpers::logAvError( "Could not reallocate FIFO" );
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if( av_audio_fifo_write( &fifo, (void**) convertedInputSamples, frameSize ) < frameSize ) {
        FFmpegHelpers::logAvError( "Could not write data to FIFO" );
        return AVERROR_EXIT;
    }
    return 0;
}

int AudioConverter::readDecodeConvertAndStore( const AVPacket& inputPacket,
                                               AVAudioFifo& fifo,
                                               AVCodecContext& inputCodecContext,
                                               AVCodecContext& outputCodecContext,
                                               SwrContext& resamplerContext,
                                               int& finished ) {
    /* Temporary storage of the input samples of the frame read from the file. */
    AVFrameWrapper inputFrame;
    AVFrame* frame = inputFrame.getFrame();
    int dataPresent = 0;
    int ret = AVERROR_EXIT;

    /* Decode one frame worth of audio samples. */
    if( ( ret = decodeAudioFrame( inputPacket, *frame, inputCodecContext, dataPresent, finished ) ) != 0 )
        return ret;
    /* If we are at the end of the file and there are no more samples
     * in the decoder which are delayed, we are actually finished.
     * This must not be treated as an error. */
    if( finished ) {
        ret = 0;
        return ret;
    }
    /* If there is decoded data, convert and store it. */
    if( dataPresent ) {
        /* Initialize the temporary storage for the converted input samples. */
        AudioSamples audioSamples( outputCodecContext.channels, frame->nb_samples, outputCodecContext.sample_fmt );
        auto samples = audioSamples.getAudioSamples();
        if( samples == nullptr ) {
            FFmpegHelpers::logAvError( "Could not allocate converted input sample pointers" );
            return AVERROR( ENOMEM );
        }

        ret = audioSamples.getError();
        if( ret ) {
            FFmpegHelpers::logAvError( "Could not allocate converted input samples", ret );
            return ret;
        }

        /* Convert the input samples to the desired output sample format.
         * This requires a temporary storage provided by converted_input_samples. */
        if( ( ret = convertSamples(
                  (const uint8_t**) frame->extended_data, samples, frame->nb_samples, resamplerContext ) ) ) {
            return ret;
        }

        /* Add the converted input samples to the FIFO buffer for later processing. */
        if( ( ret = addSamplesToFifo( fifo, samples, frame->nb_samples ) ) ) {
            return ret;
        }
    }
    return ret;
}

int AudioConverter::initOutputFrame( AVFrame* frame, const AVCodecContext& outputCodecContext, int frame_size ) {
    int error;

    /* Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity. */
    frame->nb_samples = frame_size;
    frame->channel_layout = outputCodecContext.channel_layout;
    frame->format = outputCodecContext.sample_fmt;
    frame->sample_rate = outputCodecContext.sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    error = av_frame_get_buffer( frame, 0 );
    if( error < 0 ) {
        FFmpegHelpers::logAvError( "Could not allocate output frame samples", error );
        return error;
    }

    return 0;
}

int AudioConverter::encodeAudioFrame( int64_t& pts,
                                      int streamIndex,
                                      AVFrame* frame,
                                      AVFormatContext& outputFormatContext,
                                      AVCodecContext& outputCodecContext,
                                      int& dataPresent ) {
    /* Packet used for temporary storage. */
    AVPacketWrapper outputPacket;
    AVPacket* packet = outputPacket.getPacket();
    int error;

    if( packet == nullptr ) {
        return AVERROR( ENOMEM );
    }

    /* Set a timestamp based on the sample rate for the container. */
    if( frame ) {
        pts += frame->nb_samples;
        frame->pts = pts;
    }

    /* Send the audio frame stored in the temporary packet to the encoder.
     * The output audio stream encoder is used to do this. */
    error = avcodec_send_frame( &outputCodecContext, frame );

    /* The encoder signals that it has nothing more to encode. */
    if( error == AVERROR_EOF ) {
        error = 0;
        return error;
    }
    else if( error < 0 ) {
        FFmpegHelpers::logAvError( "Could not send packet for encoding", error );
        return error;
    }

    /* Receive one encoded frame from the encoder. */
    error = avcodec_receive_packet( &outputCodecContext, packet );

    /* If the encoder asks for more data to be able to provide an
     * encoded frame, return indicating that no data is present. */
    if( error == AVERROR( EAGAIN ) ) {
        error = 0;
        return error;
        /* If the last frame has been encoded, stop encoding. */
    }
    else if( error == AVERROR_EOF ) {
        error = 0;
        return error;
    }
    else if( error < 0 ) {
        FFmpegHelpers::logAvError( "Could not encode frame", error );
        return error;
        /* Default case: Return encoded data. */
    }
    else {
        dataPresent = 1;
    }

    packet->stream_index = streamIndex;

    /* Write one audio frame from the temporary packet to the output file. */
    error = av_write_frame( &outputFormatContext, packet );
    if( dataPresent && error < 0 ) {
        FFmpegHelpers::logAvError( "Could not write frame", error );
        return error;
    }

    return error;
}

int AudioConverter::loadEncodeAndWrite( int64_t& pts,
                                        int streamIndex,
                                        AVAudioFifo& fifo,
                                        AVFormatContext& outputFormatContext,
                                        AVCodecContext& outputCodecContext ) {
    /* Temporary storage of the output samples of the frame written to the file. */
    AVFrameWrapper outputFrame;
    AVFrame* frame = outputFrame.getFrame();

    if( frame == nullptr ) {
        return AVERROR_EXIT;
    }

    /* Use the maximum number of possible samples per frame.
     * If there is less than the maximum possible frame size in the FIFO
     * buffer use this number. Otherwise, use the maximum possible frame size. */
    const int frame_size = FFMIN( av_audio_fifo_size( &fifo ), outputCodecContext.frame_size );
    int dataWritten;

    if( initOutputFrame( frame, outputCodecContext, frame_size ) ) {
        return AVERROR_EXIT;
    }

    /* Read as many samples from the FIFO buffer as required to fill the frame.
     * The samples are stored in the frame temporarily. */
    if( av_audio_fifo_read( &fifo, (void**) frame->data, frame_size ) < frame_size ) {
        FFmpegHelpers::logAvError( "Could not read data from FIFO" );
        return AVERROR_EXIT;
    }

    /* Encode one frame worth of audio samples. */
    if( encodeAudioFrame( pts, streamIndex, frame, outputFormatContext, outputCodecContext, dataWritten ) ) {
        return AVERROR_EXIT;
    }
    return 0;
}

SwrContextWrapper AudioConverter::createResamplerContext( const AVCodecContext& inputCodecContext,
                                                          const AVCodecContext& outputCodecContext ) {
    int error;

    /*
     * Create a resampler context for the conversion.
     * Set the conversion parameters.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity (they are sometimes not detected
     * properly by the demuxer and/or decoder).
     */
    SwrContextWrapper resamplerContext(
        swr_alloc_set_opts( nullptr,
                            av_get_default_channel_layout( outputCodecContext.channels ),
                            outputCodecContext.sample_fmt,
                            outputCodecContext.sample_rate,
                            av_get_default_channel_layout( inputCodecContext.channels ),
                            inputCodecContext.sample_fmt,
                            inputCodecContext.sample_rate,
                            0,
                            nullptr ) );
    if( resamplerContext.getContext() == nullptr ) {
        throw FFmpegHelpers::Exception( "Could not create resampler context" );
    }
    /*
     * Perform a sanity check so that the number of converted samples is
     * not greater than the number of samples to be converted.
     * If the sample rates differ, this case has to be handled differently
     */
    // av_assert0(outputCodecContext->sample_rate == inputCodecContext->sample_rate);
    if( outputCodecContext.sample_rate != inputCodecContext.sample_rate ) {
        throw FFmpegHelpers::Exception(
            "input and output codec contexts are not same: input: " + std::to_string( inputCodecContext.sample_rate ) +
            ", output: " + std::to_string( outputCodecContext.sample_rate ) );
    }

    /* Open the resampler with the specified parameters. */
    if( ( error = swr_init( resamplerContext.getContext() ) ) < 0 ) {
        throw FFmpegHelpers::Exception( "Could not open resample context: " + FFmpegHelpers::avStringError( error ) );
    }

    return resamplerContext;
}

int AudioConverter::fillOutputStreamCodecParams( AVStream& stream ) const {
    return avcodec_parameters_from_context( stream.codecpar, m_encoderContextWrapper.getContext() );
}

int AudioConverter::process( AVPacket& inputPacket ) {
    auto inputCodecContext = m_decoderContextWrapper.getContext();
    auto outputCodecContext = m_encoderContextWrapper.getContext();
    AVAudioFifo* fifo = m_audioFifo.getFifo();

    /* Use the encoder's desired frame size for processing. */
    const int outputFrameSize = outputCodecContext->frame_size;
    int finished = 0;
    int ret = 0;

    /* Decode one frame worth of audio samples, convert it to the
     * output sample format and put it into the FIFO buffer. */
    if( ( ret = readDecodeConvertAndStore( inputPacket,
                                           *fifo,
                                           *inputCodecContext,
                                           *outputCodecContext,
                                           *m_resamplerContext.getContext(),
                                           finished ) ) ) {
        return ret;
    }

    /* Make sure that there is one frame worth of samples in the FIFO
     * buffer so that the encoder can do its work.
     * Since the decoder's and the encoder's frame size may differ, we
     * need to FIFO buffer to store as many frames worth of input samples
     * that they make up at least one frame worth of output samples. */
    /* If we are at the end of the input file, we continue
     * encoding the remaining audio samples to the output file. */
    if( !finished && av_audio_fifo_size( fifo ) < outputFrameSize ) {
        return ret;
    }

    /* If we have enough samples for the encoder, we encode them.
     * At the end of the file, we pass the remaining samples to
     * the encoder. */
    while( av_audio_fifo_size( fifo ) >= outputFrameSize || ( finished && av_audio_fifo_size( fifo ) > 0 ) ) {
        /* Take one frame worth of audio samples from the FIFO buffer,
         * encode it and write it to the output file. */
        if( ( ret = loadEncodeAndWrite( m_pts, m_streamIndex, *fifo, m_outputFormatContext, *outputCodecContext ) ) ) {
            return ret;
        }
    }

    return ret;
}

bool AudioConverter::flush() {
    /* If we are at the end of the input file and have encoded
     * all remaining samples, we can exit this loop and finish. */
    int dataWritten;
    int ret;
    /* Flush the encoder as it may have delayed frames. */
    do {
        dataWritten = 0;
        ret = encodeAudioFrame(
            m_pts, m_streamIndex, nullptr, m_outputFormatContext, *m_encoderContextWrapper.getContext(), dataWritten );
        if( ret ) {
            return false;
        }
    } while( dataWritten );
    return true;
}

}  // namespace HubPlatform
