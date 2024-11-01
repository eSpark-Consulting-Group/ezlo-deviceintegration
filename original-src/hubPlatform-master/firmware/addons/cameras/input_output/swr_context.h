#pragma once

#include "cameras/codec/codec_context.h"

extern "C" {
#include <libswresample/swresample.h>
}

namespace HubPlatform {

class SwrContextWrapper {
public:
    SwrContextWrapper();
    SwrContextWrapper( SwrContext* );
    ~SwrContextWrapper();

    SwrContextWrapper( const SwrContextWrapper& ) = delete;
    SwrContextWrapper( SwrContextWrapper&& r ) noexcept;
    SwrContextWrapper& operator=( SwrContextWrapper&& r ) noexcept;

    SwrContext* getContext();
    const SwrContext* getContext() const;

private:
    SwrContext* m_swrContext = nullptr;
};

}  // namespace HubPlatform
