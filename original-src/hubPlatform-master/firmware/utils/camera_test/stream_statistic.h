#pragma once

#include <chrono>

struct StreamStatistic {
    int packetsCount{ 0 };
    bool firstVideoPacketIsKey{ false };

    std::chrono::milliseconds keyPacketTime{ 0 };
    std::chrono::milliseconds handshakeTime{ 0 };
};
