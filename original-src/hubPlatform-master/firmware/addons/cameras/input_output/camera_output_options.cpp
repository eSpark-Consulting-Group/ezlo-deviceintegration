#include "cameras/input_output/camera_output_options.h"

namespace HubPlatform {

std::atomic_int CameraOutputOptions::recordBufferSize( 0 );
std::atomic_int CameraOutputOptions::preRecordBufferSize( 0 );

void CameraOutputOptions::setRecordBufferSize( int value ) {
    recordBufferSize = value;
}

void CameraOutputOptions::setPreRecordBufferSize( int value ) {
    preRecordBufferSize = value;
}

int CameraOutputOptions::getRecordBufferSize() {
    return recordBufferSize;
}

int CameraOutputOptions::getPreRecordBufferSize() {
    return preRecordBufferSize;
}

}  // namespace HubPlatform
