#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

namespace HubPlatform {

class AVFrameWrapper {
public:
    AVFrameWrapper();
    AVFrameWrapper( const AVFrameWrapper& ) = delete;
    AVFrameWrapper( AVFrameWrapper&& r ) noexcept;
    AVFrameWrapper& operator=( AVFrameWrapper&& r ) noexcept;

    ~AVFrameWrapper();

    AVFrame* getFrame();
    const AVFrame* getFrame() const;

private:
    AVFrame* m_frame = nullptr;
};
}  // namespace HubPlatform
