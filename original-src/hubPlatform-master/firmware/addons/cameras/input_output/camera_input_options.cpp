#include "cameras/input_output/camera_input_options.h"
#include "log_utils/ezlo_log.h"

namespace HubPlatform {

std::atomic_int CameraInputOptions::maxDelayBetweenPacketsMsec( 0 );
std::atomic_int CameraInputOptions::packetProcessingDelayMsec( 0 );
std::atomic_int CameraInputOptions::accumulatedDelayThresholdMsec( 0 );

void CameraInputOptions::setMaxDelayBetweenPacketsMsec( int value ) {
    maxDelayBetweenPacketsMsec = value;
}

int CameraInputOptions::getMaxDelayBetweenPacketsMsec() {
    return maxDelayBetweenPacketsMsec;
}

void CameraInputOptions::setPacketProcessingDelayMsec( int value ) {
    packetProcessingDelayMsec = value;
}

int CameraInputOptions::getPacketProcessingDelayMsec() {
    return packetProcessingDelayMsec;
}

void CameraInputOptions::setAccumulatedDelayThresholdMsec( int value ) {
    accumulatedDelayThresholdMsec = value;
}

int CameraInputOptions::getAccumulatedDelayThresholdMsec() {
    return accumulatedDelayThresholdMsec;
}

}  // namespace HubPlatform
