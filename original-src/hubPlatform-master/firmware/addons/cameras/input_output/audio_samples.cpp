#include "cameras/input_output/audio_samples.h"

#include <stdlib.h>

namespace HubPlatform {

AudioSamples::AudioSamples( int channelsNumber, int frameSize, AVSampleFormat sampleFormat )
: m_audioSamples( nullptr ) {
    /* Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if( !( m_audioSamples = (uint8_t**) calloc( channelsNumber, sizeof( **m_audioSamples ) ) ) ) {
        return;
    }

    /* Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    m_error = av_samples_alloc( m_audioSamples, nullptr, channelsNumber, frameSize, sampleFormat, 0 );
    if( m_error > 0 ) {
        m_samplesNumber = m_error;
        m_error = 0;
    }
}

AudioSamples::~AudioSamples() {
    if( m_audioSamples ) {
        av_freep( &m_audioSamples[ 0 ] );
        free( m_audioSamples );
    }
}

AudioSamples::AudioSamples( AudioSamples&& r ) noexcept
: m_audioSamples( r.m_audioSamples ) {
    r.m_audioSamples = nullptr;
}

AudioSamples& AudioSamples::operator=( AudioSamples&& r ) noexcept {
    if( m_audioSamples ) {
        av_freep( &( m_audioSamples )[ 0 ] );
        free( m_audioSamples );
    }
    m_audioSamples = r.m_audioSamples;
    r.m_audioSamples = nullptr;
    return *this;
}

uint8_t** AudioSamples::getAudioSamples() const {
    return m_audioSamples;
}

int AudioSamples::getAudioSamplesNumber() const {
    return m_samplesNumber;
}

int AudioSamples::getError() const {
    return m_error;
}

}  // namespace HubPlatform
