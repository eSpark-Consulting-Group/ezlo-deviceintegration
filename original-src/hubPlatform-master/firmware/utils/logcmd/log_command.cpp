#include "log_command.h"

#include "json_utils/json_utils.h"
#include "system_utils/autoremove_file.h"
#include "system_utils/crontab/crontab.h"
#include "system_utils/environment.h"
#include "log_utils/ezlo_log.h"
#include <tiny/tiny_consolecolors.h>
#include <fstream>

#include "logcmd/log_filter.h"
#include "logcmd/log_arg_parse.h"
#include "logcmd/log_storage.h"
#include "logcmd/log_common.h"
#include "logcmd/log_resources.h"
#include "logcmd/log_exceptions.h"
#include "logcmd/log_utils/log_utils.h"

#include "system_utils/file_lock.h"
#include "system_utils/system.h"
#include "common_utils/hub_info_helper.h"
#include "system_utils/filesystem.h"

namespace HubPlatform {
namespace LogCmd {

namespace {
JsonObject makeArchiveMetadata( std::time_t timestamp ) {
    return JSON( "controller_info" << JSON( "timestamp" << timestamp << "controller_uuid"
                                                        << HubInfoHelper::getControllerUUID() ) );
}
}  // namespace

int LogCommand::run( const Json& args ) {
    const bool idleMode = json_get_bool( args, ARG_IDLE );

    try {
        loadLogSettings( m_logSettings );

        if( json_get_bool( args, ARG_STORE ) || json_get_bool( args, ARG_STORE_TO_POSTPONED ) ) {
            return store( args );
        }
        else if( json_get_bool( args, ARG_UPLOAD_ARCHIVED ) || json_get_bool( args, ARG_UPLOAD_POSTPONED ) ) {
            return uploadArchives( args );
        }
        else if( json_get_bool( args, ARG_ROTATE ) ) {
            return rotate( args );
        }
        else if( json_get_bool( args, ARG_SLICE ) ) {
            return slice( args );
        }
        else if( json_get_bool( args, ARG_ROTATE_ON_SIZE_LIMIT ) ) {
            return rotateOnSizeLimit( args );
        }
        else if( json_get_bool( args, ARG_UPDATE_SETTINGS ) ) {
            return updateSettings( args );
        }
        else {
            return upload( args );
        }

        return 0;
    }
    catch( LogDirectoryFilesNotFoundError& e ) {
        LOGGER_WARN_RAW( cc::warn( "Unable to find logs: " ) + cc::note( e.what() ) );
        return idleMode ? 0 : -1;
    }
    catch( std::exception& e ) {
        LOGGER_ERROR_RAW( cc::error( "Exception (" + std::string( typeid( e ).name() ) + "): " ) +
                          cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR_RAW( cc::error( "Unknown exception was caught" ) );
    }
    return -1;
}

int LogCommand::upload( const Json& args ) {
    const auto logDirPath = json_get_str( args, ARG_LOG_DIR );

    auto fileLock = filesystem::FileLock( Resources::fileLockPath );
    fileLock.lock();
    autoupdateSystemLogs( args, logDirPath );

    LogFiles logFiles = findLogFiles( args, logDirPath );

    if( json_get_bool( args, ARG_ROTATED_LOGS ) ) {
        auto pendingLogs = getPendingFiles();
        for( auto& file : pendingLogs )
            logFiles.emplace_back( std::move( file ) );
    }

    printLogNames( logFiles );

    auto archiveSize = LogUtils::getLogsSize( m_filesystem, logFiles );

    std::string temporaryDir = Environment::getTempDataFolder();
    std::string postponedDir = Environment::getPostponedLogsFolder();

    System::dropVirtualMemoryCaches();
    const bool archiveInRam = ( System::getMemoryInfo().freeram > 3 * archiveSize );
    std::string archiveDirPath = ( archiveInRam ) ? temporaryDir : postponedDir;
    m_filesystem.createDirectory( archiveDirPath, true );

    std::time_t now = std::time( nullptr );
    auto archiveName = getArchiveName( args, now );
    auto archivePath = archiveDirPath + "/" + archiveName;
    LogUtils::createArchive( archivePath, logFiles );
    HubPlatform::filesystem::AutoremoveFile autoRemove( archivePath );

    fileLock.unlock();
    uploadArchive( archivePath, makeArchiveMetadata( now ) );
    fileLock.lock();

    autoRemoveLogs( args, logFiles );

    return 0;
}

int LogCommand::uploadArchives( const Json& args ) {
    const bool idleMode = json_get_bool( args, ARG_IDLE );
    const auto logDirPath = json_get_str( args, ARG_LOG_DIR );

    if( !m_filesystem.isDirectoryExist( logDirPath ) && idleMode ) {
        LOGGER_INFO_RAW( cc::info( "No logs to upload" ) );
        return 0;
    }

    const LogFiles logFiles = LogUtils::findArchivedLogs( m_filesystem, logDirPath );
    printLogNames( logFiles );

    for( auto& logFile : logFiles ) {
        auto logFileModified = filesystem::fileModificationTime( logFile->path() );
        uploadArchive( logFile->path(), makeArchiveMetadata( logFileModified ) );
        autoRemoveLogs( args, { logFile } );
    }
    return 0;
}

int LogCommand::store( const Json& args ) {
    const auto logDirPath = json_get_str( args, ARG_LOG_DIR );
    const std::string outputDirPath = json_get_str( args, ARG_OUTPUT_DIR );

    auto fileLock = filesystem::FileLock( Resources::fileLockPath );
    fileLock.lock();

    autoupdateSystemLogs( args, logDirPath );

    LogFiles logFiles = findLogFiles( args, logDirPath );

    if( json_get_bool( args, ARG_ROTATED_LOGS ) ) {
        auto pendingLogs = getPendingFiles();
        for( auto& file : pendingLogs )
            logFiles.emplace_back( std::move( file ) );
    }

    printLogNames( logFiles );
    setupOutputDir( args, outputDirPath );

    auto archiveName = getArchiveName( args );
    auto archivePath = outputDirPath + "/" + archiveName;

    // NOTE: archive might be created on USB stick.
    // What would happen with process/lock/file if stick will be removed during the process ?
    // It is better to not use lock for external storage
    fileLock.unlock();
    LogUtils::createArchive( archivePath, logFiles );
    fileLock.lock();
    LOGGER_DEBUG_RAW( cc::debug( "Logs stored at: " ) + cc::note( archivePath ) );

    if( json_get_bool( args, ARG_STORE_TO_POSTPONED ) )
        rotateArchives( outputDirPath );
    else
        autorotateArchives( args, outputDirPath );

    autoRemoveLogs( args, logFiles );

    return 0;
}

/** Performs rotation of logs according to rotation policy.
 *  If rotation is disabled - does nothing.
 *  If rotation policy is 'each_rotation", uploads logs to cloud after the rotation.
 *  If rotation policy is 'store_locally", stores logs locally instead of uploading to cloud.
 *
 *  If the --force-upload flag is set, overrides selected policy and uploads logs to cloud.
 * */
int LogCommand::rotate( const Json& args ) {
    using RotationPolicy = LogSettings::RotationPolicy;

    bool forceUpload = json_get_bool( args, ARG_FORCE_UPLOAD );
    const auto& rotationPolicy = m_logSettings.rotationPolicy;
    if( rotationPolicy == RotationPolicy::Off && !forceUpload ) {
        LOGGER_INFO_RAW( cc::info( "Rotation policy is \"off\". Nothing to do." ) );
        return 0;
    }

    auto fileLock = filesystem::FileLock( Resources::fileLockPath );
    fileLock.lock();

    const auto logDirPath = Environment::getLogsFolder();
    autoupdateSystemLogs( args, logDirPath );
    const LogFiles logFiles = LogUtils::findRegularLogs( m_filesystem, logDirPath );

    LogFiles logsToPartition;
    size_t logsInRamSize = 0;

    for( auto& logFile : logFiles ) {
        auto size = m_filesystem.getFileSize( logFile->path() );
        if( size > 0 ) {
            logsInRamSize += size;
            logsToPartition.emplace_back( logFile );
        }
    }

    printLogNames( logsToPartition );

    auto pendingFiles = getPendingFiles();
    if( logsToPartition.empty() && pendingFiles.empty() ) {
        LOGGER_INFO_RAW( cc::info( "No logs to partition and no pending logs was detected. Nothing to do." ) );
        return 0;
    }

    System::dropVirtualMemoryCaches();
    const bool partitionInRam = ( System::getMemoryInfo().freeram > 3 * logsInRamSize );

    const std::string pendingDirPath = Environment::getPendingLogsFolder();
    auto existingLogs = LogUtils::joinLogFiles( LogUtils::findRotatedLogs( m_filesystem, logDirPath ), pendingFiles );
    if( partitionInRam )
        LogUtils::partitionLogsByDate( m_filesystem, logsToPartition, logDirPath, existingLogs );
    else
        LogUtils::partitionLogsByDate( m_filesystem, logsToPartition, pendingDirPath, existingLogs );

    auto logsToArchive = LogUtils::joinLogFiles( LogUtils::findRotatedLogs( m_filesystem, logDirPath ),
                                                 LogUtils::findRotatedLogs( m_filesystem, pendingDirPath ) );

    size_t archiveSize = LogUtils::getLogsSize( m_filesystem, logsToArchive );
    std::time_t now = std::time( nullptr );
    // TODO: which time should we put to archive metadata here? Putting current time for new archives.
    const auto archiveName = getArchiveName( args, now );

    bool rotateAndSend = ( rotationPolicy == RotationPolicy::EachRotation ) || forceUpload;
    if( rotateAndSend ) {
        std::string temporaryDir = Environment::getTempDataFolder();
        std::string postponedDir = Environment::getPostponedLogsFolder();

        const bool archiveInRam = ( System::getMemoryInfo().freeram > 3 * archiveSize );
        std::string archiveDirPath = ( archiveInRam ) ? temporaryDir : postponedDir;
        m_filesystem.createDirectory( archiveDirPath, true );

        auto archivePath = archiveDirPath + "/" + archiveName;
        LogUtils::createArchive( archivePath, logsToArchive );
        HubPlatform::filesystem::AutoremoveFile autoRemove( archivePath );
        LogUtils::deleteLogFiles( m_filesystem, logsToArchive );

        bool uploaded = false;
        try {
            fileLock.unlock();
            uploadArchive( archivePath, makeArchiveMetadata( now ) );
            uploaded = true;
        }
        catch( std::exception& e ) {
            LOGGER_ERROR_RAW(
                cc::error( "Failed to upload logs. Exception (" + std::string( typeid( e ).name() ) + "): " ) +
                cc::note( e.what() ) );
        }
        catch( ... ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to upload logs. Unknown exception was caught" ) );
        }

        if( !uploaded ) {
            fileLock.lock();
            if( archiveDirPath != postponedDir )
                moveArchive( archivePath, postponedDir );
            rotateArchives( postponedDir );
            autoRemove.setAutoremove( false );
        }
    }
    else if( rotationPolicy == RotationPolicy::StoreLocally ) {
        std::string rotatedDir = Environment::getRotatedLogsFolder();
        m_filesystem.createDirectory( rotatedDir, true );
        auto archivePath = rotatedDir + "/" + archiveName;
        LogUtils::createArchive( archivePath, logsToArchive );
        LogUtils::deleteLogFiles( m_filesystem, logsToArchive );
        rotateArchives( rotatedDir );
    }

    std::ofstream configFile( Environment::getTempDataFolder() + "/logs_rotate_status" );
    if( !configFile.is_open() ) {
        LOGGER_TRACE_RAW( cc::trace( "Can't open the logs_rotate_status file" ) );
        return 0;
    }
    configFile << 1 << std::endl;
    configFile.close();

    return 0;
}

int LogCommand::slice( const Json& args ) {
    const auto& fileSizeLimit = m_logSettings.fileSizeLimit;
    if( fileSizeLimit == 0 ) {
        LOGGER_TRACE_RAW( cc::trace( "Slicing is disabled. Nothing to do." ) );
        return 0;
    }

    auto fileLock = filesystem::FileLock( Resources::fileLockPath );
    if( !waitForCondition(
            [&fileLock] { return fileLock.tryLock(); }, std::chrono::seconds( 10 ), std::chrono::seconds( 1 ) ) )
        LOGGER_WARN_RAW( cc::warn( "Unable to acquire file lock for slice operation. Continue anyway" ) );

    const LogFiles logFiles = LogUtils::findRegularLogs( m_filesystem, Environment::getLogsFolder() );

    Log::info( "Log command slice: file size limit passed: " + cc::num10( fileSizeLimit ) );

    LogFiles logsToSlice;
    for( auto& logFile : logFiles ) {
        auto size = m_filesystem.getFileSize( logFile->path() );
        if( size > fileSizeLimit ) {
            logsToSlice.emplace_back( logFile );
        }
    }

    if( logsToSlice.empty() ) {
        LOGGER_INFO_RAW( cc::info( "No logs to slice. Nothing to do." ) );
        return 0;
    }

    printLogNames( logsToSlice );

    auto existingLogs = LogUtils::joinLogFiles( logFiles, getPendingFiles() );
    LogUtils::partitionLogsByDate( m_filesystem, logsToSlice, Environment::getPendingLogsFolder(), existingLogs );

    // calculate folder size
    size_t totalFolderSize = 0;
    auto getSlicedLogsSize = [this, &totalFolderSize]( const LogFile& logFile ) -> void {
        totalFolderSize += m_filesystem.getFileSize( logFile.path() );
    };

    LogUtils::processPendingLogs(
        m_filesystem, Environment::getPendingLogsFolder(), m_logSettings.pendingLogsSizeLimit, getSlicedLogsSize );

    LOGGER_DEBUG( "Pending logs folder size is : " + cc::num10( totalFolderSize ) );

    if( totalFolderSize >= m_logSettings.pendingLogsSizeLimit ) {
        LOGGER_INFO( "Slices in the 'pending' folder consume more space that allowed!" );
    }

    LogUtils::rotatePendingLogsBySize(
        m_filesystem, Environment::getPendingLogsFolder(), m_logSettings.pendingLogsSizeLimit );

    return 0;
}

/** Replace slicing with this:
 *     - check if logs folders is greater than limit.
 *     - if yes, rotate and upload or store logs archive.
 *     - if no, just slice the files that greater then file size limit.
 * @params args - not in use */
int LogCommand::rotateOnSizeLimit( const Json& args ) {
    const auto& fileSizeLimit = m_logSettings.fileSizeLimit;
    if( fileSizeLimit == 0 ) {
        LOGGER_TRACE_RAW( cc::trace( "Slicing is disabled. Nothing to do." ) );
        return 0;
    }

    auto fileLock = filesystem::FileLock( Resources::fileLockPath );
    if( !waitForCondition(
            [&fileLock] { return fileLock.tryLock(); }, std::chrono::seconds( 30 ), std::chrono::seconds( 1 ) ) )
        LOGGER_WARN_RAW( cc::warn( "Unable to acquire file lock for slice operation. Continue anyway" ) );

    const LogFiles logFiles = LogUtils::findRegularLogs( m_filesystem, Environment::getLogsFolder() );

    Log::info( "Log command rotate-on-size-limit: file size limit passed: " + cc::num10( fileSizeLimit ) );

    LogFiles logsToSlice;
    for( auto& logFile : logFiles ) {
        auto size = m_filesystem.getFileSize( logFile->path() );
        if( size > fileSizeLimit ) {
            logsToSlice.emplace_back( logFile );
        }
    }

    if( logsToSlice.empty() ) {
        LOGGER_INFO_RAW( cc::info( "No logs to slice. Nothing to do." ) );
        return 0;
    }

    printLogNames( logsToSlice );

    auto existingLogs = LogUtils::joinLogFiles( logFiles, getPendingFiles() );
    LogUtils::partitionLogsByDate( m_filesystem, logsToSlice, Environment::getPendingLogsFolder(), existingLogs );

    // calculate folder size:
    size_t totalFolderSize = 0;
    size_t numberOfFiles = 0;
    auto getSlicedLogsSize = [this, &totalFolderSize, &numberOfFiles]( const LogFile& logFile ) -> void {
        totalFolderSize += m_filesystem.getFileSize( logFile.path() );
        numberOfFiles++;
    };

    LogUtils::processPendingLogs(
        m_filesystem, Environment::getPendingLogsFolder(), m_logSettings.pendingLogsSizeLimit, getSlicedLogsSize );

    size_t pendingLogsSize = totalFolderSize;
    size_t pendingLogsCount = numberOfFiles;
    totalFolderSize = 0;
    numberOfFiles = 0;

    LogUtils::processPendingLogs(
        m_filesystem, Environment::getLogsFolder(), m_logSettings.pendingLogsSizeLimit, getSlicedLogsSize );
    size_t logsFolderSize = totalFolderSize;

    if( pendingLogsSize + logsFolderSize >= m_logSettings.pendingLogsSizeLimit ) {
        LOGGER_INFO( "Slices in pending folder consume more space that allowed, rotate logs!" );
        LOGGER_DEBUG_RAW( "Pending logs folder size is : " + cc::num10( pendingLogsSize ) +
                          ", files count: " + cc::num10( pendingLogsCount ) );
        LOGGER_DEBUG_RAW( "Logs folder size is : " + cc::num10( logsFolderSize ) +
                          ", files count: " + cc::num10( numberOfFiles ) );

        Json rotateArgs = JSON_EMPTY_OBJ;

        Impl::addFlag( rotateArgs, ARG_ROTATE, "" );
        Impl::addFlag( rotateArgs, ARG_IDLE, "" );
        Impl::addFlag( rotateArgs, ARG_AUTOREMOVE, "" );
        Impl::addFlag( rotateArgs, ARG_ROTATED_LOGS, "" );
        Impl::addValue( rotateArgs, ARG_ADD_TO_NAME, "rotate_on_size" );

        fileLock.unlock();
        rotate( rotateArgs );

        LOGGER_INFO_RAW( "Rotation on size limit completed" );
    }
    else {
        LOGGER_DEBUG_RAW( "Check if slicing needed" );

        Json sliceArgs = JSON_EMPTY_OBJ;
        Impl::addFlag( sliceArgs, ARG_SLICE, "" );

        fileLock.unlock();
        slice( sliceArgs );
    }

    return 0;
}

int LogCommand::updateSettings( const Json& args ) {
    const std::string rotationCommand = Environment::getSystemConfigsFolder() + "/cronparts/logrotate";
    const system_utils::Crontab::Record defaultRotationRecord = { { "17", "*/12", "*", "*", "*" }, rotationCommand };

    const std::string slicingCommand = Environment::getSystemConfigsFolder() + "/cronparts/logslice";
    const system_utils::Crontab::Record defaultSlicingRecord = { { "*", "*", "*", "*", "*" },
                                                                 slicingCommand };  // every minute

    system_utils::Crontab crontab;
    crontab.lock();
    crontab.load();
    auto record = crontab.findRecord( rotationCommand );
    if( !record ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to find crontab rotation command: " ) + cc::note( rotationCommand ) );
        crontab.addRecord( defaultRotationRecord );
    }
    else {
        auto& command = record->second;
        if( command.find( "hourly" ) != std::string::npos || command.find( "daily" ) != std::string::npos ) {
            LOGGER_WARN_RAW( cc::warn( "Old crontab format detected. Updating to new format ..." ) );
            crontab.deleteRecord( Environment::getSystemConfigsFolder() + "/cronparts/logrotate_daily" );
            crontab.deleteRecord( Environment::getSystemConfigsFolder() + "/cronparts/logrotate_hourly" );
            crontab.addRecord( defaultRotationRecord );
        }
    }

    if( !crontab.findRecord( slicingCommand ) ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to find crontab slicing command: " ) + cc::note( slicingCommand ) );
        crontab.addRecord( defaultSlicingRecord );
    }

    size_t seed = std::stoul( getHubSerial() );
    system_utils::Crontab::Schedule schedule = generateCronSchedule( m_logSettings.rotationInterval, seed );
    crontab.updateCommandSchedule( rotationCommand, schedule );
    if( crontab.isUpdated() ) {
        crontab.write();
        crontab.restartCron();
    }
    return 0;
}

void LogCommand::uploadArchive( const std::string& archivePath, const Json& archiveMetadata ) {
    LOGGER_INFO_RAW( cc::info( "Upload logs to cloud: " ) + cc::note( archivePath ) );
    LogStorage storage;
    auto result = storage.upload( archivePath, archiveMetadata );
    LOGGER_INFO_RAW( cc::info( "Logs have been uploaded" ) );
    LOGGER_DEBUG_RAW( cc::debug( "Upload result: " ) + cc::note( result.dump() ) );
}

void LogCommand::moveArchive( const std::string& archivePath, const std::string& dstDir ) {
    LOGGER_INFO_RAW( cc::info( "Store logs to: " ) + cc::note( dstDir ) );
    m_filesystem.createDirectory( dstDir, true );
    m_filesystem.moveFile( archivePath, dstDir + "/" );
    LOGGER_INFO_RAW( cc::info( "Logs have been stored" ) );
}

void LogCommand::rotateArchives( const std::string& directoryPath ) {
    auto rotationValue = m_logSettings.archivesTotalSizeLimit;
    LogUtils::rotateArchivesBySize( m_filesystem, directoryPath, rotationValue );
}

void LogCommand::autorotateArchives( const Json& args, const std::string& directoryPath ) {
    if( json_contains( args, ARG_ROTATE_ARCHIVES ) ) {
        size_t rotationValue = json_get_int( args, ARG_ROTATE_ARCHIVES );
        LogUtils::rotateArchivesByNum( m_filesystem, directoryPath, rotationValue );
    }
}

void LogCommand::autoRemoveLogs( const Json& args, const LogFiles& logFiles ) {
    if( json_get_bool( args, ARG_AUTOREMOVE ) ) {
        LogUtils::deleteLogFiles( m_filesystem, logFiles );
    }
}

void LogCommand::setupOutputDir( const Json& args, const std::string& outputDirPath ) {
    if( json_contains( args, ARG_CREATE_DIR ) ) {
        m_filesystem.createDirectory( outputDirPath, true );
    }
}

void LogCommand::autoupdateSystemLogs( const Json& args, const std::string& directoryPath ) {
    if( false == json_get_bool( args, ARG_NO_SYSTEM_LOGS ) )
        updateSystemLogs( directoryPath );
}

LogFiles LogCommand::findLogFiles( const Json& args, const std::string& directoryPath ) {
    if( json_get_bool( args, ARG_ROTATED_LOGS ) || json_get_bool( args, ARG_ROTATE ) ) {
        return LogUtils::findRotatedLogs( m_filesystem, directoryPath );
    }
    else if( json_contains( args, ARG_SELECTED_LOGS ) ) {
        auto selectedLogs = json_get_vec< std::string >( args, ARG_SELECTED_LOGS );
        return LogUtils::findSelectedLogs( m_filesystem, directoryPath, selectedLogs );
    }
    else if( json_get_bool( args, ARG_UPLOAD_ARCHIVED ) || json_get_bool( args, ARG_UPLOAD_POSTPONED ) ) {
        return LogUtils::findArchivedLogs( m_filesystem, directoryPath );
    }
    else
        return LogUtils::findLogs( m_filesystem, directoryPath );
}

std::string LogCommand::getArchiveName( const Json& args, std::time_t timestamp ) {
    std::string archiveName = json_get_str( args, ARG_OUTPUT_NAME );
    if( false == archiveName.empty() )
        return archiveName;
    std::string userString = json_get_str( args, ARG_ADD_TO_NAME );
    return generateArchiveName( std::move( userString ), timestamp );
}

void LogCommand::printLogNames( const LogFiles& logFiles ) {
    std::stringstream ss;
    for( auto& logFile : logFiles )
        ss << " " << logFile->fileName();
    LOGGER_DEBUG_RAW( cc::debug( "Found logs: " ) + cc::note( ss.str() ) );
}

LogFiles LogCommand::getPendingFiles() {
    const auto pendingDirPath = Environment::getPendingLogsFolder();
    m_filesystem.createDirectory( pendingDirPath, true );
    return LogUtils::findRotatedLogs( m_filesystem, pendingDirPath );
}

}  // namespace LogCmd
}  // namespace HubPlatform
