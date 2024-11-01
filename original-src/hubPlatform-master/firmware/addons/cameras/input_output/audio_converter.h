#pragma once

#include "cameras/packet/packet.h"
#include "cameras/frame/frame.h"
#include "cameras/codec/codec_context.h"
#include "cameras/input_output/swr_context.h"
#include "cameras/input_output/audio_fifo.h"
#include "cameras/defs.h"

#include "cameras/input_output/audio_samples.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

namespace HubPlatform {

class Resolution;

class AudioConverter {
public:
    AudioConverter( const AVCodecParameters& inputCodecParams, AVFormatContext& outputFormatContext, int streamIndex );
    ~AudioConverter() = default;

    int fillOutputStreamCodecParams( AVStream& stream ) const;
    int process( AVPacket& inputPacket );
    bool flush();

private:
    static AVCodecContextWrapper createDecoderContext( const AVCodecParameters& inputCodecParams );
    static AVCodecContextWrapper createEncoderContext( const AVCodecParameters& inputCodecParams,
                                                       const AVFormatContext& outputFormatContext );

    /**
     * Initialize the audio resampler based on the input and output codec settings.
     * If the input and output sample formats differ, a conversion is required
     * libswresample takes care of this, but requires initialization.
     * @param      inputCodecContext  Codec context of the input
     * @param      outputCodecContext Codec context of the output
     */
    static SwrContextWrapper createResamplerContext( const AVCodecContext& inputCodecContext,
                                                     const AVCodecContext& outputCodecContext );

    /**
     * Decode one audio frame from the input file.
     * @param      frame                Audio frame to be decoded
     * @param      input_codec_context  Codec context of the input file
     * @param[out] data_present         Indicates whether data has been decoded
     * @param[out] finished             Indicates whether the end of file has
     *                                  been reached and all data has been
     *                                  decoded. If this flag is false, there
     *                                  is more data to be decoded, i.e., this
     *                                  function has to be called again.
     * @return Error code (0 if successful)
     */
    static int decodeAudioFrame( const AVPacket& inputPacket,
                                 AVFrame& frame,
                                 AVCodecContext& inputCodecContext,
                                 int& data_present,
                                 int& finished );

    /**
     * Convert the input audio samples into the output sample format.
     * The conversion happens on a per-frame basis, the size of which is
     * specified by frame_size.
     * @param      inputData       Samples to be decoded. The dimensions are
     *                              channel (for multi-channel audio), sample.
     * @param[out] convertedData   Converted samples. The dimensions are channel
     *                              (for multi-channel audio), sample.
     * @param      frameSize       Number of samples to be converted
     * @param      resampleContext Resample context for the conversion
     * @return Error code (0 if successful)
     */
    static int convertSamples( const uint8_t** inputData,
                               uint8_t** convertedData,
                               const int frameSize,
                               SwrContext& resampleContext );

    /**
     * Add converted input audio samples to the FIFO buffer for later processing.
     * @param fifo                    Buffer to add the samples to
     * @param convertedInputSamples Samples to be added. The dimensions are channel
     *                                (for multi-channel audio), sample.
     * @param frameSize              Number of samples to be converted
     * @return Error code (0 if successful)
     */
    static int addSamplesToFifo( AVAudioFifo& fifo, uint8_t** convertedInputSamples, const int frameSize );

    /**
     * Read one audio frame from the input file, decode, convert and store
     * it in the FIFO buffer.
     * @param      fifo                 Buffer used for temporary storage
     * @param      input_format_context Format context of the input file
     * @param      input_codec_context  Codec context of the input file
     * @param      output_codec_context Codec context of the output file
     * @param      resampler_context    Resample context for the conversion
     * @param[out] finished             Indicates whether the end of file has
     *                                  been reached and all data has been
     *                                  decoded. If this flag is false,
     *                                  there is more data to be decoded,
     *                                  i.e., this function has to be called
     *                                  again.
     * @return Error code (0 if successful)
     */
    static int readDecodeConvertAndStore( const AVPacket& inputPacket,
                                          AVAudioFifo& fifo,
                                          AVCodecContext& inputCodecContext,
                                          AVCodecContext& outputCodecContext,
                                          SwrContext& resamplerContext,
                                          int& finished );

    /**
     * Initialize one input frame for writing to the output file.
     * The frame will be exactly frame_size samples large.
     * @param[out] frame                Frame to be initialized
     * @param      output_codec_context Codec context of the output file
     * @param      frame_size           Size of the frame
     * @return Error code (0 if successful)
     */
    static int initOutputFrame( AVFrame* frame, const AVCodecContext& outputCodecContext, int frame_size );

    /**
     * Encode one frame worth of audio to the output file.
     * @param      pts                   Global timestamp for the audio frames
     * @param      streamIndex           audio stram index
     * @param      frame                 Samples to be encoded
     * @param      output_format_context Format context of the output file
     * @param      output_codec_context  Codec context of the output file
     * @param[out] data_present          Indicates whether data has been
     *                                   encoded
     * @return Error code (0 if successful)
     */
    static int encodeAudioFrame( int64_t& pts,
                                 int streamIndex,
                                 AVFrame* frame,
                                 AVFormatContext& outputFormatContext,
                                 AVCodecContext& outputCodecContext,
                                 int& dataPresent );

    /**
     * Load one audio frame from the FIFO buffer, encode and write it to the
     * output file.
     * @param pts                   Global timestamp for the audio frames
     * @param streamIndex           audio stram index
     * @param fifo                  Buffer used for temporary storage
     * @param output_format_context Format context of the output file
     * @param output_codec_context  Codec context of the output file
     * @return Error code (0 if successful)
     */
    static int loadEncodeAndWrite( int64_t& pts,
                                   int streamIndex,
                                   AVAudioFifo& fifo,
                                   AVFormatContext& outputFormatContext,
                                   AVCodecContext& outputCodecContext );

private:
    AVFormatContext& m_outputFormatContext;

    AVCodecContextWrapper m_decoderContextWrapper;
    AVCodecContextWrapper m_encoderContextWrapper;
    SwrContextWrapper m_resamplerContext;
    AVAudioFifoWrapper m_audioFifo;

    /* Global timestamp for the audio frames. */
    int64_t m_pts = 0;
    int m_streamIndex = 1;
};

}  // namespace HubPlatform
