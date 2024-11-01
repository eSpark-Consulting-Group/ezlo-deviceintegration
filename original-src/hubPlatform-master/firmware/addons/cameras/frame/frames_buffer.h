#pragma once

#include "cameras/packet/packet.h"
#include "cameras/defs.h"

#include <queue>
#include <mutex>

namespace HubPlatform {

class FramesBuffer {
public:
    using Frame = FrameWrapperSharedPtr;

    bool appendElement( const Frame& frame );
    bool getFrontElement( Frame& frame );
    bool empty() const;
    void clear();

private:
    mutable std::mutex m_queueMutex;
    Frame m_frame;
};

}  // namespace HubPlatform
