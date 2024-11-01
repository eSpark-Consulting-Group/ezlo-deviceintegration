#pragma once

#include <memory>

namespace HubPlatform {
namespace LogReader {

class LogWriterInterface {
public:
    LogWriterInterface( const LogWriterInterface& ) = delete;
    LogWriterInterface( LogWriterInterface&& ) = delete;
    LogWriterInterface& operator=( const LogWriterInterface& ) = delete;
    LogWriterInterface& operator=( LogWriterInterface&& ) = delete;

    explicit LogWriterInterface() = default;
    virtual ~LogWriterInterface() = default;
    virtual void sendLog( const std::string& ) = 0;
};

using LogWriterInterfacePtr = std::shared_ptr< LogWriterInterface >;

}  // namespace LogReader
}  // namespace HubPlatform
