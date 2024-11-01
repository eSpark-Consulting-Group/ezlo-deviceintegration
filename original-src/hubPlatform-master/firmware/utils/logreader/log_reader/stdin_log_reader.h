#pragma once

#include "log_reader_interface.h"

#include <iostream>

namespace HubPlatform {
namespace LogReader {

class StdInLogReader : public LogReaderInterface {
public:
    StdInLogReader( const StdInLogReader& ) = delete;
    StdInLogReader( StdInLogReader&& ) = delete;
    StdInLogReader& operator=( const StdInLogReader& ) = delete;
    StdInLogReader& operator=( StdInLogReader&& ) = delete;

    StdInLogReader() = default;
    void run( const CallbackFunc callbackFunc ) override;
};
}  // namespace LogReader
}  // namespace HubPlatform