#include "log_storage_impl.h"

#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"
#include "database_client/dbtools.h"

#include "logcmd/log_exceptions.h"

namespace HubPlatform {
namespace LogCmd {

DefaultLogStorageImpl::DefaultLogStorageImpl() {
    std::string dbPath = getStorageFilePath();
    if( dbPath.empty() )
        throw LogUploadError( "Database path is empty" );
    m_dbClient = std::make_unique< LMDBClient >( dbPath, getTemporaryStorageFilePath() );
    m_storage = std::make_unique< StorageInterface >( *m_dbClient );
}

void DefaultLogStorageImpl::upload( const std::string& filePath,
                                    const Json& storageMetadata,
                                    UploadResponse response ) {
    namespace CloudStorage = Common::CloudStorageInterface;
    CloudStorage::UploadFileData data( filePath, "", storageMetadata );
    CloudStorage::Errors error =
        m_storage->uploadLogs( data, CloudStorage::StorageType::ArchiveControllerLog, response );
    if( error != CloudStorage::Errors::Ok ) {
        LOGGER_ERROR( cc::error( "Failed to upload logs, error: " ) + toString( error ) );
    }
}

void DefaultLogStorageImpl::stopTransfer() {
    m_storage->stopTransfer();
}

}  // namespace LogCmd
}  // namespace HubPlatform
