#include "cloud_storage_type.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

std::string toString( StorageType storageType ) {
    switch( storageType ) {
    case StorageType::Backup:
        return "backup";
    case StorageType::Multimedia:
        return "multimedia";
    case StorageType::ControllerBackup:
        return "controller_backup";
    case StorageType::ArchiveControllerBackup:
        return "archive_controller_backup";
    case StorageType::ControllerLog:
        return "controller_log";
    case StorageType::ArchiveControllerLog:
        return "archive_controller_log";
    }
    return "";
}

std::string toString( StorageBucket value ) {
    switch( value ) {
    case StorageBucket::ControllerBackups:
        return "controller_backups";
    case StorageBucket::ControllerLogs:
        return "controller_logs";
    case StorageBucket::Media:
        return "media";
    case StorageBucket::Test:
        return "test";
    case StorageBucket::Scripts:
        return "scripts";
    case StorageBucket::Default:
    default:;
        return "default";
    }
}

std::string toString( PrepareUploadType prepareUploadType ) {
    switch( prepareUploadType ) {
    case PrepareUploadType::UploadAndPreview:
        return "upload-and-preview";
    }
    return "";
}

std::string toString( OwnerEntityType ownerEntityType ) {
    switch( ownerEntityType ) {
    case OwnerEntityType::User:
        return "user";
    case OwnerEntityType::Location:
        return "location";
    case OwnerEntityType::Controller:
        return "controller";
    }
    return "";
}

std::string toString( Errors error ) {
    switch( error ) {
    case Errors::Ok:
        return "ok";
    case Errors::InvalidUrl:
        return "invalid_url";
    case Errors::InvalidToken:
        return "invalid_token";
    case Errors::InvalidXapp:
        return "invalid_xapp";
    case Errors::RequestError:
        return "request_error";
    case Errors::WrongPathFormat:
        return "wrong_path_format";
    case Errors::WrongFileFormat:
        return "wrong_file_format";
    }
    return "";
}

std::string toString( LoadType loadType ) {
    switch( loadType ) {
    case LoadType::Upload:
        return "upload";
    case LoadType::UploadRedirect:
        return "uploadRedirect";
    case LoadType::UploadLogs:
        return "uploadLogs";
    case LoadType::Download:
        return "download";
    case LoadType::DownloadRedirect:
        return "downloadRedirect";
    }
    return "";
}

std::string toString( UploadStepType uploadStep ) {
    switch( uploadStep ) {
    case UploadStepType::UploadRedirectPrepare:
        return "uploadRedirectPrepare";
    case UploadStepType::UploadRedirectFileData:
        return "uploadRedirectFileData";
    case UploadStepType::UploadRedirectFinished:
        return "uploadRedirectFinished";
    }
    return "";
}

std::string toString( DownloadStepType downloadStep ) {
    switch( downloadStep ) {
    case DownloadStepType::DownloadRedirectPrepare:
        return "downloadRedirectPrepare";
    case DownloadStepType::DownloadRedirectFinished:
        return "downloadRedirectFinished";
    }
    return "";
}

JsonArray getBucketsForBackups() {
    JsonArray list;
    list.push_back( toString( StorageBucket::Default ) );
    list.push_back( toString( StorageBucket::ControllerBackups ) );
    return list;
}

namespace Resources {
namespace FileKeyFields {
const char* const Key = "key";
const char* const Uuid = "uuid";
const char* const MD5Checksum = "md5_checksum";
}  // namespace FileKeyFields
}  // namespace Resources

FileKey::FileKey( std::string fileKey, std::string fileUuid, std::string fileMd5Checksum )
: key( std::move( fileKey ) )
, uuid( std::move( fileUuid ) )
, md5Checksum( std::move( fileMd5Checksum ) ) {}

Json FileKey::toJson() const {
    using namespace Resources::FileKeyFields;
    return { { Key, key }, { Uuid, uuid }, { MD5Checksum, md5Checksum } };
}

bool FileKey::fromJson( const JsonObject& json ) {
    using namespace Resources::FileKeyFields;
    if( json.is_object() && json.find( Key ) != json.end() && json.find( Uuid ) != json.end() ) {
        key = json_get_str( json, Key );
        uuid = json_get_str( json, Uuid );
        md5Checksum = json_get_str( json, MD5Checksum );
        return true;
    }
    return false;
}

UploadFileData::UploadFileData( std::string filePath )
: file( std::move( filePath ) ) {}

UploadFileData::UploadFileData( std::string filePath, std::string fileUuid, Json metadata )
: file( std::move( filePath ) )
, uuid( std::move( fileUuid ) )
, metadata( std::move( metadata ) ) {}

Json UploadFileData::toJson() const {
    return { { "file", file }, { "uuid", uuid }, { "metadata", metadata } };
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform