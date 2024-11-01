#pragma once

#include "log_writer/log_writer_interface.h"
#include "log_utils/blocking_fixed_log_queue.h"
#include <thread>

namespace HubPlatform {
namespace LogReader {

namespace UT {
class LogManagerTestSuite;
}

class LogManager {
    friend class UT::LogManagerTestSuite;

public:
    LogManager( const LogManager& ) = delete;
    LogManager& operator=( const LogManager& ) = delete;
    LogManager( LogManager&& ) = delete;
    LogManager& operator=( LogManager&& ) = delete;

    explicit LogManager( std::vector< LogWriterInterfacePtr >& logWriters );
    explicit LogManager( std::vector< LogWriterInterfacePtr >& logWriters, const size_t queueLimit );
    void produceLog( const std::string& rawServiceLog );

    ~LogManager();

private:
    void init();
    void stop();
    void consumeLog( const size_t consumerIndex, LogWriterInterfacePtr logWriterPtr );

private:
    const size_t DEFAULT_LOG_LIMIT = 500;
    std::vector< LogWriterInterfacePtr >& m_logWriters;
    Container::BlockingFixedLogQueue m_logQueue;
    std::vector< std::thread > m_consumerThreads;
    std::atomic_bool m_loopConsume;
};
}  // namespace LogReader
}  // namespace HubPlatform
