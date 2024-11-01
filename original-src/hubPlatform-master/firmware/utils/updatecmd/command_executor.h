#pragma once

#include <functional>
#include <vector>
#include <string>
#include <ostream>
#include <chrono>
#include <utility>
#include <memory>

namespace HubPlatform {
namespace UpdateCommand {

class CommandExecutor {
public:
    using Executor = std::function< std::pair< std::string, int >( std::string ) >;
    enum class Status { Ok, Failed, Hanged };

    CommandExecutor( std::ostream* logStream = nullptr );
    CommandExecutor( Executor executor, std::ostream* logStream = nullptr );

    Status run( const std::vector< std::string >& commands, std::chrono::milliseconds timeout );

private:
    std::shared_ptr< Executor > m_executorPtr;
    std::ostream* m_logStream = nullptr;
};

}  // namespace UpdateCommand
}  // namespace HubPlatform
