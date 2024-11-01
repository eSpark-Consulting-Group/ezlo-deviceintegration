#pragma once

#include <string>
#include <memory>
#include "json_utils/json_fwd.h"
#include "database_client/lmdb_client.h"
#include "cloud_storage/cloud_storage_interface.h"

namespace HubPlatform {
namespace LogCmd {

class LogStorageImpl {
public:
    using UploadResponse = std::function<
        void( Json userData, const std::string& error, const Common::CloudStorageInterface::FileKey& fileKey ) >;
    virtual ~LogStorageImpl() = default;
    virtual void upload( const std::string& filePath, const Json& storageMetadata, UploadResponse response ) = 0;
    virtual void stopTransfer() = 0;
};

class DefaultLogStorageImpl : public LogStorageImpl {
    using StorageInterface = HubPlatform::Common::CloudStorageInterface::RemoteInterface;

public:
    DefaultLogStorageImpl();
    void upload( const std::string& filePath, const Json& storageMetadata, UploadResponse response ) override;
    void stopTransfer() override;

private:
    std::unique_ptr< LMDBClient > m_dbClient;
    std::unique_ptr< StorageInterface > m_storage;
};

}  // namespace LogCmd
}  // namespace HubPlatform
