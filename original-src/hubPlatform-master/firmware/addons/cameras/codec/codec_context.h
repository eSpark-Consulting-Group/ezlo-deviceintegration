#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace HubPlatform {

class AVCodecContextWrapper {
public:
    AVCodecContextWrapper();
    explicit AVCodecContextWrapper( const AVCodec* codec );
    ~AVCodecContextWrapper();

    AVCodecContextWrapper( const AVCodecContextWrapper& ) = delete;
    AVCodecContextWrapper( AVCodecContextWrapper&& r ) noexcept;
    AVCodecContextWrapper& operator=( AVCodecContextWrapper&& r ) noexcept;

    AVCodecContext* getContext();
    const AVCodecContext* getContext() const;

private:
    AVCodecContext* m_codecContext;
};
}  // namespace HubPlatform
