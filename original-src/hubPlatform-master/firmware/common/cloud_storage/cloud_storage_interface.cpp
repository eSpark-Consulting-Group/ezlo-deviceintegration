#include <fstream>
#include "cloud_storage/cloud_storage_interface.h"
#include "cloud_storage/cloud_storage_impl.h"
#include "database_client/idbclient.h"
#include "common_utils/cpp14.h"
#include "crypto_utils/digests.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

const std::string md5ChecksumKey = "md5_checksum";

RemoteInterface::RemoteInterface( IDbClient& dbClient )
: m_pimpl( new CloudStorageImpl( dbClient ) ) {}

RemoteInterface::~RemoteInterface() {
    delete m_pimpl;
}

Errors RemoteInterface::uploadFile( const UploadFileData& data, StorageType storageType, UploadResponse response ) {
    auto dataWithMD5 = data;
    std::ifstream file( dataWithMD5.file, std::ios::binary );
    if( !file ) {
        LOGGER_ERROR_RAW( cc::error( "File not found: " + data.file ) );
    }
    else {
        auto md5Checksum = CryptoUtils::Digests::md5( file );

        dataWithMD5.metadata[ md5ChecksumKey ] = md5Checksum;
    }

    return m_pimpl->uploadFile( dataWithMD5, storageType, std::move( response ) );
}

Errors RemoteInterface::uploadBackup( const UploadFileData& data,
                                      StorageType storageType,
                                      RemoteInterface::UploadResponse response ) {
    auto dataWithMD5 = data;
    std::ifstream file( dataWithMD5.file, std::ios::binary );
    if( !file ) {
        LOGGER_ERROR_RAW( cc::error( "File not found: " + data.file ) );
    }
    else {
        auto md5Checksum = CryptoUtils::Digests::md5( file );

        dataWithMD5.metadata[ md5ChecksumKey ] = md5Checksum;
    }

    return m_pimpl->uploadBackup( dataWithMD5, storageType, std::move( response ) );
}

Errors RemoteInterface::uploadLogs( const UploadFileData& data,
                                    StorageType storageType,
                                    RemoteInterface::UploadResponse response ) {
    auto dataWithMD5 = data;
    std::ifstream file( dataWithMD5.file, std::ios::binary );
    if( !file ) {
        LOGGER_ERROR_RAW( cc::error( "File not found: " + data.file ) );
    }
    else {
        auto md5Checksum = CryptoUtils::Digests::md5( file );

        dataWithMD5.metadata[ md5ChecksumKey ] = md5Checksum;
    }

    return m_pimpl->uploadLogs( dataWithMD5, storageType, std::move( response ) );
}

Errors RemoteInterface::downloadFile( const FileKey& fileKey, const Path& downloadPath, DownloadResponse response ) {
    return m_pimpl->downloadFile( fileKey, downloadPath, std::move( response ) );
}

void RemoteInterface::stopTransfer() {
    m_pimpl->stopTransfer();
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
