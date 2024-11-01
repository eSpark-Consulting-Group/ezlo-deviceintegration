#include "updatecmd/command_executor.h"
#include "updatecmd/log.h"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <iostream>

#include "system_utils/system.h"

namespace HubPlatform {
namespace UpdateCommand {
namespace {

struct CommandExecutorHandler {
    std::mutex mutex;
    std::atomic_bool isFinished{ false };
    std::condition_variable condition;
    std::atomic< CommandExecutor::Status > status{ CommandExecutor::Status::Ok };
};

const CommandExecutor::Executor defaultExecutor = []( const std::string& command ) {
    auto packedExitCode = System::getCommandOutput( command.c_str(), true /*catchStderr*/ );
    packedExitCode.second = System::unpackExitCode( packedExitCode.second ).exitCode;
    return packedExitCode;
};

}  // namespace

CommandExecutor::CommandExecutor( std::ostream* logStream )
: CommandExecutor( defaultExecutor, logStream ){};

CommandExecutor::CommandExecutor( Executor executor, std::ostream* logStream )
: m_executorPtr( std::make_shared< Executor >( std::move( executor ) ) )
, m_logStream( logStream ) {}

CommandExecutor::Status CommandExecutor::run( const std::vector< std::string >& commands,
                                              std::chrono::milliseconds timeout ) {
    if( commands.empty() )
        return Status::Ok;

    std::ostream& log = ( m_logStream ) ? *m_logStream : std::cout;

    auto handler = std::make_shared< CommandExecutorHandler >();

    auto threadFunc = [&, handler]() {
        auto executorPtr = m_executorPtr;
        auto& executor = *executorPtr;
        for( auto& command : commands ) {
            LOG_INFO( "Executing: " + command );
            auto result = executor( command );
            if( handler->status == Status::Hanged )
                break;

            LOG_INFO( result.first );

            if( result.second != 0 ) {
                LOG_WARN( "Command failed with return code: " + std::to_string( result.second ) );
                handler->status = Status::Failed;
                break;
            }
        }

        handler->isFinished = true;
        handler->condition.notify_all();
    };

    std::thread thread( threadFunc );

    std::unique_lock< std::mutex > lock( handler->mutex );
    handler->condition.wait_for( lock, timeout, [&] { return handler->isFinished.load(); } );

    if( false == handler->isFinished ) {
        handler->status = Status::Hanged;
        LOG_WARN( "Command hanged. Detaching thread." );
        thread.detach();
    }
    else {
        size_t retry = 100;
        for( ; retry > 0; --retry ) {
            if( thread.joinable() ) {
                thread.join();
                break;
            }
            else {
                std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            }
        }
        if( retry == 0 ) {
            LOG_WARN( "Unable to join thread. Detaching hanged thread." );
            thread.detach();
        }
    }

    return handler->status;
}

}  // namespace UpdateCommand
}  // namespace HubPlatform
