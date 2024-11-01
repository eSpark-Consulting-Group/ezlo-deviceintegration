#pragma once

#include <functional>
#include <stdexcept>
#include <utility>

namespace HubPlatform {
namespace LogReader {

using CallbackFunc = std::function< void( std::string ) >;
class LogReaderInterface {
public:
    LogReaderInterface( LogReaderInterface& ) = delete;
    LogReaderInterface( LogReaderInterface&& ) = delete;
    LogReaderInterface& operator=( const LogReaderInterface& ) = delete;
    LogReaderInterface& operator=( LogReaderInterface&& ) = delete;

    LogReaderInterface() = default;
    virtual void run( const CallbackFunc callbackFunc ) {
        if( !callbackFunc ) {
            throw std::invalid_argument( "Function is null" );
        }
    }
};
}  // namespace LogReader
}  // namespace HubPlatform