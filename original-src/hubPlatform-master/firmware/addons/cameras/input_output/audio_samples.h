#pragma once

extern "C" {
#include "libavutil/samplefmt.h"
}

#include <stdint.h>

namespace HubPlatform {

class AudioSamples {
public:
    AudioSamples( int channelsNumber, int frameSize, AVSampleFormat sampleFormat );
    ~AudioSamples();

    AudioSamples( const AudioSamples& ) = delete;
    AudioSamples( AudioSamples&& r ) noexcept;
    AudioSamples& operator=( AudioSamples&& r ) noexcept;

    uint8_t** getAudioSamples() const;
    int getAudioSamplesNumber() const;
    int getError() const;

private:
    uint8_t** m_audioSamples;
    int m_error = -1;
    int m_samplesNumber = 0;
};

}  // namespace HubPlatform
