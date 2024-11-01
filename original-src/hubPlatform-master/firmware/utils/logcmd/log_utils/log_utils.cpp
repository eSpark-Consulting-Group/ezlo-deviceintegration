#include "logcmd/log_utils/log_utils.h"

#include "logcmd/log_exceptions.h"
#include "logcmd/log_filter.h"
#include "system_utils/system.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <map>
#include <iomanip>

namespace HubPlatform {
namespace LogCmd {
namespace LogUtils {

namespace {

LogFiles findLogs( IFilesystem& filesystem, const std::string& directoryPath, const LogFilter& filter ) {
    if( !filesystem.isDirectoryExist( directoryPath ) )
        throw LogDirectoryDoesNotExistError( "Log directory doesn't exist: " + directoryPath );

    auto files = filesystem.findFiles( directoryPath );

    LogFiles filtered;
    for( auto& fileName : files ) {
        auto logFile = LogFile::makeLogFile( directoryPath, fileName );
        if( logFile && filter( *logFile ) )
            filtered.emplace_back( std::move( logFile ) );
    }
    return filtered;
}

}  // namespace

LogFiles findLogs( IFilesystem& filesystem, const std::string& directoryPath ) {
    LogFilterAny filter;
    return findLogs( filesystem, directoryPath, filter );
}

LogFiles findRegularLogs( IFilesystem& filesystem, const std::string& directoryPath ) {
    LogFilterRegular filter;
    return findLogs( filesystem, directoryPath, filter );
}

LogFiles findRotatedLogs( IFilesystem& filesystem, const std::string& directoryPath ) {
    LogFilterRotated filter;
    return findLogs( filesystem, directoryPath, filter );
}

LogFiles findArchivedLogs( IFilesystem& filesystem, const std::string& directoryPath ) {
    LogFilterArchives filter;
    return findLogs( filesystem, directoryPath, filter );
}

LogFiles findSelectedLogs( IFilesystem& filesystem,
                           const std::string& directoryPath,
                           const std::vector< std::string >& names ) {
    LogFilterSelected filter( names );
    return findLogs( filesystem, directoryPath, filter );
}

LogFiles joinLogFiles( const LogFiles& listA, const LogFiles& listB ) {
    LogFiles merged;
    for( auto& file : listA )
        merged.emplace_back( file );
    for( auto& file : listB )
        merged.emplace_back( file );
    return merged;
}

PartitionMap buildPartitionMap( const LogFiles& logFiles ) {
    static const std::string suffix{ ".log." };

    PartitionMap map;
    LogFilterRotated isRotated;

    for( auto& logFile : logFiles ) {
        if( !logFile )
            continue;

        if( !isRotated( *logFile ) )
            continue;

        auto it = map.find( logFile->logName() );
        if( it == map.cend() ) {
            map[ logFile->logName() ] = logFile->partNum();
        }
        else if( it->second < logFile->partNum() ) {
            it->second = logFile->partNum();
        }
    }
    return map;
}

std::string padNumber( size_t number, unsigned char width ) {
    std::stringstream ss;
    ss << std::setfill( '0' ) << std::setw( width ) << number;
    return ss.str();
}

std::string formatSliceName( const std::string& fileName, size_t filePosition, unsigned char suffixPadding ) {
    return fileName + "." + padNumber( filePosition, suffixPadding );
}

std::string formatSliceDate() {
    time_t now = time( nullptr );
    auto localNow = *localtime( &now );
    char formatted[ 128 ];
    std::strftime( formatted, sizeof( formatted ), "%m%d%H%M%S", &localNow );
    return formatted;
}

/** Slice log to the file with next .mmddHHMMSS suffix. */
void partitionLogsByDate( IFilesystem& filesystem,
                          const LogFiles& srcLogs,
                          const std::string& dstDir,
                          const LogFiles& existingLogs ) {
    auto partitionedLogs = buildPartitionMap( existingLogs );

    for( auto& logFile : srcLogs ) {
        auto it = partitionedLogs.find( logFile->logName() );

        /* Change partition method:
         *      existing: ha-luad.log -> ha-luad.log.<max-suffix + 1>
         *      modified: ha-luad.log -> ha-luad.log.<date-time>
         * */
        std::string fileName, extension;
        std::tie( fileName, extension ) = filesystem::getFilenameAndExtension( logFile->logName() );

        std::string datePart = formatSliceDate();

        // Make rotated log file: name.ext.<digits>
        auto dstPath = dstDir + "/" + LogFile::makeRotatedLogFileName( fileName, extension, datePart );
        LOGGER_DEBUG( "Log file slice name: " + dstPath );

        filesystem.copyFile( logFile->path(), dstPath );
        filesystem.truncateFile( logFile->path() );
    }
}

void createArchive( const std::string& name, const LogFiles& logFiles ) {
    std::map< std::string, LogFiles > map;

    for( auto& logFile : logFiles )
        map[ logFile->dirName() ].emplace_back( logFile );

    std::stringstream ss;
    ss << "tar -czf " << name;
    for( auto& element : map ) {
        auto& directory = element.first;
        auto& files = element.second;
        ss << " -C " << directory;
        for( auto& file : files )
            ss << " " << file->fileName();
    }

    std::cout.flush();
    std::cerr.flush();
    int result = std::system( ss.str().c_str() );
    result = System::unpackExitCode( result ).exitCode;
    if( result != 0 && result != 1 )
        throw LogArchiveFailed( "Log archive failed with code: " + std::to_string( result ) );
}

void deleteLogFiles( IFilesystem& filesystem, const LogFiles& logFiles ) {
    for( auto& logFile : logFiles )
        filesystem.removeFile( logFile->path() );
}

size_t getLogsSize( IFilesystem& filesystem, const LogFiles& logFiles ) {
    size_t size = 0;
    for( auto& logFile : logFiles )
        size += filesystem.getFileSize( logFile->path() );
    return size;
}

void rotateArchivesByNum( IFilesystem& filesystem, const std::string& directoryPath, size_t numLimit ) {
    auto archiveFiles = findArchivedLogs( filesystem, directoryPath );
    if( archiveFiles.empty() )
        return;
    if( archiveFiles.size() <= numLimit )
        return;

    // Old logs first
    auto predicate = []( const std::shared_ptr< LogFile >& logA, const std::shared_ptr< LogFile >& logB ) {
        return logA->fileName() < logB->fileName();
    };

    std::sort( archiveFiles.begin(), archiveFiles.end(), predicate );
    archiveFiles.resize( archiveFiles.size() - numLimit );
    deleteLogFiles( filesystem, archiveFiles );
}

void rotateArchivesBySize( IFilesystem& filesystem, const std::string& directoryPath, size_t sizeLimit ) {
    LOGGER_INFO_RAW( "Rotate archives by size, directory: " + directoryPath +
                     ", size limit: " + std::to_string( sizeLimit ) );
    auto archiveFiles = findArchivedLogs( filesystem, directoryPath );
    if( archiveFiles.empty() )
        return;

    // Old logs first
    auto predicate = []( const std::shared_ptr< LogFile >& logA, const std::shared_ptr< LogFile >& logB ) {
        return logA->fileName() < logB->fileName();
    };
    std::sort( archiveFiles.begin(), archiveFiles.end(), predicate );

    size_t totalSize = 0;
    for( auto it = archiveFiles.rbegin(); it != archiveFiles.rend(); ++it ) {
        auto& logFile = *it;
        totalSize += filesystem.getFileSize( logFile->path() );

        if( totalSize > sizeLimit ) {
            LOGGER_ERROR_RAW( "Archives total size greater that size limit! Remove further archives!" );

            size_t filesToDelete = archiveFiles.size() - std::distance( archiveFiles.rbegin(), it );
            archiveFiles.resize( filesToDelete );
            deleteLogFiles( filesystem, archiveFiles );
            return;
        }
    }
}

void processPendingLogs( IFilesystem& filesystem,
                         const std::string& directoryPath,
                         size_t sizeLimit,
                         const ProcessCallback& processLogFile ) {
    LOGGER_INFO_RAW( "Rotate logs by size, directory: " + directoryPath +
                     ", size limit: " + std::to_string( sizeLimit ) );
    auto logFiles = findRotatedLogs( filesystem, directoryPath );
    if( logFiles.empty() )
        return;

    if( processLogFile == nullptr ) {
        LOGGER_ERROR( "No process callback passed" );
        return;
    }

    auto orderBySuffixAscFileNameAsc = []( const std::shared_ptr< LogFile >& logA,
                                           const std::shared_ptr< LogFile >& logB ) {
        if( logA->partNum() < logB->partNum() )
            return true;
        if( logA->fileName() < logB->fileName() )
            return true;
        return false;
    };
    std::sort( logFiles.begin(), logFiles.end(), orderBySuffixAscFileNameAsc );

    for( auto& logfile : logFiles ) {
        processLogFile( *logfile );
    }
}

void rotatePendingLogsBySize( IFilesystem& filesystem, const std::string& directoryPath, size_t sizeLimit ) {
    LOGGER_INFO_RAW( "Rotate logs by size, directory: " + directoryPath +
                     ", size limit: " + std::to_string( sizeLimit ) );
    auto logFiles = findRotatedLogs( filesystem, directoryPath );
    if( logFiles.empty() )
        return;

    auto orderBySuffixAscFileNameAsc = []( const std::shared_ptr< LogFile >& logA,
                                           const std::shared_ptr< LogFile >& logB ) {
        if( logA->partNum() < logB->partNum() )
            return true;
        if( logA->fileName() < logB->fileName() )
            return true;
        return false;
    };
    std::sort( logFiles.begin(), logFiles.end(), orderBySuffixAscFileNameAsc );

    size_t totalSize = 0;
    for( auto it = logFiles.begin(); it != logFiles.end(); ++it ) {
        auto& logFile = *it;
        // Calculate list of files that should be deleted
        totalSize += filesystem.getFileSize( logFile->path() );

        if( totalSize > sizeLimit ) {
            LOGGER_ERROR_RAW( "Logs total size greater that size limit! Remove further logs!" );
            while( it != logFiles.end() ) {
                LOGGER_WARN_RAW( "Remove log file: " + ( *it )->path() );
                filesystem.removeFile( ( *it++ )->path() );
            }
            return;
        }
    }
}

}  // namespace LogUtils
}  // namespace LogCmd
}  // namespace HubPlatform
