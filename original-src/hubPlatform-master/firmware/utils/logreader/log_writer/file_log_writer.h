#pragma once
#include "log_writer_interface.h"

namespace HubPlatform {
namespace LogReader {

class FileLogWriter final : public LogWriterInterface {
public:
    FileLogWriter() = delete;
    FileLogWriter( const FileLogWriter& ) = delete;
    FileLogWriter( FileLogWriter&& ) = delete;
    FileLogWriter& operator=( const FileLogWriter& ) = delete;
    FileLogWriter& operator=( FileLogWriter&& ) = delete;

    FileLogWriter( std::string filePath );
    void sendLog( const std::string& data ) override;

private:
    const std::string m_filePath;
};

}  // namespace LogReader
}  // namespace HubPlatform