#include "system_monitor/process.h"

#include "utils.h"

#include "base_utils/utils.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <thread>
#include <atomic>
#include <condition_variable>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace SystemMonitor {

Process::Process( std::string cmd, std::string pidfile, std::string logfile, std::chrono::milliseconds repeatInterval )
: m_cmd( std::move( cmd ) )
, m_pidfile( std::move( pidfile ) )
, m_logfile( std::move( logfile ) )
, m_repeatInterval( repeatInterval ) {
    if( m_cmd.empty() )
        throw ProcessError( "Empty process command" );
    if( m_pidfile.empty() )
        throw ProcessError( "Empty pidfile for command: " + m_cmd );
    if( m_logfile.empty() )
        throw ProcessError( "Empty logfile for command: " + m_cmd );

    m_thread = std::thread( &Process::run, this );
}

Process::~Process() {
    stop();
}

bool Process::isRunning() {
    return m_isRunning;
}

bool Process::isProcessAlive() {
    std::lock_guard< std::mutex > lock( m_processMutex );
    if( m_pid <= 0 )
        return false;

    int status = 0;
    int childProcessState = waitpid( m_pid, &status, WNOHANG );

    // Process exited
    if( childProcessState != 0 ) {
        return false;
    }
    return true;
}

int Process::getPid() {
    std::lock_guard< std::mutex > processLock( m_processMutex );
    int pid = m_pid;
    return pid;
}

void Process::stop( std::chrono::milliseconds killTimeout ) {
    {
        std::unique_lock< std::mutex > lock( m_cvMutex );
        m_canRun = false;
        m_cv.notify_one();
    }

    {
        if( isProcessAlive() ) {
            {
                std::lock_guard< std::mutex > processLock( m_processMutex );
                ::kill( m_pid, SIGTERM );
            }

            waitForCondition( [this]() { return !isProcessAlive(); }, killTimeout, std::chrono::milliseconds{ 200 } );
            if( isProcessAlive() ) {
                std::lock_guard< std::mutex > processLock( m_processMutex );
                LOGGER_WARN( cc::warn( "Process is not responding to SIGTERM, sending SIGKILL to pid: " ) +
                             cc::num10( m_pid ) );
                ::kill( m_pid, SIGKILL );
            }
        }
    }

    if( m_thread.joinable() )
        m_thread.join();
}

void Process::run() {
    while( m_canRun ) {
        launchProcess();

        if( m_pid > 0 )
            Utils::storeProcessPid( m_pid, m_pidfile );

        while( m_canRun ) {
            if( isProcessAlive() ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
                continue;
            }
            std::lock_guard< std::mutex > lock( m_processMutex );
            m_pid = 0;
            m_isRunning = false;
            break;
        }

        std::unique_lock< std::mutex > lock( m_cvMutex );
        m_cv.wait_for( lock, m_repeatInterval, [&]() { return !m_canRun.load(); } );
    }

    if( !isProcessAlive() ) {
        m_pid = 0;
        m_isRunning = false;
    }
}

void Process::launchProcess() {
    Utils::addLogHeader( m_logfile );
    std::string command = m_cmd + " >> " + m_logfile + " 2>&1";

    std::lock_guard< std::mutex > lock( m_processMutex );
    m_pid = Utils::launchCommand( std::move( command ) );
    if( m_pid > 0 ) {
        m_isRunning = true;
    }
}

}  // namespace SystemMonitor
}  // namespace HubPlatform
