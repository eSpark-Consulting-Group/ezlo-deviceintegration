#include "cameras/input_output/swr_context.h"

namespace HubPlatform {

SwrContextWrapper::SwrContextWrapper()
: m_swrContext( swr_alloc() ) {}

SwrContextWrapper::SwrContextWrapper( SwrContext* context )
: m_swrContext( context ) {}

SwrContextWrapper::~SwrContextWrapper() {
    if( m_swrContext ) {
        swr_free( &m_swrContext );
    }
}

SwrContextWrapper::SwrContextWrapper( SwrContextWrapper&& r ) noexcept
: m_swrContext( r.m_swrContext ) {
    r.m_swrContext = nullptr;
}

SwrContextWrapper& SwrContextWrapper::operator=( SwrContextWrapper&& r ) noexcept {
    if( m_swrContext ) {
        swr_free( &m_swrContext );
    }
    m_swrContext = r.m_swrContext;
    r.m_swrContext = nullptr;
    return *this;
}

SwrContext* SwrContextWrapper::getContext() {
    return m_swrContext;
}

const SwrContext* SwrContextWrapper::getContext() const {
    return m_swrContext;
}

}  // namespace HubPlatform
