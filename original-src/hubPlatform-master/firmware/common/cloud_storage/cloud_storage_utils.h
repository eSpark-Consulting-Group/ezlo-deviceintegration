#pragma once

#include "json_utils/json_fwd.h"
#include "cloud_storage/cloud_storage_type.h"
#include "service_interfaces/service.h"

#include <chrono>
#include <functional>

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

void getFilesList( Services::Service& service,
                   StorageType storageType,
                   Services::Service::ReceiveCallback callback,
                   const std::chrono::seconds& requestTimeout );

void storagePrepareUpload( Services::Service& service,
                           PrepareUploadType prepareUploadType,
                           Services::Service::ReceiveCallback callback,
                           std::chrono::seconds requestTimeout );

JsonObject storageControllerTokenRequest( const char* apiVersion );

JsonObject storageRedirectUploadFinishedRequest( std::string id,
                                                 size_t fileSize,
                                                 std::string uploadUuid,
                                                 std::string storageFileUuid );

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
