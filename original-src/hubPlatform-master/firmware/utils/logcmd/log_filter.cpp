#include "log_filter.h"

#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>

#include <cctype>
#include <set>
#include <iostream>

namespace HubPlatform {
namespace LogCmd {

bool LogFilterAny::operator()( const LogFile& logFile ) const {
    return logFile.type() == LogFile::LogType::Regular || logFile.type() == LogFile::LogType::Rotated;
}

bool LogFilterRegular::operator()( const LogFile& logFile ) const {
    return logFile.type() == LogFile::LogType::Regular;
}

bool LogFilterRotated::operator()( const LogFile& logFile ) const {
    return logFile.type() == LogFile::LogType::Rotated;
}

bool LogFilterSelected::operator()( const LogFile& logFile ) const {
    return m_selectedNames.count( logFile.fileName() );
}

LogFilterSelected::LogFilterSelected( const std::vector< std::string >& selectedNames ) {
    LogFilterAny isLogFile;
    for( auto& name : selectedNames ) {
        auto logFile = LogFile::makeLogFile( "", name );
        if( logFile )
            m_selectedNames.emplace( name );
        else {
            LOGGER_WARN_RAW( cc::warn( "Invalid log name: " ) + cc::note( name ) );
        }
    }
}

bool LogFilterArchives::operator()( const LogFile& logFile ) const {
    return logFile.type() == LogFile::LogType::Archive;
}

}  // namespace LogCmd
}  // namespace HubPlatform
