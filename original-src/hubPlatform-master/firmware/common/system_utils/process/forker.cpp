#include "system_utils/process/forker.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#include <sys/wait.h>
#include <string.h>
#include <fstream>
#include <future>

namespace HubPlatform {
namespace Process {

namespace {

void forkProcess() {
    // Fork off the parent process
    auto pid = fork();
    if( pid != 0 ) {
        // Let the parent terminate
        exit( pid < 0 ? EXIT_FAILURE : EXIT_SUCCESS );
    }
}

}  // namespace

void daemonize() {
    // TODO: use system daemon() function

    forkProcess();

    // On success: The child process becomes session leader
    if( setsid() < 0 ) {
        exit( EXIT_FAILURE );
    }

    // Fork off for the second time
    forkProcess();

    // Set new file permissions
    umask( 0 );

    // Close all open file descriptors
    for( auto fd = sysconf( _SC_OPEN_MAX ); fd >= 0; --fd ) {
        close( fd );
    }
}

constexpr int Forker::ExeclExitError;

Forker::~Forker() {
    m_isRunning = false;
    if( m_thread.joinable() ) {
        m_thread.join();
    }
}

Forker::Instance Forker::getInstance() {
    static std::mutex mutex;
    std::lock_guard< std::mutex > lock( mutex );
    static std::weak_ptr< Forker > exitingInstance;
    if( auto instance = exitingInstance.lock() ) {
        return instance;
    }
    else {
        Forker::Instance newInstance( new Forker() );
        exitingInstance = newInstance;
        return newInstance;
    }
}

pid_t Forker::fork( const std::string& shellCommand, Forker::OnProcessCompletion onProcessCompletion ) {
    if( shellCommand.empty() ) {
        throw ForkError( "empty shellCommand" );
    }
    return fork(
        [shellCommand]( const std::string& errorsOutputFile ) {
            const auto command = "execute() { " + shellCommand + "; }; execute 2> " + errorsOutputFile;
            execCommand( command );
        },
        onProcessCompletion );
}

pid_t Forker::fork( ProcessJob job, Forker::OnProcessCompletion onProcessCompletion ) {
    if( !onProcessCompletion ) {
        throw ForkError( "empty onProcessCompletion" );
    }

    {
        std::lock_guard< std::mutex > lock( m_mutex );
        if( !m_isRunning ) {
            m_isRunning = true;
            m_thread = std::thread( [this]() {
                while( m_isRunning ) {
                    waitForProcess();
                }
            } );
        }
    }

    char errorsOutputFile[] = "/tmp/tmp.XXXXXX";
    close( mkstemp( errorsOutputFile ) );

    pid_t pid = ::fork();
    if( pid > 0 ) {
        // old process gets pid of new child process
        std::lock_guard< std::mutex > lock( m_mutex );
        FileAutoRemover errorFileRemover( errorsOutputFile );
        m_processes.emplace( pid, ProcessData{ onProcessCompletion, std::move( errorFileRemover ) } );
    }
    else if( pid == 0 ) {
        // new process
        // write error message to file
        setsid(); /* obtain a new process group */
        for( int i = getdtablesize(); i >= 0; --i ) {
            close( i ); /* close all descriptors */
        }
        job( errorsOutputFile );
        ::_exit( 0 );
    }
    else if( pid < 0 ) {
        throw ForkError( strerror( errno ) );
    }
    return pid;
}

pid_t Forker::fork( const std::string& shellCommand ) {
    if( shellCommand.empty() ) {
        throw ForkError( "empty shellCommand" );
    }
    pid_t pid = ::fork();
    if( pid == 0 ) {
        // new process
        setsid(); /* obtain a new process group */
        for( int i = getdtablesize(); i >= 0; --i ) {
            close( i ); /* close all descriptors */
        }

        execCommand( shellCommand );
    }
    return pid;
}

void Forker::kill( pid_t pid, int signal ) {
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

void Forker::execCommand( const std::string& shellCommand ) {
    // new child process runs here with new pid
    ::execl( "/bin/sh", "sh", "-c", shellCommand.c_str(), nullptr );
    // if execl runs successfully it creates separate process and ::_exit never runs
    // in case of execl separate process is not creates and child process returns execlExitError
    ::_exit( ExeclExitError );  // will be called only in case of execl() error
}

void Forker::waitForProcess() {
    int waitStatus = 0;
    int result = 0;

    std::unique_lock< std::mutex > lock( m_mutex );
    for( auto& element : m_processes ) {
        auto& pid = element.first;
        result = ::waitpid( pid, &waitStatus, WNOHANG );
        if( result != 0 )  // process finished
            break;
    }
    lock.unlock();

    if( !m_isRunning ) {
        return;
    }

    if( 0 == result ) {
        // no child process exited: sleep, the repeat
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        return;
    }
    if( result < 0 ) {
        int lastError = errno;
        if( lastError != ECHILD ) {
            std::cerr << "waitpid error: " + std::to_string( lastError ) + std::string( ", text: " ) +
                             strerror( lastError )
                      << std::endl
                      << std::flush;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        return;
    }
    // pid of an exited process
    // Process has finished;
    pid_t pid = result;

    lock.lock();
    auto foundIt = m_processes.find( pid );
    if( foundIt == m_processes.end() ) {
        return;
    }
    auto onProcessCompletion = foundIt->second.onProcessCompletion;
    FileAutoRemover errorFileRemover = std::move( foundIt->second.errorFileRemover );
    m_processes.erase( foundIt );
    lock.unlock();

    std::string error;
    std::ifstream errorStream( errorFileRemover.getFileName().c_str() );
    if( errorStream.good() ) {
        error = { std::istreambuf_iterator< char >( errorStream ), std::istreambuf_iterator< char >() };
    }
    else {
        std::cerr << "error file was not opened" << std::endl << std::flush;
    }

    if( WIFEXITED( waitStatus ) == 0 ) {
        if( WIFSIGNALED( waitStatus ) ) {
            int termSignal = WTERMSIG( waitStatus );
            onProcessCompletion( pid, Status::TermSignal, termSignal, error );
            return;
        }
        else {
            onProcessCompletion( pid, Status::UnknownError, -1, error );
            return;
        }
    }
    int exitCode = WEXITSTATUS( waitStatus );
    if( exitCode != 0 ) {
        if( WIFSIGNALED( waitStatus ) ) {
            int termSignal = WTERMSIG( waitStatus );
            onProcessCompletion( pid, Status::TermSignal, termSignal, error );
            return;
        }
    }
    onProcessCompletion( pid, Status::Ok, exitCode, error );
}

ForkError::ForkError( const std::string& error ) noexcept
: m_error( error ) {}

const char* ForkError::what() const _GLIBCXX_USE_NOEXCEPT {
    return m_error.c_str();
};

Forker::FileAutoRemover::FileAutoRemover( std::string fileName )
: m_fileName( std::move( fileName ) ) {}

Forker::FileAutoRemover::FileAutoRemover( Forker::FileAutoRemover&& other ) noexcept
: m_fileName( std::move( other.m_fileName ) ) {}

Forker::FileAutoRemover::~FileAutoRemover() {
    if( !m_fileName.empty() ) {
        filesystem::removeFile( m_fileName.c_str() );
    }
}

const std::string& Forker::FileAutoRemover::getFileName() const {
    return m_fileName;
}

}  // namespace Process
}  // namespace HubPlatform
