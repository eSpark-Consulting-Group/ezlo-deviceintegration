#include "cameras/codec/codec_params.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <stdexcept>

namespace HubPlatform {

AVCodecParamsWrapper::AVCodecParamsWrapper( const AVCodecParameters* sourceCodecParams )
: m_codecParams( avcodec_parameters_alloc() ) {
    if( !m_codecParams ) {
        throw std::runtime_error( "avcodec_parameters_alloc() failed" );
    }
    if( avcodec_parameters_copy( m_codecParams, sourceCodecParams ) < 0 ) {
        avcodec_parameters_free( &m_codecParams );
        throw std::runtime_error( "avcodec_parameters_copy() failed" );
    }
}

AVCodecParamsWrapper::~AVCodecParamsWrapper() {
    avcodec_parameters_free( &m_codecParams );
}

AVCodecParamsWrapper::AVCodecParamsWrapper( const AVCodecParamsWrapper& right )
: AVCodecParamsWrapper( right.m_codecParams ) {}

AVCodecParamsWrapper::AVCodecParamsWrapper( AVCodecParamsWrapper&& right ) noexcept
: m_codecParams( right.m_codecParams ) {
    right.m_codecParams = nullptr;
}

AVCodecParamsWrapper& AVCodecParamsWrapper::operator=( AVCodecParamsWrapper&& right ) noexcept {
    if( &right == this )
        return *this;
    m_codecParams = right.m_codecParams;
    right.m_codecParams = nullptr;
    return *this;
}

AVCodecParameters* AVCodecParamsWrapper::get() {
    return m_codecParams;
}

const AVCodecParameters* AVCodecParamsWrapper::get() const {
    return m_codecParams;
}

}  // namespace HubPlatform
