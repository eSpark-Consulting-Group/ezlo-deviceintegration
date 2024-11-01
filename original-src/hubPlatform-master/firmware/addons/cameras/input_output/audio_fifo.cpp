#include "cameras/input_output/audio_fifo.h"

namespace HubPlatform {

AVAudioFifoWrapper::AVAudioFifoWrapper()
: m_audioFifo( nullptr ) {}

AVAudioFifoWrapper::AVAudioFifoWrapper( AVSampleFormat sampleFormat, int channels, int samplesNumber )
: m_audioFifo( av_audio_fifo_alloc( sampleFormat, channels, samplesNumber ) ) {}

AVAudioFifoWrapper::~AVAudioFifoWrapper() {
    if( m_audioFifo ) {
        av_audio_fifo_free( m_audioFifo );
    }
}

AVAudioFifoWrapper::AVAudioFifoWrapper( AVAudioFifoWrapper&& r ) noexcept
: m_audioFifo( r.m_audioFifo ) {
    r.m_audioFifo = nullptr;
}

AVAudioFifoWrapper& AVAudioFifoWrapper::operator=( AVAudioFifoWrapper&& r ) noexcept {
    if( m_audioFifo ) {
        av_audio_fifo_free( m_audioFifo );
    }
    m_audioFifo = r.m_audioFifo;
    r.m_audioFifo = nullptr;
    return *this;
}

AVAudioFifo* AVAudioFifoWrapper::getFifo() {
    return m_audioFifo;
}

const AVAudioFifo* AVAudioFifoWrapper::getFifo() const {
    return m_audioFifo;
}

}  // namespace HubPlatform
