#include "file_log_writer.h"
#include "system_utils/filesystem.h"
#include "common_utils/cpp14.h"

#include <fstream>
#include <iostream>
#include <memory>

namespace HubPlatform {
namespace LogReader {

FileLogWriter::FileLogWriter( std::string filePath )
: m_filePath( std::move( filePath ) ) {}

void FileLogWriter::sendLog( const std::string& data ) {
    if( !filesystem::isExist( filesystem::getPath( m_filePath ) ) ) {
        filesystem::createFolder( filesystem::getPath( m_filePath ) );
    }

    std::unique_ptr< std::ofstream > fileWriter =
        std::make_unique< std::ofstream >( m_filePath, std::ios::out | std::ios::app );
    if( fileWriter->is_open() ) {
        try {
            *fileWriter << data << std::endl;
            fileWriter->close();
        }
        catch( ... ) {
            fileWriter->close();
        }
    }
}
}  // namespace LogReader
}  // namespace HubPlatform