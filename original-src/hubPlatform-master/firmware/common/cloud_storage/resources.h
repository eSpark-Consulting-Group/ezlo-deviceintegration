#pragma once

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {
namespace Resources {

namespace DB {
namespace Fields {
extern const char* const Id;
extern const char* const Xapp;
extern const char* const Token;
extern const char* const TokenExpirationDate;
extern const char* const UploadUrls;
extern const char* const UploadLogsUrls;
extern const char* const DownloadUrls;
extern const char* const DownloadRedirectUrls;
extern const char* const UploadRedirectUrls;
extern const char* const UploadRedirectFinishedUrls;
extern const char* const CloudApiUrls;
}  // namespace Fields
}  // namespace DB

namespace Api {
namespace Fields {
extern const char* const Id;
extern const char* const Method;
extern const char* const Api;
extern const char* const Params;
extern const char* const Endpoints;
extern const char* const Download;
extern const char* const Upload;
extern const char* const UploadLogs;
extern const char* const RedirectDownload;
extern const char* const RedirectUpload;
extern const char* const RedirectUploadFinished;
extern const char* const Token;
extern const char* const String;
extern const char* const ExpiresTs;
extern const char* const Xapp;
extern const char* const Result;
extern const char* const StorageType;
extern const char* const DownloadUrl;
extern const char* const UploadUrl;
extern const char* const CloudApi;
}  // namespace Fields

namespace Methods {
extern const char* const CloudStorageControllerToken;
extern const char* const CloudStorageControllerTokenVersion;
}  // namespace Methods

}  // namespace Api

namespace Http {
namespace Options {
namespace Headers {
extern const char* const Xapp;
extern const char* const Type;
extern const char* const Bucket;
extern const char* const Filename;
extern const char* const FileFormat;
extern const char* const Meta;
extern const char* const Uuid;
extern const char* const TransferEncoding;
extern const char* const RedirectSupport;
extern const char* const RedirectJson;
extern const char* const OwnerEntityType;
extern const char* const Expect;
}  // namespace Headers

}  // namespace Options
namespace Response {
extern const char* const Code;
extern const char* const Data;
extern const char* const Key;
extern const char* const Uuid;
extern const char* const Reason;
extern const char* const Text;
}  // namespace Response
}  // namespace Http

namespace MetadataFields {
extern const char* const LoadType;
extern const char* const FileKey;
extern const char* const DownloadPath;
extern const char* const Attempt;
extern const char* const Step;
}  // namespace MetadataFields

const char* const CloudStorageRequestHandler = "cloudStorageRequestHandler";
const char* const StorageFile = "storageFile";
const char* const StorageFileKey = "key";
const char* const StorageFileUuid = "uuid";
const char* const StorageFileSize = "size";
const char* const UploadFileData = "uploadFileData";
const char* const UploadUuid = "uploadUuid";

namespace CloudResponse {
const char* const Status = "status";
const char* const Data = "data";
const char* const DataErrorText = "error_text";
}  // namespace CloudResponse

}  // namespace Resources
}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
