#pragma once

#include "cloud_storage/cloud_storage_type.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <chrono>

class IDbClient;

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

class CloudStorageImpl;

class RemoteInterface final {
public:
    using UploadResponse =
        std::function< void( Json requestMetadata, const std::string& error, const FileKey& fileKey ) >;
    using DownloadResponse = std::function< void( Json requestMetadata, const std::string& error ) >;
    using Path = std::string;

    explicit RemoteInterface( IDbClient& dbClient );
    ~RemoteInterface();

    /* @brief return file key as response. In case of error returns empty key */
    Errors uploadFile( const UploadFileData& data, StorageType storageType, UploadResponse response );
    Errors uploadBackup( const UploadFileData& data, StorageType storageType, UploadResponse response );
    Errors uploadLogs( const UploadFileData& data, StorageType storageType, UploadResponse response );
    Errors downloadFile( const FileKey& fileKey, const Path& downloadPath, DownloadResponse response );
    void stopTransfer();

private:
    CloudStorageImpl* m_pimpl;
};

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
