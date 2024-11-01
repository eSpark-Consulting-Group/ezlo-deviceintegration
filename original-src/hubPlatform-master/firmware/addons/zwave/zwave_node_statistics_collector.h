#pragma once

#include "base_utils/circular_buffer.h"

#include <chrono>
#include <functional>
#include <mutex>

namespace HubPlatform {
namespace Zwave {

class NodeStatisticsCollector {
public:
    struct CommandInfo {
        uint16_t m_nodeId;
        enum class Status : uint8_t { Sent, Failed } m_status;
        std::chrono::system_clock::time_point m_commandSent;
        std::chrono::system_clock::duration m_delay;

        CommandInfo()
        : m_nodeId{ 0 }
        , m_status{ Status::Sent }
        , m_commandSent{}
        , m_delay{} {}

        CommandInfo( uint16_t nodeId,
                     Status status,
                     std::chrono::system_clock::time_point commandSent,
                     std::chrono::system_clock::duration delay = {} )
        : m_nodeId{ nodeId }
        , m_status{ status }
        , m_commandSent{ commandSent }
        , m_delay{ delay } {}
    };

public:
    NodeStatisticsCollector( size_t bufferMaxSize = 100 );

    void onNodeCommandDelay( const uint16_t nodeId,
                             std::chrono::system_clock::time_point commandSent,
                             std::chrono::system_clock::duration delay );

    void onNodeCommandFailed( const uint16_t nodeId, std::chrono::system_clock::time_point commandSent );

    void fetchStatisctics( std::function< void( const CommandInfo& ) > callback );

private:
    std::mutex m_mutex;
    common::CircularBuffer< CommandInfo > m_commandSentInfos;
};

}  // namespace Zwave
}  // namespace HubPlatform
