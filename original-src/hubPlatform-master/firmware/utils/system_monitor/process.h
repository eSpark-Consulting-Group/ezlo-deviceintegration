#pragma once

#include <string>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

namespace HubPlatform {
namespace SystemMonitor {

class ProcessError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    explicit ProcessError( const std::string& msg )
    : runtime_error( msg ){};
};

class Process {
public:
    Process( std::string cmd, std::string pidfile, std::string logfile, std::chrono::milliseconds repeatInterval );
    Process( const Process& ) = delete;
    Process( Process&& ) = delete;  // std::mutex not movable or copyable
    ~Process();
    bool isRunning();
    int getPid();
    void stop( std::chrono::milliseconds killTimeout = std::chrono::seconds( 10 ) );

private:
    bool isProcessAlive();
    void run();
    void launchProcess();

private:
    std::string m_cmd;
    std::string m_pidfile;
    std::string m_logfile;
    std::chrono::milliseconds m_repeatInterval;
    std::mutex m_processMutex;
    int m_pid = 0;
    std::atomic_bool m_isRunning{ false };
    std::thread m_thread;

    std::mutex m_cvMutex;
    std::condition_variable m_cv;
    std::atomic_bool m_canRun{ true };
};

}  // namespace SystemMonitor
}  // namespace HubPlatform
