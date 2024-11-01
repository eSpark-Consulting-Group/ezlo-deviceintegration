#pragma once
#include "log_writer_interface.h"

#include <iostream>

namespace HubPlatform {
namespace LogReader {

class StdOutLogWriter final : public LogWriterInterface {
public:
    StdOutLogWriter( const StdOutLogWriter& ) = delete;
    StdOutLogWriter( StdOutLogWriter&& ) = delete;
    StdOutLogWriter& operator=( const StdOutLogWriter& ) = delete;
    StdOutLogWriter& operator=( StdOutLogWriter&& ) = delete;

    explicit StdOutLogWriter() = default;
    void sendLog( const std::string& data ) override {
        // this is cout because this method should be write data to stdout as it is
        std::cout << data << std::endl;
    }
};

}  // namespace LogReader
}  // namespace HubPlatform
