#pragma once
#include <atomic>
#include <chrono>

namespace HubPlatform {

class CameraInputOptions {
public:
    static void setMaxDelayBetweenPacketsMsec( int value );
    static int getMaxDelayBetweenPacketsMsec();

    static void setPacketProcessingDelayMsec( int value );
    static int getPacketProcessingDelayMsec();

    static void setAccumulatedDelayThresholdMsec( int value );
    static int getAccumulatedDelayThresholdMsec();

private:
    static std::atomic_int maxDelayBetweenPacketsMsec;
    static std::atomic_int packetProcessingDelayMsec;
    static std::atomic_int accumulatedDelayThresholdMsec;
};

}  // namespace HubPlatform
