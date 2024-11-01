#include "zwave_node_statistics_collector.h"

namespace HubPlatform {
namespace Zwave {

NodeStatisticsCollector::NodeStatisticsCollector( size_t bufferMaxSize )
: m_commandSentInfos( bufferMaxSize ) {}

void NodeStatisticsCollector::onNodeCommandDelay( const uint16_t nodeId,
                                                  std::chrono::system_clock::time_point commandSent,
                                                  std::chrono::system_clock::duration delay ) {
    std::unique_lock< std::mutex > lock( m_mutex );

    m_commandSentInfos.push( CommandInfo{ nodeId, CommandInfo::Status::Sent, commandSent, delay } );
}

void NodeStatisticsCollector::onNodeCommandFailed( const uint16_t nodeId,
                                                   std::chrono::system_clock::time_point commandSent ) {
    std::unique_lock< std::mutex > lock( m_mutex );

    m_commandSentInfos.push( CommandInfo{ nodeId, CommandInfo::Status::Failed, commandSent } );
}

void NodeStatisticsCollector::fetchStatisctics( std::function< void( const CommandInfo& ) > callback ) {
    std::unique_lock< std::mutex > lock( m_mutex );

    while( auto info = m_commandSentInfos.pop() ) {
        callback( *info );
    }
}

}  // namespace Zwave
}  // namespace HubPlatform
