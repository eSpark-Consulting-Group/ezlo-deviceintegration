#pragma once

#include "logcmd/log_file.h"
#include "logcmd/log_settings.h"
#include "logcmd/filesystem/filesystem.h"
#include "json_utils/json_utils.h"
#include <string>

namespace HubPlatform {
namespace LogCmd {

class LogCommand {
public:
    int run( const Json& args );

private:
    int upload( const Json& args );
    int uploadArchives( const Json& args );
    int store( const Json& args );
    int rotate( const Json& args );
    int slice( const Json& args );
    int rotateOnSizeLimit( const Json& args );
    int updateSettings( const Json& args );

    void uploadArchive( const std::string& archivePath, const Json& archiveMetadata );
    void moveArchive( const std::string& archivePath, const std::string& dstDir );
    void rotateArchives( const std::string& directoryPath );
    void autorotateArchives( const Json& args, const std::string& directoryPath );
    void autoRemoveLogs( const Json& args, const LogFiles& logFiles );
    void setupOutputDir( const Json& args, const std::string& outputDirPath );
    void autoupdateSystemLogs( const Json& args, const std::string& directoryPath );
    std::string getArchiveName( const Json& args, std::time_t timestamp = 0 );
    LogFiles findLogFiles( const Json& args, const std::string& directoryPath );
    void printLogNames( const LogFiles& logFiles );
    LogFiles getPendingFiles();

private:
    LogSettings m_logSettings;
    Filesystem m_filesystem;
};

}  // namespace LogCmd
}  // namespace HubPlatform
