#include "system_utils/process/process_reader.h"

#include <unistd.h>
#include <sys/wait.h>

namespace HubPlatform {
namespace Process {

constexpr int ExeclExitError{ 127 };
constexpr std::chrono::milliseconds ReadInterval{ 200 };

ProcessReader::ProcessReader( const std::string& cmd, OnData stdout, OnData stderr )
: m_stdout( std::move( stdout ) )
, m_stderr( std::move( stderr ) ) {
    ::pipe( m_stdoutPipe );
    ::pipe( m_stderrPipe );

    m_pid = ::fork();
    if( m_pid == 0 ) {
        // Close reading end of the pipes
        ::close( m_stdoutPipe[ 0 ] );
        ::close( m_stderrPipe[ 0 ] );

        // Redirect stdout/stderr to writing end of the pipe
        ::dup2( m_stdoutPipe[ 1 ], STDOUT_FILENO );
        ::dup2( m_stderrPipe[ 1 ], STDERR_FILENO );

        ::execl( "/bin/sh", "sh", "-c", cmd.c_str(), nullptr );
        ::_exit( ExeclExitError );  // will be called only in case of execl() error
    }
    else {
        // Close writing end of the pipes
        ::close( m_stdoutPipe[ 1 ] );
        ::close( m_stderrPipe[ 1 ] );

        m_readerThread = std::thread( &ProcessReader::run, this );
    }
}

ProcessReader::~ProcessReader() {
    stop();
    if( m_readerThread.joinable() )
        m_readerThread.join();
}

void ProcessReader::stop( std::chrono::milliseconds timeout ) {
    if( isFinished() )
        return;

    kill( m_pid, SIGTERM );
    for( std::chrono::milliseconds time{ 0 }; time <= timeout; time += std::chrono::milliseconds( 500 ) ) {
        if( isFinished() ) {
            return;
        }
    }
    kill( m_pid, SIGKILL );
}

void ProcessReader::run() {
    char stdoutBuff[ 1024 ];
    char stderrBuff[ 1024 ];

    ssize_t stdoutCount{ 0 };
    ssize_t stderrCount{ 0 };

    auto readStdout = [&]() {
        while( stdoutCount >= 0 && ( stdoutCount = read( m_stdoutPipe[ 0 ], stdoutBuff, sizeof( stdoutBuff ) ) ) > 0 ) {
            m_stdout( stdoutBuff, stdoutCount );
        }
    };

    auto readStderr = [&]() {
        while( stderrCount >= 0 && ( stderrCount = read( m_stderrPipe[ 0 ], stderrBuff, sizeof( stderrBuff ) ) ) > 0 ) {
            m_stderr( stderrBuff, stderrCount );
        }
    };

    while( true ) {
        int status = 0;
        int childProcessState = waitpid( m_pid, &status, WNOHANG );

        readStdout();
        readStderr();

        // Process exited
        if( childProcessState != 0 ) {
            readStdout();
            readStderr();
            if( WIFEXITED( status ) ) {
                m_exitCode = WEXITSTATUS( status );
            }
            break;
        }

        std::this_thread::sleep_for( ReadInterval );
    }
    m_isRunning = false;
}

bool ProcessReader::isFinished() {
    return !m_isRunning;
}

int ProcessReader::getExitCode() {
    return m_exitCode;
}

int ProcessReader::wait() {
    if( m_readerThread.joinable() )
        m_readerThread.join();
    return m_exitCode;
}

int ProcessReader::getPid() {
    return m_pid;
}

void ProcessReader::kill( pid_t pid, int signal ) {
    if( pid <= 0 ) {
        return;
    }
    const auto command = "kill_all_childs() {\n"
                         "    cpids=`pgrep -P $1|xargs`\n"
                         "    for cpid in $cpids;\n"
                         "    do\n"
                         "        kill_all_childs $cpid\n"
                         "    done\n"
                         "    kill -" +
                         std::to_string( signal ) +
                         " $1\n"
                         "}\n"
                         "kill_all_childs " +
                         std::to_string( pid ) +
                         "\n"
                         "kill -" +
                         std::to_string( signal ) + " " + std::to_string( pid ) + "\n";
    std::system( command.c_str() );
}

}  // namespace Process
}  // namespace HubPlatform
