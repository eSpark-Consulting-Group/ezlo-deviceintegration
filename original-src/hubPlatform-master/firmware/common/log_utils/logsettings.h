#pragma once

#include <stddef.h>
#include <string>

namespace ezlo {
namespace log_settings {

extern size_t maxLogMessageSizeGet();
extern void maxLogMessageSizeSet( const size_t nSize );
extern size_t maxLogFileSizeGet();
extern void maxLogFileSizeSet( const size_t nSize );
extern size_t maxLogFilesCountGet();
extern void maxLogFilesCountSet( const size_t filesCount );
extern size_t maxDBRowsCountGet();
extern void maxDBRowsCountSet( const size_t maxRowsCount );
extern size_t autoCleanDBRowsCountGet();
extern void autoCleanDBRowsCountSet( const size_t autoCleanCount );

extern bool ensureLogDirectoryExist();
extern std::string getLogDirectory();  // default returned value is "/var/log/firmware/", note: "/" is present at
                                       // the end of returned directory path

extern bool setLogDirectory( const char* strLogDirectory );
extern bool setLogDirectory( const std::string& strLogDirectory );

};  // namespace log_settings
};  // namespace ezlo
