#include "cameras/codec/codec_context.h"
#include <utility>

namespace HubPlatform {

AVCodecContextWrapper::AVCodecContextWrapper()
: m_codecContext( avcodec_alloc_context3( nullptr ) ) {}

AVCodecContextWrapper::AVCodecContextWrapper( const AVCodec* codec )
: m_codecContext( avcodec_alloc_context3( codec ) ) {}

AVCodecContextWrapper::~AVCodecContextWrapper() {
    avcodec_free_context( &m_codecContext );
}

AVCodecContextWrapper::AVCodecContextWrapper( AVCodecContextWrapper&& r ) noexcept {
    this->operator=( std::move( r ) );
}

AVCodecContextWrapper& AVCodecContextWrapper::operator=( AVCodecContextWrapper&& r ) noexcept {
    if( this != &r ) {
        avcodec_free_context( &m_codecContext );
        m_codecContext = r.m_codecContext;
        r.m_codecContext = nullptr;
    }
    return *this;
}

AVCodecContext* AVCodecContextWrapper::getContext() {
    return m_codecContext;
}

const AVCodecContext* AVCodecContextWrapper::getContext() const {
    return m_codecContext;
}

}  // namespace HubPlatform
