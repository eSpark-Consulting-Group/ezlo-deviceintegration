#pragma once
#include <atomic>

namespace HubPlatform {

class CameraFramedSourceOptions {
public:
    static void setBufferSize( int value );
    static int getBufferSize();

private:
    static std::atomic_int bufferSize;
};

}  // namespace HubPlatform
