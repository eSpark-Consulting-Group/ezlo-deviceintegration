#include "cloud_storage/cloud_storage_utils.h"
#include "cloud_storage/resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

void getFilesList( Services::Service& service,
                   StorageType storageType,
                   Services::Service::ReceiveCallback callback,
                   const std::chrono::seconds& timeout ) {
    Json params;
    params[ "type" ] = toString( storageType );
    params[ "buckets" ] = getBucketsForBackups();
    JsonObject message;
    message[ "method" ] = "cloud.storage_list";
    message[ "api" ] = "2";
    message[ "params" ] = std::move( params );

    service.sendAsyncRequest( GroupsNames::UI_GROUP, message, CLOUD_MESSAGE, std::move( callback ), timeout );
}

/** Composes NMA Route Cloud call: cloud.storage_prepare_upload, version 1. */
void storagePrepareUpload( Services::Service& service,
                           PrepareUploadType prepareUploadType,
                           Services::Service::ReceiveCallback callback,
                           std::chrono::seconds requestTimeout ) {
    Json params = JSON_EMPTY_OBJ;
    params[ "type" ] = toString( prepareUploadType );
    JsonObject message;
    message[ "method" ] = "cloud.storage_prepare_upload";
    message[ "api" ] = "1";
    message[ "params" ] = std::move( params );

    service.sendAsyncRequest( GroupsNames::UI_GROUP, message, CLOUD_MESSAGE, std::move( callback ), requestTimeout );
}

JsonObject storageControllerTokenRequest( const char* apiVersion ) {
    using namespace Resources::Api;
    if( !apiVersion || !*apiVersion ) {
        apiVersion = Methods::CloudStorageControllerTokenVersion;
    }

    JsonObject message = JSON( Fields::Method << Methods::CloudStorageControllerToken << Fields::Api << apiVersion
                                              << Fields::Params << JSON_EMPTY_OBJ );
    return message;
}

/** Composes HTTP cloud command to finish redirect upload: storage-redirect-upload-finished, v1.
 * @params id - request identifier
 * @params fileSize - size of a file in bytes.
 * @params uploadUuid - identifier of upload redirect flow (same for three steps: upload metadata, upload, body, finish
 * upload with redirect).
 * @params storageFileUuid - identifier of an uploaded file to use to download file back. */
JsonObject storageRedirectUploadFinishedRequest( std::string id,
                                                 size_t fileSize,
                                                 std::string uploadUuid,
                                                 std::string storageFileUuid ) {
    Json commandParams = {
        { "size", fileSize }, { "upload_uuid", uploadUuid }, { "storage_file_uuid", storageFileUuid }
    };
    Json cloudStorageRedirectUploadFinished = {
        { "call", "storage-redirect-upload-finished" }, { "version", "1" }, { "params", commandParams }, { "id", id }
    };

    return cloudStorageRedirectUploadFinished;
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
