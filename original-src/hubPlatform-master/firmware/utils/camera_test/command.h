#pragma once

#include <string>
#include <chrono>

struct Command {
    enum class Type { NONE, PRINT_HELP, PRINT_STREAMS_LIST, CONTINUOUS_STREAMING_TEST, KEY_FRAME_TEST };

public:
    Type type{ Type::NONE };
    int logLevel{ 0 };

    // only one of these values should be used
    int streamIdx{ -1 };
    std::string streamUrl;

    // used only if running "key-frame"
    std::chrono::seconds connectionDuration{ -1 };
    std::chrono::seconds delayDuration{ -1 };
    std::chrono::seconds totalTestDuration{ -1 };

    // used only if running "continuous-streaming"
    std::chrono::milliseconds packetDurationThreshold{ -1 };

public:
    // needed to prevent overflow in further usages of durations
    static constexpr std::chrono::seconds MaxDuration{ std::chrono::hours{ 240 } };

    static constexpr std::chrono::seconds DefaultKeyFrameTestConnectionDuration{ 2 };
    static constexpr std::chrono::seconds DefaultKeyFrameTestDelayDuration{ 0 };
    static constexpr std::chrono::seconds DefaultKeyFrameTestDuration{ MaxDuration };
    static constexpr std::chrono::milliseconds DefaultStreamingTestPacketDurationThreshold{ 300 };
};
