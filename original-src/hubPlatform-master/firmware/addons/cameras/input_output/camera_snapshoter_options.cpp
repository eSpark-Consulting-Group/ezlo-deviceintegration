#include "cameras/input_output/camera_snapshoter_options.h"

namespace HubPlatform {

std::atomic_int CameraSnapshoterOptions::bufferSize( 0 );
std::atomic_int CameraSnapshoterOptions::queueLimit( 0 );
std::atomic_int CameraSnapshoterOptions::timeoutSec( 0 );

void CameraSnapshoterOptions::setBufferSize( int value ) {
    bufferSize = value;
}

void CameraSnapshoterOptions::setQueueLimit( int value ) {
    queueLimit = value;
}

void CameraSnapshoterOptions::setTimeoutSec( int value ) {
    timeoutSec = value;
}

int CameraSnapshoterOptions::getBufferSize() {
    return timeoutSec;
}

int CameraSnapshoterOptions::getQueueLimit() {
    return queueLimit;
}

int CameraSnapshoterOptions::getTimeoutSec() {
    return timeoutSec;
}

}  // namespace HubPlatform
