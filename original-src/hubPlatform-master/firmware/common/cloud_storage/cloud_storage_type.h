#pragma once

#include "json_utils/json_fwd.h"
#include <string>

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

enum class StorageType {
    Backup,
    Multimedia,
    ControllerBackup,
    ArchiveControllerBackup,
    ControllerLog,
    ArchiveControllerLog
};
enum class StorageBucket { Default = 0, Test, Scripts, ControllerBackups, ControllerLogs, Media };
enum class PrepareUploadType { UploadAndPreview };
enum class OwnerEntityType { Controller, User, Location };
enum class LoadType { Upload = 0, UploadLogs, Download, UploadRedirect, DownloadRedirect };
enum class UploadStepType { UploadRedirectPrepare = 0, UploadRedirectFileData = 1, UploadRedirectFinished = 2 };
enum class DownloadStepType { DownloadRedirectPrepare = 0, DownloadRedirectFinished = 1 };
enum class Errors { Ok, InvalidUrl, InvalidToken, InvalidXapp, RequestError, WrongPathFormat, WrongFileFormat };

std::string toString( StorageType storageType );
std::string toString( StorageBucket value );
std::string toString( PrepareUploadType prepareUploadType );
std::string toString( OwnerEntityType ownerEntityType );
std::string toString( Errors error );
std::string toString( LoadType loadType );
std::string toString( UploadStepType uploadStep );
std::string toString( DownloadStepType downloadStep );
JsonArray getBucketsForBackups();

struct FileKey {
    FileKey() = default;
    FileKey( std::string fileKey, std::string fileUuid, std::string fileMd5Checksum );
    Json toJson() const;
    bool fromJson( const JsonObject& json );

public:
    std::string key;
    std::string uuid;
    std::string md5Checksum;
};

struct UploadFileData {
    explicit UploadFileData( std::string filePath );
    UploadFileData( std::string filePath, std::string fileUuid, Json metadata );
    Json toJson() const;

public:
    std::string file;
    std::string uuid;
    Json metadata;
};

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
