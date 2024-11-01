#include "cloud_storage/resources.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {
namespace Resources {

namespace DB {
namespace Fields {
const char* const Id = "_id";
const char* const Xapp = "xapp";
const char* const Token = "token";
const char* const TokenExpirationDate = "tokenExpirationDate";

const char* const UploadUrls = "uploadUrls";
const char* const UploadLogsUrls = "uploadLogsUrls";
const char* const DownloadUrls = "downloadUrls";

const char* const DownloadRedirectUrls = "downloadRedirectUrls";
const char* const UploadRedirectUrls = "uploadRedirectUrls";
const char* const UploadRedirectFinishedUrls = "uploadRedirectFinishedUrls";
const char* const CloudApiUrls = "cloudApiUrls";
}  // namespace Fields
}  // namespace DB

namespace Api {
namespace Fields {
const char* const Id = "id";
const char* const Method = "method";
const char* const Api = "api";
const char* const Params = "params";
const char* const Endpoints = "endpoints";
const char* const Download = "download";
const char* const Upload = "upload";
const char* const UploadLogs = "upload_logs";
const char* const RedirectDownload = "redirect_download";
const char* const RedirectUpload = "redirect_upload";
const char* const RedirectUploadFinished = "redirect_upload_finished";
const char* const Token = "token";
const char* const String = "string";
const char* const ExpiresTs = "expires_ts";
const char* const Xapp = "xapp";
const char* const Result = "result";
const char* const StorageType = "type";
const char* const DownloadUrl = "downloadUrl";
const char* const UploadUrl = "uploadUrl";
const char* const CloudApi = "cloud_api";
}  // namespace Fields

namespace Methods {
const char* const CloudStorageControllerToken = "cloud.storage_controller_token";
const char* const CloudStorageControllerTokenVersion = "1";
}  // namespace Methods

}  // namespace Api

namespace Http {
namespace Options {
namespace Headers {
const char* const Xapp = "X-APP";
const char* const Type = "p-type";
const char* const Bucket = "p-bucket";
const char* const Filename = "p-filename";
const char* const FileFormat = "p-file_format";
const char* const Meta = "p-meta";
const char* const Uuid = "p-uuid";
const char* const RedirectSupport = "p-redirect_support";
const char* const RedirectJson = "p-redirect_json";
const char* const OwnerEntityType = "p-owner_entity_type";
const char* const TransferEncoding = "transfer-encoding";
const char* const Expect = "expect";
}  // namespace Headers
}  // namespace Options

namespace Response {
const char* const Code = "code";
const char* const Data = "data";
const char* const Key = "key";
const char* const Uuid = "uuid";
const char* const Reason = "reason";
const char* const Text = "text";
}  // namespace Response
}  // namespace Http

namespace MetadataFields {
const char* const LoadType = "loadType";
const char* const FileKey = "fileKey";
const char* const DownloadPath = "downloadPath";
const char* const Attempt = "attempt";
const char* const Step = "step";
}  // namespace MetadataFields

}  // namespace Resources
}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
