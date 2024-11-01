#include "cameras/rtsp_server/camera_framed_source_options.h"

namespace HubPlatform {

std::atomic_int CameraFramedSourceOptions::bufferSize( 0 );

void CameraFramedSourceOptions::setBufferSize( int value ) {
    bufferSize = value;
}

int CameraFramedSourceOptions::getBufferSize() {
    return bufferSize;
}

}  // namespace HubPlatform
