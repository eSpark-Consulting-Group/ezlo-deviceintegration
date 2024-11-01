#pragma once

#include <string>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>

namespace HubPlatform {
namespace Process {

class ProcessReaderError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit ProcessReaderError( const std::string& msg )
    : runtime_error( msg ){};
};

class ProcessReader {
public:
    using OnData = std::function< void( const char* bytes, size_t n ) >;
    ProcessReader( const std::string& cmd, OnData stdout, OnData stderr );
    ~ProcessReader();

    void stop( std::chrono::milliseconds timeout = std::chrono::seconds{ 10 } );
    bool isFinished();
    int getExitCode();
    int wait();
    int getPid();

private:
    void run();
    void kill( pid_t pid, int signal );

private:
    OnData m_stdout;
    OnData m_stderr;

    int m_stdoutPipe[ 2 ];
    int m_stderrPipe[ 2 ];

    pid_t m_pid = 0;
    std::atomic_int m_exitCode{ -1 };
    std::atomic_int m_isRunning{ true };
    std::thread m_readerThread;
};

}  // namespace Process
}  // namespace HubPlatform
