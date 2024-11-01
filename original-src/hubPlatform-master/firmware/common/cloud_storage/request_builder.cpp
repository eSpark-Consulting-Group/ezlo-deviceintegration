#include "request_builder.h"
#include "cloud_storage/cloud_storage_type.h"
#include "cloud_storage/controller_token.h"
#include "cloud_storage/resources.h"
#include "json_utils/json_wrapper.h"
#include "log_utils/ezlo_log.h"
#include "system_utils/filesystem.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

Errors RequestBuilder::httpOptionsPrepareDownload( CommonHttp::HttpOptions& httpOptions,
                                                   const ControllerToken& controllerToken ) const {
    const Endpoints& endpoints = controllerToken.getEndpoints();
    std::string selectedUrl;
    if( -1 == endpoints.selectDownloadUrl( selectedUrl ) ) {
        return Errors::InvalidUrl;
    }
    RequestBuilder builder;
    builder.httpOptionsSetUrl( httpOptions, selectedUrl );
    auto token = controllerToken.getTokenValue();
    builder.httpOptionsAuth( httpOptions, token );
    return Errors::Ok;
}

Errors RequestBuilder::httpOptionsPrepareDownloadRedirect( CommonHttp::HttpOptions& httpOptions,
                                                           const ControllerToken& controllerToken,
                                                           const FileKey& fileKey ) const {
    const Endpoints& endpoints = controllerToken.getEndpoints();
    auto token = controllerToken.getTokenValue();
    std::string selectedUrl;
    if( -1 == endpoints.selectDownloadRedirectUrl( selectedUrl ) ) {
        return Errors::InvalidUrl;
    }
    // selectedUrl of a form 'http://<storage-download>/?uuid=febd6610-5dc1-11ed-97d4-c9bac5f6a699&redirect_support=1'
    selectedUrl = selectedUrl + "?redirect_support=1&uuid=" + fileKey.uuid;

    httpOptionsSetUrl( httpOptions, selectedUrl );
    httpOptionsAuth( httpOptions, token );
    return Errors::Ok;
}

Errors RequestBuilder::httpOptionsPrepareUpload( CommonHttp::HttpOptions& httpOptions,
                                                 const ControllerToken& controllerToken ) const {
    const Endpoints& endpoints = controllerToken.getEndpoints();
    auto token = controllerToken.getTokenValue();

    std::string selectedUrl;
    if( -1 == endpoints.selectUploadUrl( selectedUrl ) ) {
        return Errors::InvalidUrl;
    }
    auto xapp = controllerToken.getXAppValue();
    if( xapp.empty() ) {
        return Errors::InvalidXapp;
    }
    httpOptionsSetUrl( httpOptions, selectedUrl );
    httpOptionsSetXApp( httpOptions, xapp );
    httpOptionsAuth( httpOptions, token );
    return Errors::Ok;
}

Errors RequestBuilder::httpOptionsPrepareUploadLogs( CommonHttp::HttpOptions& httpOptions,
                                                     const ControllerToken& controllerToken ) const {
    const Endpoints& endpoints = controllerToken.getEndpoints();
    auto token = controllerToken.getTokenValue();
    std::string selectedUrl;
    if( -1 == endpoints.selectUploadLogsUrl( selectedUrl ) ) {
        if( -1 == endpoints.selectUploadUrl( selectedUrl ) ) {
            return Errors::InvalidUrl;
        }
    }
    auto xapp = controllerToken.getXAppValue();
    if( xapp.empty() ) {
        return Errors::InvalidXapp;
    }
    httpOptionsSetUrl( httpOptions, selectedUrl );
    httpOptionsSetXApp( httpOptions, xapp );
    httpOptionsAuth( httpOptions, token );
    return Errors::Ok;
}

Errors RequestBuilder::httpOptionsPrepareUploadRedirect( CommonHttp::HttpOptions& httpOptions,
                                                         const ControllerToken& controllerToken ) const {
    const Endpoints& endpoints = controllerToken.getEndpoints();

    std::string selectedUrl;
    if( endpoints.selectUploadRedirectUrl( selectedUrl ) == -1 ) {
        return Errors::InvalidUrl;
    }
    auto xapp = controllerToken.getXAppValue();
    if( xapp.empty() ) {
        return Errors::InvalidXapp;
    }
    auto token = controllerToken.getTokenValue();
    if( token.empty() ) {
        return Errors::InvalidToken;
    }

    auto uploadToEndpoint = selectedUrl + "/upload";
    httpOptionsSetUrl( httpOptions, uploadToEndpoint );
    httpOptionsSetXApp( httpOptions, xapp );
    httpOptionsAuth( httpOptions, token );
    return Errors::Ok;
}

void RequestBuilder::httpOptionsSetXApp( CommonHttp::HttpOptions& httpOptions, const std::string& xapp ) const {
    httpOptions.headers.insert( { Resources::Http::Options::Headers::Xapp, xapp } );
}

void RequestBuilder::httpOptionsSetUrl( CommonHttp::HttpOptions& httpOptions, const std::string& selectedUrl ) const {
    httpOptions.url = selectedUrl;
}

void RequestBuilder::httpOptionsAuth( CommonHttp::HttpOptions& httpOptions, const std::string& token ) const {
    httpOptions.headers.insert( { "Authorization", "Bearer " + token } );
}

void RequestBuilder::debugPrint( CommonHttp::HttpOptions& httpOptions, const std::string& token ) {
    std::string logMsg = "Headers: \n";
    for( const auto& header : httpOptions.headers ) {
        logMsg += "        " + header.first + ": " + header.second + "\n";
    }
    logMsg += "    Authorization: " + token.substr( 0, token.find_last_of( '.' ) ) + "\n";
    logMsg += "    url: " + httpOptions.url + "\n";
    LOGGER_DEBUG( logMsg );
}

void RequestBuilder::httpOptionsSetFileMetaHeaders( const UploadFileData& data,
                                                    CommonHttp::HttpOptions& httpOptions ) const {
    filesystem::File::Name filename;
    filesystem::File::Extension extension;
    try {
        std::tie( filename, extension ) = filesystem::getFilenameAndExtension( data.file );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
    }
    std::string fullName = filename;
    if( !extension.empty() )
        fullName += "." + extension;
    httpOptions.headers.insert( { Resources::Http::Options::Headers::Filename, fullName } );
    httpOptions.headers.insert( { Resources::Http::Options::Headers::FileFormat, std::move( extension ) } );
    if( !data.metadata.empty() ) {
        httpOptions.headers.insert( { Resources::Http::Options::Headers::Meta, data.metadata.dump() } );
    }
    if( !data.uuid.empty() ) {
        httpOptions.headers.insert( { Resources::Http::Options::Headers::Uuid, data.uuid } );
    }
}

void RequestBuilder::httpOptionsSetStorageTypes( StorageType& storageType,
                                                 StorageBucket& storageBucket,
                                                 CommonHttp::HttpOptions& httpOptions ) const {
    httpOptions.headers.insert( { Resources::Http::Options::Headers::Type, toString( storageType ) } );
    httpOptions.headers.insert( { Resources::Http::Options::Headers::Bucket, toString( storageBucket ) } );
}

void RequestBuilder::httpOptionsSetDefaults( CommonHttp::HttpOptions& httpOptions,
                                             const char* httpMethod,
                                             const char* contentType ) const {
    httpOptions.type = httpMethod;
    httpOptions.skipSecurity = false;
    httpOptions.handler = "empty";
    httpOptions.contentType = contentType;
}

void RequestBuilder::httpOptionsSetFileBody( const UploadFileData& data, CommonHttp::HttpOptions& httpOptions ) const {
    httpOptions.mimeParts.push_back( { "file", data.file, CommonHttp::Mime::DataType::FILE } );
}

void RequestBuilder::httpOptionsSetTimeouts( CommonHttp::HttpOptions& httpOptions ) const {
    httpOptions.timeoutMs = Limits::HttpResponseTimeoutMs;
    httpOptions.connectTimeoutMs = Limits::HttpConnectionTimeoutMs;
    httpOptions.redirection = 0;
}

void RequestBuilder::httpOptionsSuppressExpect( CommonHttp::HttpOptions& httpOptions ) const {
    httpOptions.headers.insert( { Resources::Http::Options::Headers::Expect, "" } );
}

void RequestBuilder::httpOptionsUploadWithoutRedirect( CommonHttp::HttpOptions& httpOptions,
                                                       const UploadFileData& data,
                                                       StorageType storageType,
                                                       StorageBucket storageBucket ) const {
    const char* httpMethod = "POST";
    const char* contentType = "";
    httpOptionsSetDefaults( httpOptions, httpMethod, contentType );
    httpOptionsSetTimeouts( httpOptions );
    httpOptionsSetStorageTypes( storageType, storageBucket, httpOptions );

    httpOptionsSetFileMetaHeaders( data, httpOptions );
    httpOptionsSetFileBody( data, httpOptions );
    httpOptions.userData =
        JSON( "loadType" << toString( LoadType::Upload ) << "uploadFileData" << data.toJson() << "storageType"
                         << toString( storageType ) << "storageBucket" << toString( storageBucket ) << "attempt" << 1 );
}

void RequestBuilder::httpOptionsUploadRedirect( CommonHttp::HttpOptions& httpOptions,
                                                const UploadFileData& data,
                                                StorageType storageType,
                                                StorageBucket storageBucket ) const {
    const char* httpMethod = "POST";
    const char* contentType = "";
    httpOptionsSetDefaults( httpOptions, httpMethod, contentType );
    httpOptionsSetTimeouts( httpOptions );
    httpOptionsSetStorageTypes( storageType, storageBucket, httpOptions );

    httpOptionsSetFileMetaHeaders( data, httpOptions );
    // no file body

    httpOptions.headers.insert( { Resources::Http::Options::Headers::RedirectSupport, "1" } );
    httpOptions.headers.insert( { Resources::Http::Options::Headers::RedirectJson, "1" } );
    httpOptions.headers.insert(
        { Resources::Http::Options::Headers::OwnerEntityType, toString( OwnerEntityType::Controller ) } );

    httpOptions.userData = JSON( "loadType" << toString( LoadType::UploadRedirect ) << "step"
                                            << toString( UploadStepType::UploadRedirectPrepare ) << "uploadFileData"
                                            << data.toJson() << "storageType" << toString( storageType )
                                            << "storageBucket" << toString( storageBucket ) << "attempt" << 1 );
#ifdef _DEBUG
    httpOptions.verbose = true;
#endif
}

void RequestBuilder::httpOptionsUploadRedirectFileBody( CommonHttp::HttpOptions& httpOptions,
                                                        const UploadFileData& uploadFileData,
                                                        const Json& metadata ) const {
    const char* httpMethod = "PUT";
    const char* contentType = "";
    httpOptionsSetDefaults( httpOptions, httpMethod, contentType );
    httpOptionsSetTimeouts( httpOptions );
    httpOptionsSuppressExpect( httpOptions );

    httpOptions.userData = JSON( "loadType" << toString( LoadType::UploadRedirect ) << "step"
                                            << toString( UploadStepType::UploadRedirectFileData ) << "attempt" << 1 );
    jsonCopy( metadata, { "storageType", "storageBucket", "uploadFileData" }, httpOptions.userData );
#ifdef _DEBUG
    httpOptions.verbose = true;
#endif
}

/** Composes POST request to storageRedirectFinishedUrl endpoint.
 * Client code should add httpOptions.data field and httpOptions.contentLength to pass the data payload to a server. */
Errors RequestBuilder::httpOptionsUploadRedirectFinished( CommonHttp::HttpOptions& httpOptions,
                                                          const ControllerToken& controllerToken,
                                                          const UploadFileData& data,
                                                          const Json& metadata ) const {
    httpOptionsSetDefaults( httpOptions, "POST", "application/json" );
    httpOptionsSetTimeouts( httpOptions );

    httpOptions.userData = JSON( "loadType" << toString( LoadType::UploadRedirect ) << "step"
                                            << toString( UploadStepType::UploadRedirectFinished ) << "uploadFileData"
                                            << data.toJson() << "attempt" << 1 );
    jsonCopy( metadata, { "storageType", "storageBucket", "uploadFileData" }, httpOptions.userData );

    const Endpoints& endpoints = controllerToken.getEndpoints();
    auto token = controllerToken.getTokenValue();
    if( token.empty() )
        return Errors::InvalidToken;
    auto xapp = controllerToken.getXAppValue();
    if( xapp.empty() )
        return Errors::InvalidXapp;

    std::string selectedUrl;
    if( endpoints.selectUploadRedirectFinishedUrl( selectedUrl ) == -1 ) {
        return Errors::InvalidUrl;
    }

    auto uploadToEndpoint = selectedUrl + "/v1/request";
    httpOptionsSetUrl( httpOptions, uploadToEndpoint );
    httpOptionsAuth( httpOptions, token );
    httpOptionsSetXApp( httpOptions, xapp );
    httpOptionsSuppressExpect( httpOptions );

#ifdef _DEBUG
    httpOptions.verbose = true;
#endif
    return Errors::Ok;
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform