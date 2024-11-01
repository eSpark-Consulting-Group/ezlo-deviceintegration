#include "watcher/system_log/system_log.h"

#include "common_utils/cpp14.h"
#include "base_utils/string_utils.h"

#include <functional>

namespace HubPlatform {
namespace Watcher {

SystemLog::~SystemLog() {
    stop();
}

void SystemLog::subscribe( std::string subscriberId, OnLogRecord callback ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_subscribers.emplace( std::move( subscriberId ), std::move( callback ) );
}

void SystemLog::unsubscribe( std::string subscriberId ) {
    std::lock_guard< std::mutex > lock( m_mutex );
    m_subscribers.erase( subscriberId );
}

void SystemLog::start( std::string logCmd ) {
    if( m_processReader )
        return;

    auto stdoutEvent = [this]( const char* bytes, size_t n ) {
        m_buffer.append( bytes, n );
        auto lastLinePos = m_buffer.rfind( '\n' );
        if( lastLinePos == std::string::npos )
            return;

        auto lines = StringUtils::splitByLines( m_buffer.substr( 0, lastLinePos + 1 ) );

        std::lock_guard< std::mutex > lock( m_mutex );
        for( auto& line : lines ) {
            for( auto& element : m_subscribers ) {
                element.second( line );
            }
        }
        m_buffer = m_buffer.substr( lastLinePos + 1 );
    };

    auto stderrEvent = []( const char* bytes, size_t n ) {};

    m_processReader = std::make_unique< Process::ProcessReader >( logCmd, stdoutEvent, stderrEvent );
}

void SystemLog::stop() {
    if( m_processReader ) {
        m_processReader.reset();
    }
}

}  // namespace Watcher
}  // namespace HubPlatform
