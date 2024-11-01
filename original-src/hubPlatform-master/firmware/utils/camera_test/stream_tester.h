#pragma once

#include "camera_test/stream_statistic.h"

#include <chrono>
#include <string>
#include <functional>
#include <unordered_map>

struct AVPacket;
struct AVFormatContext;

class StreamTester {
public:
    using OnPacketDurationThresholdExceeded = std::function< void( std::chrono::milliseconds ) >;

    struct TestResult {
        bool hasError{ false };
        std::string errorMessage;

        StreamStatistic statistic;
    };

public:
    StreamTester( std::string url,
                  std::chrono::seconds connectionDuration,
                  std::chrono::milliseconds packetDurationThreshold,
                  OnPacketDurationThresholdExceeded packetDurationCallback );
    ~StreamTester();

    TestResult runTest();

private:
    bool initStream( std::string& errorMessage );
    void processPacketDuration( const AVPacket& packet );

private:
    std::string m_url;
    std::chrono::seconds m_connectionDuration{ 0 };

    std::chrono::milliseconds m_packetDurationThreshold{ 0 };
    std::unordered_map< int, int64_t > m_lastDtsPerStream;
    OnPacketDurationThresholdExceeded m_onPacketDurationCallback;

    int m_videoStreamIdx{ -1 };
    AVFormatContext* m_inputContext{ nullptr };
};
