#include "cameras/frame/frame.h"
#include <memory>

namespace HubPlatform {

AVFrameWrapper::AVFrameWrapper()
: m_frame( av_frame_alloc() ) {}

AVFrameWrapper::AVFrameWrapper( AVFrameWrapper&& r ) noexcept {
    this->operator=( std::move( r ) );
}

AVFrameWrapper& AVFrameWrapper::operator=( AVFrameWrapper&& r ) noexcept {
    if( m_frame ) {
        av_frame_free( &m_frame );
    }
    m_frame = r.m_frame;
    r.m_frame = nullptr;
    return *this;
}

AVFrameWrapper::~AVFrameWrapper() {
    if( m_frame ) {
        av_frame_free( &m_frame );
    }
}

AVFrame* AVFrameWrapper::getFrame() {
    return m_frame;
}

const AVFrame* AVFrameWrapper::getFrame() const {
    return m_frame;
}

}  // namespace HubPlatform
