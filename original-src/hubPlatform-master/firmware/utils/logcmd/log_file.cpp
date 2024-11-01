#include "logcmd/log_file.h"

#include <set>
#include <cctype>

#include "logcmd/log_exceptions.h"

namespace HubPlatform {
namespace LogCmd {

namespace {
// NOTE: it would be much easier to use regex but it doesnt implemented on all toolchains
bool isRegularLogFile( const std::string& filename ) {
    static const std::string suffix{ ".log" };
    static size_t suffixLen = suffix.size();

    if( filename.size() <= suffixLen )
        return false;

    if( 0 != filename.compare( filename.size() - suffixLen, suffixLen, suffix ) )
        return false;

    return true;
}

bool validSymbol( const char& c ) {
    static const std::set< char > validSymbols{ '-', '.', '_' };
    return validSymbols.count( c );
}

bool isValidFilename( const std::string& filename ) {
    if( filename.empty() )
        return false;

    for( const char& c : filename ) {
        if( !std::isalnum( c ) && !validSymbol( c ) )
            return false;
    }
    return true;
}

bool isLogArchive( const std::string& name ) {
    static const std::string suffix{ "logs.tgz" };
    static size_t suffixLen = suffix.size();

    if( name.size() <= suffixLen )
        return false;

    if( 0 != name.compare( name.size() - suffixLen, suffixLen, suffix ) )
        return false;

    return true;
}

}  // namespace

LogFile::LogType LogFile::type() const {
    return m_type;
}

const std::string& LogFile::dirName() const {
    return m_dirName;
}

const std::string& LogFile::fileName() const {
    return m_fileName;
}

const std::string& LogFile::logName() const {
    if( m_type == LogType::Archive )
        throw LogFileError( "Log archive does not have logName, use fileName instead" );
    return m_logName;
}

size_t LogFile::partNum() const {
    return m_partNum;
}

std::string LogFile::path() const {
    return m_dirName + "/" + m_fileName;
}

std::shared_ptr< LogFile > LogFile::makeLogFile( std::string dirName, std::string fileName ) {
    if( fileName.empty() )
        return nullptr;

    auto logFile = std::make_shared< LogFile >();

    logFile->m_dirName = std::move( dirName );
    if( logFile->m_dirName.size() > 1 && logFile->m_dirName.back() == '/' )
        logFile->m_dirName.pop_back();

    if( isLogArchive( fileName ) ) {
        logFile->m_type = LogFile::LogType::Archive;
        logFile->m_fileName = std::move( fileName );
        return logFile;
    }

    // rotated log
    if( std::isdigit( fileName.back() ) ) {
        if( !isRotatedLogFile( fileName ) )
            return nullptr;

        if( !isValidFilename( fileName ) )
            return nullptr;

        logFile->m_type = LogFile::LogType::Rotated;

        auto pos = fileName.rfind( '.' );
        const std::string suffixWithDigits = fileName.substr( pos + 1 );
        logFile->m_partNum = std::stoi( suffixWithDigits );

        logFile->m_logName = fileName.substr( 0, pos );
        logFile->m_fileName = std::move( fileName );

        const char dateSuffixPattern[] = "mmddHHMMSS";
        logFile->m_hasDateSuffix = suffixWithDigits.size() >= sizeof( dateSuffixPattern ) - 1;
        return logFile;
    }

    if( isRegularLogFile( fileName ) ) {
        if( !isValidFilename( fileName ) )
            return nullptr;

        logFile->m_type = LogFile::LogType::Regular;
        logFile->m_logName = fileName;
        logFile->m_fileName = std::move( fileName );
        return logFile;
    }

    return nullptr;
}

bool LogFile::isRotatedLogFile( const std::string& filename ) {
    static const std::string suffix{ ".log." };

    if( filename.size() <= suffix.size() )
        return false;

    auto suffixPos = filename.rfind( suffix );
    if( suffixPos == std::string::npos )
        return false;

    if( suffixPos == 0 )  // empty basename
        return false;

    std::string digits = filename.substr( suffixPos + suffix.size() );
    if( digits.empty() )
        return false;

    for( auto& digit : digits ) {
        if( false == std::isdigit( digit ) )
            return false;
    }

    return true;
}

/** Makes rotated file ane compatible with isRotatedLogFile checker */
std::string LogFile::makeRotatedLogFileName( const std::string& fileName,
                                             const std::string& extension,
                                             const std::string& rotatedDigits ) {
    return fileName + "." + extension + "." + rotatedDigits;
}

bool LogFile::hasDateSuffix() const {
    return m_hasDateSuffix;
}

}  // namespace LogCmd
}  // namespace HubPlatform
