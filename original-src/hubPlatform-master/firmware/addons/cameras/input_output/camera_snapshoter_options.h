#pragma once

#include <atomic>

namespace HubPlatform {

class CameraSnapshoterOptions {
public:
    static void setBufferSize( int value );
    static void setQueueLimit( int value );
    static void setTimeoutSec( int value );

    static int getBufferSize();
    static int getQueueLimit();
    static int getTimeoutSec();

private:
    static std::atomic_int bufferSize;
    static std::atomic_int queueLimit;
    static std::atomic_int timeoutSec;
};

}  // namespace HubPlatform
