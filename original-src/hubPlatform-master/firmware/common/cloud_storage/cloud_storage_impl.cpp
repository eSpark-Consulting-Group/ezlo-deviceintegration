#include "cloud_storage/cloud_storage_impl.h"
#include "cloud_storage/controller_token.h"
#include "cloud_storage/request_builder.h"
#include "cloud_storage/cloud_storage_utils.h"
#include "cloud_storage/exceptions.h"
#include "cloud_storage/resources.h"
#include "database_client/idbclient.h"
#include "database_client/dbid.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "system_utils/filesystem.h"
#include "http/http_events.h"
#include "http/resources.h"
#include "common_utils/streambuf_from_vector.h"
#include "common_utils/cpp14.h"
#include "base_utils/utils.h"

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

CloudStorageImpl::CloudStorageImpl( IDbClient& dbClient )
: m_dbClient( dbClient ) {
    setHttpReceivingMethod();
    setHttpClosingConnectionMethod();
    setHttpDataSendMethod();
    // set exit delay interval to avoid exit on every http request end
    exitDelayInterval( std::chrono::seconds( 90 ) );
    m_dispatcher = std::make_unique< Dispatcher >();
}

CloudStorageImpl::~CloudStorageImpl() {
    if( m_http.joinable() ) {
        m_http.join();
        LOGGER_TRACE_RAW( cc::trace( "CloudStorageImpl: Exit from join in plugin " ) );
    }
}

Errors CloudStorageImpl::uploadFile( const UploadFileData& data, StorageType storageType, UploadResponse response ) {
    auto bucket = getBucket( storageType );
    LOGGER_INFO( cc::info( "Upload file to the bucket: " ) + cc::note( toString( bucket ) ) );
    return uploadFile( LoadType::Upload, data, storageType, bucket, std::move( response ) );
}

Errors CloudStorageImpl::uploadBackup( const UploadFileData& data,
                                       StorageType storageType,
                                       CloudStorageImpl::UploadResponse response ) {
    auto bucket = getBucket( storageType );
    LOGGER_INFO( cc::info( "Upload backup to bucket: " ) + cc::note( toString( bucket ) ) );
    return uploadFile( LoadType::Upload, data, storageType, bucket, std::move( response ) );
}

Errors CloudStorageImpl::uploadLogs( const UploadFileData& data,
                                     StorageType storageType,
                                     CloudStorageImpl::UploadResponse response ) {
    auto bucket = getBucket( storageType );
    LOGGER_INFO( cc::info( "Upload logs to bucket: " ) + cc::note( toString( bucket ) ) );
    return uploadFile( LoadType::UploadLogs, data, storageType, bucket, std::move( response ) );
}

/* Uploads file without or with redirect according to the storage_controller_token command output.
 * @param response - callback( String error, FileKey fileKey ) to pass response data to
 * */
Errors CloudStorageImpl::uploadFile( LoadType loadType,
                                     const UploadFileData& data,
                                     StorageType storageType,
                                     StorageBucket storageBucket,
                                     CloudStorageImpl::UploadResponse responseCallback ) {
    // Choose load method
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( m_dbClient, controllerToken );
    const Endpoints& endpoints = controllerToken.getEndpoints();
    LoadType exactLoadType = selectLoadType( loadType, endpoints );
    CommonHttp::HttpOptions httpOptions;
    RequestBuilder builder;

    auto token = controllerToken.getTokenValue();
    if( token.empty() ) {
        return Errors::InvalidToken;
    }
    auto xapp = controllerToken.getXAppValue();
    if( xapp.empty() ) {
        return Errors::InvalidXapp;
    }
    if( exactLoadType == LoadType::UploadLogs ) {
        builder.httpOptionsUploadWithoutRedirect( httpOptions, data, storageType, storageBucket );
        auto result = builder.httpOptionsPrepareUploadLogs( httpOptions, controllerToken );
        if( result != Errors::Ok )
            return result;
    }
    else if( exactLoadType == LoadType::Upload ) {
        builder.httpOptionsUploadWithoutRedirect( httpOptions, data, storageType, storageBucket );
        auto result = builder.httpOptionsPrepareUpload( httpOptions, controllerToken );
        if( result != Errors::Ok )
            return result;
    }
    else if( exactLoadType == LoadType::UploadRedirect ) {
        builder.httpOptionsUploadRedirect( httpOptions, data, storageType, storageBucket );
        auto result = builder.httpOptionsPrepareUploadRedirect( httpOptions, controllerToken );
        if( result != Errors::Ok )
            return result;
    }

    Errors error;
    CommonHttp::Http::RequestId requestId;
    std::tie( error, requestId ) = sendHttpRequest( httpOptions );
    if( error == Errors::Ok ) {
        std::lock_guard< std::mutex > lock( m_mutex );
        if( exactLoadType == LoadType::Upload || exactLoadType == LoadType::UploadLogs ) {
            m_uploadResponses.insert( { requestId, std::move( responseCallback ) } );
        }
        else {
            Dispatcher::OnRequestFinishedCallback onUploadRedirectStepOneCallback =
                [this, responseCallback]( HttpResponse response ) {
                    return this->onUploadRedirectStepOneCallback( std::move( response ), responseCallback );
                };
            m_dispatcher->registerRequest( requestId, httpOptions.userData, onUploadRedirectStepOneCallback, nullptr );
        }
    }
    return error;
}

Errors CloudStorageImpl::downloadFile( const FileKey& fileKey,
                                       const Path& downloadPath,
                                       DownloadResponse downloadCallback ) {
    // Choose load method
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( m_dbClient, controllerToken );
    const Endpoints& endpoints = controllerToken.getEndpoints();
    LoadType exactLoadType = selectLoadType( LoadType::Download, endpoints );

    return downloadFile( controllerToken, exactLoadType, fileKey, downloadPath, std::move( downloadCallback ) );
}

Errors CloudStorageImpl::downloadFile( const ControllerToken& controllerToken,
                                       LoadType exactLoadType,
                                       const FileKey& fileKey,
                                       const Path& downloadPath,
                                       DownloadResponse downloadCallback ) {
    using namespace Resources::Http::Options;
    using namespace Resources;

    CommonHttp::HttpOptions httpOptions;

    auto token = controllerToken.getTokenValue();
    if( token.empty() ) {
        return Errors::InvalidToken;
    }

    RequestBuilder builder;
    if( exactLoadType == LoadType::Download ) {
        auto httpMethod = "POST";
        auto contentType = "application/json";
        builder.httpOptionsSetDefaults( httpOptions, httpMethod, contentType );
        builder.httpOptionsSetTimeouts( httpOptions );
        httpOptions.headers.insert( { Headers::TransferEncoding, "chunked" } );
        httpOptions.data = fileKey.toJson().dump();
        httpOptions.receivedDataFilePath = downloadPath;
        httpOptions.userData = JSON( MetadataFields::LoadType
                                     << toString( LoadType::Download ) << MetadataFields::FileKey << fileKey.toJson()
                                     << MetadataFields::DownloadPath << downloadPath << MetadataFields::Attempt << 1 );

        auto result = builder.httpOptionsPrepareDownload( httpOptions, controllerToken );
        if( result != Errors::Ok )
            return result;
    }
    else if( exactLoadType == LoadType::DownloadRedirect ) {
        auto httpMethod = "GET";
        auto contentType = "application/json";
        builder.httpOptionsSetDefaults( httpOptions, httpMethod, contentType );
        builder.httpOptionsSetTimeouts( httpOptions );
        httpOptions.receivedDataFilePath = downloadPath;
        httpOptions.userData =
            JSON( MetadataFields::LoadType
                  << toString( LoadType::DownloadRedirect ) << MetadataFields::Step
                  << toString( DownloadStepType::DownloadRedirectPrepare ) << MetadataFields::FileKey
                  << fileKey.toJson() << MetadataFields::DownloadPath << downloadPath << MetadataFields::Attempt << 1 );

        auto result = builder.httpOptionsPrepareDownloadRedirect( httpOptions, controllerToken, fileKey );
        if( result != Errors::Ok )
            return result;
        httpOptions.redirection = 3;
    }

    Errors error;
    CommonHttp::Http::RequestId requestId;
    std::tie( error, requestId ) = sendHttpRequest( httpOptions );
    if( error == Errors::Ok ) {
        std::lock_guard< std::mutex > lock( m_mutex );
        if( exactLoadType == LoadType::Download ) {
            m_downloadsInfo.insert( { requestId, { std::move( downloadCallback ), "" } } );
        }
        else if( exactLoadType == LoadType::DownloadRedirect ) {
            auto onDownloadFinished = [this, downloadCallback, controllerToken]( const HttpResponse& response ) {
                return this->onDownloadFinishedCallback( response, controllerToken, downloadCallback );
            };
            m_dispatcher->registerRequest( requestId, httpOptions.userData, onDownloadFinished, nullptr );
        }
    }
    return error;
}

void CloudStorageImpl::stopTransfer() {
    m_http.stop();
}

/** Method serves functions: upload, upload logs, download, simultaneously. */
std::pair< Errors, CommonHttp::Http::RequestId > CloudStorageImpl::sendHttpRequest(
    CommonHttp::HttpOptions& httpOptions ) {
    try {
        ControllerToken controllerToken;
        ControllerToken::fetchDbRecord( m_dbClient, controllerToken );
        auto token = controllerToken.getTokenValue();
        if( token.empty() ) {
            return { Errors::InvalidToken, {} };
        }
        RequestBuilder::debugPrint( httpOptions, token );
        auto requestId = m_http.request( Resources::CloudStorageRequestHandler, httpOptions );
        return { Errors::Ok, requestId };
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( ex.what() );
        return { Errors::RequestError, {} };
    }
}

void CloudStorageImpl::setHttpReceivingMethod() {
    using namespace CommonHttp::Resources;
    auto eventName = HttpEvents::HttpDataReceived;
    m_http.on( eventName, [this]( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
        httpLegacyDataReceived( eventHandler, event );
        httpDataReceived( eventHandler, event );
    } );
}

/** Register event handler to perform streaming of uploaded data. */
void CloudStorageImpl::setHttpDataSendMethod() {
    using namespace CommonHttp::Resources;
    const auto eventName = HttpEvents::HttpDataSend;
    m_http.on( eventName, [this]( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
        httpDataSend( eventHandler, event );
    } );
}

void CloudStorageImpl::setHttpClosingConnectionMethod() {
    using namespace CommonHttp::Resources;
    m_http.on( HttpEvents::HttpConnectionClosed,
               [this]( const std::string& eventHandler, const CommonHttp::EventPtr& event ) {
                   httpLegacyConnectionClosed( eventHandler, event );
                   httpConnectionClosed( eventHandler, event );
               } );
}

void CloudStorageImpl::httpLegacyDataReceived( const std::string&, const CommonHttp::EventPtr& event ) {
    using namespace CommonHttp::Resources;

    if( !event.get() )
        return;
    std::lock_guard< std::mutex > lock( m_mutex );
    auto requestId = event->id();
    auto uploadResponseIt = m_uploadResponses.find( requestId );
    if( uploadResponseIt != m_uploadResponses.end() ) {
        auto response = uploadResponseIt->second;
        m_uploadResponses.erase( uploadResponseIt );
        sendResponse( event->toJson(), response, HttpEvents::HttpDataReceived );
        return;
    }
    auto downloadInfoIt = m_downloadsInfo.find( requestId );
    if( downloadInfoIt != m_downloadsInfo.end() ) {
        using namespace Resources::Http::Response;
        auto info = downloadInfoIt->second;
        auto message = event->toJson();
        auto code = json_get_int( message, Code );
        if( info.error.empty() && code != 200 ) {
            json_get( message, std::vector< Json >{ Reason, Text }, info.error );
        }
        return;
    }

    // Print error only for legacy requests
    const Json& metadata = event->userData();
    std::string loadType = json_get_str( metadata, Resources::MetadataFields::LoadType );
    std::string loadStep = json_get_str( metadata, Resources::MetadataFields::Step );

    if( loadType.empty() || loadStep.empty() ) {
        LOGGER_ERROR( cc::error( "Legacy request record not found: " ) + event->id() );
    }
}

void CloudStorageImpl::httpLegacyConnectionClosed( const std::string& /* eventHandler */,
                                                   const CommonHttp::EventPtr& event ) {
    using namespace CommonHttp::Resources;

    if( !event.get() )
        return;
    std::lock_guard< std::mutex > lock( m_mutex );
    auto requestId = event->id();
    auto uploadResponseIt = m_uploadResponses.find( requestId );
    if( uploadResponseIt != m_uploadResponses.end() ) {
        auto response = uploadResponseIt->second;
        m_uploadResponses.erase( uploadResponseIt );
        sendResponse( event->toJson(), response, HttpEvents::HttpConnectionClosed );
        return;
    }
    auto downloadInfoIt = m_downloadsInfo.find( requestId );
    if( downloadInfoIt != m_downloadsInfo.end() ) {
        using namespace Resources::Http::Response;
        auto info = downloadInfoIt->second;
        auto message = event->toJson();
        int reasonCode;
        json_get( message, std::vector< Json >{ Reason, Code }, reasonCode );
        if( info.error.empty() && static_cast< CommonHttp::Reason >( reasonCode ) != CommonHttp::Reason::NONE ) {
            json_get( message, std::vector< Json >{ Reason, Text }, info.error );
        }
        info.response( event->userData(), info.error );
        m_downloadsInfo.erase( downloadInfoIt );
        return;
    }
}

/** Sends error or fileKey to response( error, fileKey ) callback.
 * */
void CloudStorageImpl::sendResponse( const Json& message,
                                     const CloudStorageImpl::UploadResponse& callback,
                                     const std::string& errorMessage ) {
    try {
        using namespace Resources::Http::Response;
        std::string error;
        FileKey fileKey;
        Json userData;
        if( json_get_int( message, Code ) == 200 ) {
            auto data = ::parseJson( json_get_str( message, Data ) );
            fileKey.key = json_get_str( data[ "file" ], Key );
            fileKey.uuid = json_get_str( data[ "file" ], Uuid );
            fileKey.md5Checksum = json_get_str( data[ "file" ], "checksum" );
        }
        else {
            json_get( message, std::vector< Json >{ Reason, Text }, error );
        }
        json_get( message, "user_data", userData );
        if( callback != nullptr ) {
            callback( userData, error, fileKey );
        }
        else {
            LOGGER_ERROR( cc::error( "uploadResponse callback is not passed" ) );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( errorMessage + ": " + ex.what() );
    }
}

LoadType CloudStorageImpl::selectLoadType( LoadType baseLoadType, const Endpoints& endpoints ) {
    std::string selectedUrl;
    if( baseLoadType == LoadType::Download ) {
        if( endpoints.hasDownloadRedirectSupport() )
            return LoadType::DownloadRedirect;
        return LoadType::Download;
    }
    else if( baseLoadType == LoadType::UploadLogs ) {
        if( endpoints.selectUploadLogsUrl( selectedUrl ) > -1 ) {
            return LoadType::UploadLogs;
        }
        if( endpoints.selectUploadRedirectUrl( selectedUrl ) > -1 ) {
            return LoadType::UploadRedirect;
        }
        if( endpoints.selectUploadUrl( selectedUrl ) > -1 ) {
            return LoadType::Upload;
        }
        return baseLoadType;
    }
    else if( baseLoadType == LoadType::Upload ) {
        if( endpoints.selectUploadRedirectUrl( selectedUrl ) > -1 ) {
            return LoadType::UploadRedirect;
        }
    }
    return baseLoadType;
}

void CloudStorageImpl::httpDataReceived( const std::string& /* eventHandler */, const CommonHttp::EventPtr& event ) {
    if( event == nullptr )
        return;
    m_dispatcher->dispatchDataReceived( event );
}

void CloudStorageImpl::httpDataSend( const std::string& /* eventHandler */, const CommonHttp::EventPtr& event ) {
    if( event == nullptr )
        return;
    m_dispatcher->dispatchDataSend( event, m_http );
}

void CloudStorageImpl::httpConnectionClosed( const std::string& /* eventHandler */,
                                             const CommonHttp::EventPtr& event ) {
    if( event == nullptr )
        return;
    m_dispatcher->dispatchConnectionClosed( event );
}

void CloudStorageImpl::waitTillFinish() {
    if( m_http.joinable() ) {
        m_http.join();
        LOGGER_TRACE( cc::trace( "Waiting for http join finished" ) );
    }
}

void CloudStorageImpl::exitDelayInterval( std::chrono::seconds delay ) {
    m_http.exitDelayInterval( delay.count() );
}

std::string CloudStorageImpl::makeHttpErrorMessage( const HttpResponse& response ) {
    std::string errorMessage;
    if( !response.hasError() ) {
        return "";
    }
    if( response.getHttpStatus() == 0 ) {
        errorMessage = "Http error reason: " + std::to_string( response.getErrorReasonCode() ) +
                       ", text: " + response.getErrorReasonText();
    }
    else {
        errorMessage = "Wrong http status: " + std::to_string( response.getHttpStatus() ) +
                       ", text: " + response.getHttpStatusText();
    }
    return errorMessage;
}

Json& CloudStorageImpl::makeReplacements( Json& stepOneResponse ) const {
    if( !m_replacements.empty() ) {
        auto from = json_get_str( m_replacements, "from" );
        auto to = json_get_str( m_replacements, "to" );

        Json url;
        if( json_get( stepOneResponse, { "data", "url" }, url ) ) {
            auto storageUrl = json_get_str( url );

            replaceAll( storageUrl, from, to );
            if( storageUrl != json_get_str( stepOneResponse, "url" ) ) {
                stepOneResponse[ "data" ][ "url" ] = storageUrl;
            }
        }
    }
    return stepOneResponse;
}

bool CloudStorageImpl::onUploadRedirectStepOneCallback( HttpResponse response,
                                                        const UploadResponse& responseCallback ) {
    LOGGER_TRACE( cc::trace( "responseId " ) + response.getId() + cc::trace( ", response body size: " ) +
                  cc::num10( response.getBody().size() ) );

    if( response.hasError() || response.hasHttpStatusNot200() ) {
        if( responseCallback ) {
            std::string errorMessage = makeHttpErrorMessage( response );
            responseCallback( response.getMetadata(), errorMessage, {} );
            return false;
        }
    }

    namespace MetadataFields = Resources::MetadataFields;
    const Json& metadata = response.getMetadata();
    std::string loadType = json_get_str( metadata, MetadataFields::LoadType );
    std::string loadStep = json_get_str( metadata, MetadataFields::Step );

    try {
        CommonUtils::streambufFromVector< char > streamBuf( response.getBody() );
        std::istream is( &streamBuf );
        Json stepOneResponse;

        try {
            stepOneResponse = Json::parse( is );
            LOGGER_TRACE( cc::trace( "stepOneResponse: " ) + cc::j( stepOneResponse ) );

            stepOneResponse = makeReplacements( stepOneResponse );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR( cc::error( "Couldn't parse json from step one response body: " ) + ex.what() );
        }
        int status = json_get_int( stepOneResponse, Resources::CloudResponse::Status );
        if( status != 1 ) {
            throw CloudResponseStatusError( "upload redirect: incorrect status field: " + cc::num10( status ) );
        }

        Json stepOneData;
        if( !json_get( stepOneResponse, Resources::CloudResponse::Data, stepOneData ) ) {
            throw WrongFieldError( "incorrect format of step one data" );
        }

        std::string storageFileKey = json_get_str( stepOneData, "storage_file_key" );
        std::string storageFileUuid = json_get_str( stepOneData, "storage_file_uuid" );
        std::string uploadUuid = json_get_str( stepOneData, "upload_uuid" );
        std::string stepTwoUrl = json_get_str( stepOneData, "url" );
        Json uploadFileDataJson;
        if( !json_get( metadata, Resources::UploadFileData, uploadFileDataJson ) )
            throw WrongFieldError( "Invalid uploadFileData" );

        // compose step two request
        {
            CommonHttp::HttpOptions httpOptions;
            httpOptions.verbose = Log::getLogLevel() == Log::TRACE;

            httpOptions.skipSecurity = true;
            httpOptions.contentType = "application/octet-stream";
            httpOptions.handler = "empty";
            httpOptions.url = stepTwoUrl;

            Json fileMetadata;
            json_get( uploadFileDataJson, "metadata", fileMetadata );
            std::string filePath = json_get_str( uploadFileDataJson, "file" );

            UploadFileData uploadFileData( filePath, storageFileUuid, fileMetadata );
            RequestBuilder builder;

            builder.httpOptionsUploadRedirectFileBody( httpOptions, uploadFileData, metadata );
            Json userData;

            uint64_t storageFileSize = filesystem::getFileSize( filePath );
            Json storageFile =
                JSON( Resources::StorageFileKey << storageFileKey << Resources::StorageFileUuid << storageFileUuid
                                                << Resources::StorageFileSize << storageFileSize
                                                << Resources::UploadUuid << uploadUuid );
            httpOptions.userData[ Resources::StorageFile ] = storageFile;
            json_add_str( httpOptions.userData, "parentRequestId", response.getId() );

            httpOptions.verbose = Log::getLogLevel() >= Log::TRACE;

            auto uploadContextPtr = makeFileUploadContext( filePath );
            if( !uploadContextPtr->endOfStream() ) {
                httpOptions.data = uploadContextPtr->readPortion( PORTION_SIZE_DEFAULT );
                httpOptions.last = ( storageFileSize <= PORTION_SIZE_DEFAULT );
            }
            if( httpOptions.last )
                uploadContextPtr.reset();

            auto requestId = m_http.request( Resources::CloudStorageRequestHandler, httpOptions );

            auto onUploadRedirectFileBodyComplete = [this, responseCallback]( const HttpResponse& response ) {
                return this->onUploadRedirectFileBodyCallback( response, responseCallback );
            };

            m_dispatcher->registerRequest(
                requestId, httpOptions.userData, onUploadRedirectFileBodyComplete, uploadContextPtr );
        }

        return true;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "onUploadRedirectStepOneCallback failed: " ) + e.what() );
        if( responseCallback )
            responseCallback( response.getMetadata(), std::string{ "Exception caught: " } + e.what(), {} );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "onUploadRedirectStepOneCallback failed: ..." ) );
        if( responseCallback )
            responseCallback( response.getMetadata(), std::string{ "Unknown exception caught" }, {} );
    }
    return false;
}

bool CloudStorageImpl::onUploadRedirectFileBodyCallback( HttpResponse response,
                                                         const UploadResponse& responseCallback ) {
    LOGGER_TRACE( cc::trace( "responseId " ) + response.getId() + cc::trace( ", response body size: " ) +
                  cc::num10( response.getBody().size() ) );

    if( response.hasError() || response.hasHttpStatusNot200() ) {
        if( responseCallback ) {
            std::string errorMessage = makeHttpErrorMessage( response );
            LOGGER_ERROR( cc::error( "Http error: " ) + errorMessage );

            responseCallback( response.getMetadata(), errorMessage, {} );
            return false;
        }
    }

    CommonHttp::HttpOptions httpOptions;
    ControllerToken controllerToken;
    ControllerToken::fetchDbRecord( m_dbClient, controllerToken );

    RequestBuilder builder;

    const Json& userData = response.getMetadata();

    Json storageFileData;
    json_get( userData, Resources::StorageFile, storageFileData );

    int fileSize = json_get_int( storageFileData, Resources::StorageFileSize );
    auto storageFileUuid = json_get_str( storageFileData, Resources::StorageFileUuid );
    auto uploadUuid = json_get_str( storageFileData, Resources::UploadUuid );

    Json uploadFileData;
    json_get( userData, Resources::UploadFileData, uploadFileData );
    auto filePath = json_get_str( uploadFileData, "file" );

    Json fileMetadata;
    json_get( uploadFileData, "metadata", fileMetadata );

    UploadFileData uploadData( filePath, uploadUuid, fileMetadata );

    // userData/storageFile = { key, storageFileUuid, size, uploadUuid }
    builder.httpOptionsUploadRedirectFinished( httpOptions, controllerToken, uploadData, userData );
    httpOptions.userData[ Resources::StorageFile ] = storageFileData;

    Json uploadFinishedCommand =
        storageRedirectUploadFinishedRequest( db::DbId::genOID(), fileSize, uploadUuid, storageFileUuid );
    httpOptions.data = uploadFinishedCommand.dump();
    LOGGER_TRACE( cc::trace( "Step 3 payload: " ) + cc::j( uploadFinishedCommand ) );

    httpOptions.contentLength = httpOptions.data.size();
    httpOptions.verbose = Log::getLogLevel() >= Log::TRACE;

    auto requestId = m_http.request( Resources::CloudStorageRequestHandler, httpOptions );

    auto onUploadRedirectFinished = [responseCallback]( const HttpResponse& response ) {
        return CloudStorageImpl::onUploadRedirectFinishedCallback( response, responseCallback );
    };
    m_dispatcher->registerRequest( requestId, httpOptions.userData, onUploadRedirectFinished, nullptr );

    return true;
}

bool CloudStorageImpl::onUploadRedirectFinishedCallback( HttpResponse response,
                                                         const UploadResponse& responseCallback ) {
    LOGGER_TRACE( cc::trace( "response id: " ) + cc::str( response.getId() ) );

    if( !response.hasError() && !response.hasHttpStatusNot200() ) {
        try {
            CommonUtils::streambufFromVector< char > streamBuf( response.getBody() );
            std::istream is( &streamBuf );
            Json stepThreeResponse;
            try {
                stepThreeResponse = Json::parse( is );
                LOGGER_TRACE( cc::trace( "Show response of Step 3: " ) + cc::j( stepThreeResponse ) );
            }
            catch( const std::exception& ex ) {
                LOGGER_ERROR( cc::error( "Couldn't parse json from step three response body: " ) + ex.what() );
            }

            int status = json_get_int( stepThreeResponse, Resources::CloudResponse::Status );
            if( status != 1 ) {
                Json errorText;
                json_get( stepThreeResponse,
                          { Resources::CloudResponse::Data, Resources::CloudResponse::DataErrorText },
                          errorText );
                const std::string cloudError = json_get_str( errorText );
                throw CloudResponseStatusError( "upload redirect finished: incorrect status field: " +
                                                std::to_string( status ) + ", error: '" + cloudError + "'" );
            }

            const Json& userData = response.getMetadata();
            FileKey fileKey;
            Json storageFileData;
            if( json_get( userData, Resources::StorageFile, storageFileData ) ) {
                fileKey.key = json_get_str( storageFileData, Resources::StorageFileKey );
                fileKey.uuid = json_get_str( storageFileData, Resources::StorageFileUuid );
            }
            else {
                LOGGER_ERROR( cc::error( "Couldn't fetch storage file data!" ) );
            }

            Json stepThreeData;
            if( !json_get( stepThreeResponse, Resources::CloudResponse::Data, stepThreeData ) ) {
                throw WrongFieldError( "incorrect format of step three data" );
            }

            if( responseCallback )
                responseCallback( response.getMetadata(), "", fileKey );

            return true;
        }
        catch( const std::exception& e ) {
            Log::error( cc::error( "onUploadRedirectFinishedCallback failed: " ) + e.what() );
            if( responseCallback )
                responseCallback( response.getMetadata(), std::string{ "Exception caught: " } + e.what(), {} );
        }
        catch( ... ) {
            Log::error( cc::error( "onUploadRedirectFinishedCallback failed: ..." ) );
            if( responseCallback )
                responseCallback( response.getMetadata(), std::string{ "Unknown exception caught" }, {} );
        }
    }
    else {
        std::string errorMessage = makeHttpErrorMessage( response );
        Log::error( cc::error( errorMessage ) );
        if( responseCallback )
            responseCallback( response.getMetadata(), errorMessage, {} );
    }

    return false;
}

void CloudStorageImpl::onDownloadFinishedCallback( const HttpResponse& response,
                                                   const ControllerToken& controllerToken,
                                                   const CloudStorageImpl::DownloadResponse& downloadCallback ) {
    LOGGER_TRACE( cc::trace( "responseId: " ) + response.getId() );

    if( downloadCallback != nullptr ) {
        if( response.getErrorReasonCode() != 0 ) {
            using namespace Resources;
            // exit with error
            bool exitDownload = false;
            FileKey fileKey;
            {
                Json fileKeyJson;
                if( !json_get_obj( response.getMetadata(), MetadataFields::FileKey, fileKeyJson ) ) {
                    LOGGER_ERROR(
                        cc::error( "Unable to fetch FileKey from request metadata, Exit onDownloadFinished." ) );
                    exitDownload = true;
                }
                fileKey.fromJson( fileKeyJson );
            }
            std::string downloadPath = json_get_str( response.getMetadata(), MetadataFields::DownloadPath );

            if( exitDownload ) {
                downloadCallback( response.getMetadata(), response.getErrorReasonText() );
            }
            else {
                LOGGER_DEBUG( cc::debug( "Try download without redirect, parent requestId: " ) + response.getId() );
                downloadFile( controllerToken, LoadType::Download, fileKey, downloadPath, downloadCallback );
                return;
            }
        }
        else {
            // exit with Ok
            downloadCallback( response.getMetadata(), "" );
        }
    }
}

void CloudStorageImpl::replaceExternalUrlRule( const JsonObject& replacements ) {
    m_replacements = replacements;
}

/** Finds bucket that corresponds to the storage type. */
StorageBucket CloudStorageImpl::getBucket( StorageType storageType, StorageBucket defaultBucket ) {
    switch( storageType ) {
    case StorageType::ArchiveControllerBackup:
    case StorageType::Backup:
    case StorageType::ControllerBackup:
        return StorageBucket::ControllerBackups;

    case StorageType::ArchiveControllerLog:
    case StorageType::ControllerLog:
        return StorageBucket::ControllerLogs;

    case StorageType::Multimedia:
        return StorageBucket::Media;
    default:
        return defaultBucket;
    }
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
