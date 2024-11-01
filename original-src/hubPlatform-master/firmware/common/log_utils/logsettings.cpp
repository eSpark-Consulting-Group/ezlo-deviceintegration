
#include "log_utils/logsettings.h"
#include "system_utils/filesystem.h"
#include "log_definitions.h"

namespace ezlo {
namespace log_settings {

static volatile size_t g_nMinLogMessageSize = 256;
static volatile size_t g_nMaxLogMessageSize = 1024;
static volatile size_t g_nMinLogFileSize = 8 * 1024;
static volatile size_t g_nMaxLogFileDefaultSize = 500 * 1024 * (size_t) 1024;
static volatile size_t g_nMaxLogFilesDefaultCount = 10;
static volatile size_t g_nMaxAllowedDBRowsCount = 100000;
static volatile size_t g_nAutoCleanDBRowsCount = 100;

size_t maxLogMessageSizeGet() {
    return g_nMaxLogMessageSize;
}

void maxLogMessageSizeSet( const size_t nSize ) {
    if( nSize > g_nMinLogMessageSize )
        g_nMaxLogMessageSize = nSize;
}

size_t maxLogFileSizeGet() {
    return g_nMaxLogFileDefaultSize;
}

void maxLogFileSizeSet( const size_t nSize ) {
    if( nSize > g_nMinLogFileSize && nSize < g_nMaxLogFileDefaultSize )
        g_nMaxLogFileDefaultSize = nSize;
}

size_t maxLogFilesCountGet() {
    return g_nMaxLogFilesDefaultCount;
}

void maxLogFilesCountSet( const size_t filesCount ) {
    if( filesCount < g_nMaxLogFilesDefaultCount )
        g_nMaxLogFilesDefaultCount = filesCount;
}

void maxDBRowsCountSet( const size_t maxRowsCount ) {
    if( maxRowsCount < g_nMaxAllowedDBRowsCount )
        g_nMaxAllowedDBRowsCount = maxRowsCount;
}

size_t maxDBRowsCountGet() {
    return g_nMaxAllowedDBRowsCount;
}

void autoCleanDBRowsCountSet( const size_t autoCleanCount ) {
    if( autoCleanCount < g_nMaxAllowedDBRowsCount )
        g_nAutoCleanDBRowsCount = autoCleanCount;
}

size_t autoCleanDBRowsCountGet() {
    return g_nAutoCleanDBRowsCount;
}

static std::string g_strLogDirectory;

bool ensureLogDirectoryExist() {
    const auto& logDirectory = getLogDirectory();
    if( HubPlatform::filesystem::isDirectoryExist( logDirectory ) )
        return true;

    const std::string cmd{ "mkdir -p \"" + logDirectory + "\"" };
    ::system( cmd.c_str() );

    return HubPlatform::filesystem::isDirectoryExist( logDirectory );
}

std::string getLogDirectory() {  // default returned value is "/var/log/firmware/", note: "/" is present at the end
                                 // of directory path
    if( g_strLogDirectory.empty() ) {
        g_strLogDirectory = LoggerPath;
        ensureLogDirectoryExist();
    }
    return g_strLogDirectory;
}
bool setLogDirectory( const char* strLogDirectory ) {
    if( strLogDirectory == nullptr || strLogDirectory[ 0 ] == '\0' )
        return false;
    std::string s( strLogDirectory );
    size_t n = s.length();
    if( n == 0 )
        return false;
    if( s[ n - 1 ] != '/' )
        s += '/';
    g_strLogDirectory = s;
    return ensureLogDirectoryExist();
}
bool setLogDirectory( const std::string& strLogDirectory ) {
    return strLogDirectory.empty() ? false : setLogDirectory( strLogDirectory.c_str() );
}

};  // namespace log_settings
};  // namespace ezlo
