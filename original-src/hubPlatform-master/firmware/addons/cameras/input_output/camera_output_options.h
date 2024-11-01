#pragma once
#include <atomic>

namespace HubPlatform {

class CameraOutputOptions {
public:
    static void setRecordBufferSize( int value );
    static void setPreRecordBufferSize( int value );

    static int getRecordBufferSize();
    static int getPreRecordBufferSize();

private:
    static std::atomic_int recordBufferSize;
    static std::atomic_int preRecordBufferSize;
};

}  // namespace HubPlatform
