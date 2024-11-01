#include "log_manager.h"
#include "log_writer_interface.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace LogReader {

LogManager::LogManager( std::vector< LogWriterInterfacePtr >& logWriters )
: m_loopConsume( true )
, m_logWriters( logWriters )
, m_logQueue( DEFAULT_LOG_LIMIT, m_logWriters.size() ) {
    init();
}

LogManager::LogManager( std::vector< LogWriterInterfacePtr >& logWriters, const size_t queueLimit )
: m_loopConsume( true )
, m_logWriters( logWriters )
, m_logQueue( queueLimit == 0 ? DEFAULT_LOG_LIMIT : queueLimit, m_logWriters.size() ) {
    init();
}

void LogManager::init() {
    for( size_t index = 0; index < m_logWriters.size(); index++ ) {
        m_consumerThreads.emplace_back( &LogManager::consumeLog, this, index, m_logWriters[ index ] );
    }
}

void LogManager::stop() {
    m_loopConsume = false;
    m_logQueue.unblockConsumer();
}

LogManager::~LogManager() {
    stop();
    for( std::thread& it : m_consumerThreads ) {
        if( it.joinable() ) {
            it.join();
        }
    }
}

void LogManager::consumeLog( const size_t consumerIndex, LogWriterInterfacePtr logWriterPtr ) {
    bool result = false;
    do {
        try {
            std::string rawServiceLog;
            result = false;
            if( m_logQueue.pop( consumerIndex, rawServiceLog ) ) {
                logWriterPtr->sendLog( rawServiceLog );
                result = true;
            }
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Error caught while sending log :" ) + cc::note( ex.what() ) );
        }
    } while( m_loopConsume || result );
}

void LogManager::produceLog( const std::string& rawServiceLog ) {
    m_logQueue.push( rawServiceLog );
}
}  // namespace LogReader
}  // namespace HubPlatform
