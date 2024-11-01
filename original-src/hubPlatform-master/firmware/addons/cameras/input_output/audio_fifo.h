#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
}

namespace HubPlatform {

class AVAudioFifoWrapper {
public:
    AVAudioFifoWrapper();
    /**
     * Initialize a FIFO buffer for the audio samples to be encoded.
     * @param  sampleFormat
     * @param  channels
     * @param  samplesNumber
     */
    AVAudioFifoWrapper( AVSampleFormat sampleFormat, int channels, int samplesNumber );
    ~AVAudioFifoWrapper();

    AVAudioFifoWrapper( const AVAudioFifoWrapper& ) = delete;
    AVAudioFifoWrapper( AVAudioFifoWrapper&& r ) noexcept;
    AVAudioFifoWrapper& operator=( AVAudioFifoWrapper&& r ) noexcept;

    AVAudioFifo* getFifo();
    const AVAudioFifo* getFifo() const;

private:
    AVAudioFifo* m_audioFifo;
};

}  // namespace HubPlatform
