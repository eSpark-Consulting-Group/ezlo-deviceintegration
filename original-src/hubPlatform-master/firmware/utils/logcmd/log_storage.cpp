#include "log_storage.h"

#include <thread>
#include <atomic>
#include <condition_variable>

#include "common_utils/cpp14.h"
#include "json_utils/json_utils.h"

#include "logcmd/log_storage_impl.h"
#include "logcmd/log_exceptions.h"

namespace HubPlatform {
namespace LogCmd {

namespace {

struct ResponseHandler {
    std::mutex mutex;
    std::atomic_bool isReceived{ false };
    std::condition_variable condition;
};

void waitForResponse( ResponseHandler& handler, const std::chrono::seconds& timeout ) {
    std::unique_lock< std::mutex > lock( handler.mutex );
    handler.condition.wait_for( lock, timeout, [&] { return handler.isReceived.load(); } );
}

Json getResponseResult( const Json& fileStorageInfo, const std::string& errorMessage = "" ) {
    return JSON( "fileStorageInfo" << fileStorageInfo << "error" << errorMessage );
}

}  // namespace

LogStorage::LogStorage()
: LogStorage( std::make_unique< DefaultLogStorageImpl >() ) {}

LogStorage::LogStorage( StoragePimpl storagePimpl )
: m_storagePimpl( std::move( storagePimpl ) ) {}

LogStorage::~LogStorage() = default;

Json LogStorage::upload( const std::string& filePath, const Json& storageMetadata ) {
    bool fileUploaded = false;
    Json result;

    ResponseHandler responseHandler;
    LogStorageImpl::UploadResponse response =
        [&fileUploaded, &result, &responseHandler](
            const Json& userData, const std::string& error, const Common::CloudStorageInterface::FileKey& fileKey ) {
            if( error.empty() ) {
                result = getResponseResult( fileKey.toJson() );
                fileUploaded = true;
            }
            else {
                result = getResponseResult( JSON_EMPTY_OBJ, error );
            }

            std::lock_guard< std::mutex > lock( responseHandler.mutex );
            responseHandler.isReceived = true;
            responseHandler.condition.notify_all();
        };

    m_storagePimpl->upload( filePath, storageMetadata, response );
    waitForResponse( responseHandler, std::chrono::seconds( 30 ) );
    m_storagePimpl->stopTransfer();

    if( !fileUploaded ) {
        throw LogUploadError( "File was not uploaded: " + result.dump() );
    }

    return result;
}

}  // namespace LogCmd
}  // namespace HubPlatform
