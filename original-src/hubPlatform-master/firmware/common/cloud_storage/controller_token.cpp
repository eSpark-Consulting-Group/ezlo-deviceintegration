#include "cloud_storage/controller_token.h"
#include "provision_data_utils/provision_data_manager.h"
#include "database_client/idbclient.h"
#include "json_utils/json_wrapper.h"
#include "cloud_storage/resources.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include <chrono>

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

JsonObject ControllerToken::makeDbRecord( const std::string& xapp,
                                          const Json& download,
                                          const Json& upload,
                                          const Json& uploadLogs,
                                          const JsonArray& downloadRedirect,
                                          const JsonArray& uploadRedirect,
                                          const JsonArray& uploadRedirectFinished,
                                          const JsonArray& cloudApi,
                                          const std::string& tokenStr,
                                          std::chrono::seconds tokenExpirationDateSec ) {
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();

    auto ifArray = []( const Json& source ) -> JsonArray {
        if( !source.is_array() )
            return JSON_EMPTY_ARR;
        return source;
    };
    using namespace Resources::DB;
    return JSON(
        Fields::Xapp << xapp << Fields::Token << provision.encryptSensitiveData( tokenStr ) << Fields::UploadUrls
                     << ifArray( upload ) << Fields::UploadLogsUrls << ifArray( uploadLogs ) << Fields::DownloadUrls
                     << ifArray( download ) << Fields::DownloadRedirectUrls << ifArray( downloadRedirect )
                     << Fields::UploadRedirectUrls << ifArray( uploadRedirect ) << Fields::UploadRedirectFinishedUrls
                     << ifArray( uploadRedirectFinished ) << Fields::CloudApiUrls << ifArray( cloudApi )
                     << Fields::TokenExpirationDate << tokenExpirationDateSec.count() );
}

bool ControllerToken::fromApiRecord( const Json& source ) {
    m_tokenData.clear();

    using namespace Resources;
    try {
        auto xapp = json_get_str( source, Api::Fields::Xapp );
        auto& endpoints = source[ Api::Fields::Endpoints ];
        processUnknownEndpoints( endpoints );
        auto& download = endpoints[ Api::Fields::Download ];
        auto& upload = endpoints[ Api::Fields::Upload ];
        auto& uploadLogs = endpoints[ Api::Fields::UploadLogs ];
        // redirect support
        auto& redirectDownload = endpoints[ Api::Fields::RedirectDownload ];
        auto& redirectUpload = endpoints[ Api::Fields::RedirectUpload ];
        auto& redirectUploadFinished = endpoints[ Api::Fields::RedirectUploadFinished ];
        auto& cloudApi = endpoints[ Api::Fields::CloudApi ];
        auto& token = source[ Api::Fields::Token ];
        auto tokenStr = json_get_str( token, Api::Fields::String );
        auto tokenExpirationDateSec = ExpirationTime( json_get_int( token, Api::Fields::ExpiresTs ) );

        Json dbRecord = makeDbRecord( xapp,
                                      download,
                                      upload,
                                      uploadLogs,
                                      redirectDownload,
                                      redirectUpload,
                                      redirectUploadFinished,
                                      cloudApi,
                                      tokenStr,
                                      tokenExpirationDateSec );

        m_tokenData = std::move( dbRecord );
        return m_endpoints.init( m_tokenData, getUnknownEndpointsData() );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Failed to extract cloud storage data from api response: " ) + cc::note( ex.what() ) );
    }

    return false;
}

JsonObject ControllerToken::toDbRecord() const {
    using namespace Resources::DB;

    JsonArray upload = m_tokenData[ Fields::UploadUrls ];
    JsonArray uploadLogs = m_tokenData[ Fields::UploadLogsUrls ];
    JsonArray uploadRedirect = m_tokenData[ Fields::UploadRedirectUrls ];
    JsonArray uploadRedirectFinished = m_tokenData[ Fields::UploadRedirectFinishedUrls ];
    JsonArray download = m_tokenData[ Fields::DownloadUrls ];
    JsonArray downloadRedirect = m_tokenData[ Fields::DownloadRedirectUrls ];
    JsonArray cloudApi = m_tokenData[ Fields::CloudApiUrls ];
    auto tokenExpirationDateSec = ExpirationTime( json_get_int( m_tokenData, Fields::TokenExpirationDate ) );
    auto xapp = m_tokenData[ Fields::Xapp ];

    JsonObject dbRecord = { { Fields::Xapp, xapp },
                            { Fields::Token, m_tokenData[ Fields::Token ] },  // already encrypted
                            { Fields::UploadUrls, upload },
                            { Fields::UploadLogsUrls, uploadLogs },
                            { Fields::DownloadUrls, download },
                            { Fields::DownloadRedirectUrls, downloadRedirect },
                            { Fields::UploadRedirectUrls, uploadRedirect },
                            { Fields::UploadRedirectFinishedUrls, uploadRedirectFinished },
                            { Fields::CloudApiUrls, cloudApi },
                            { Fields::TokenExpirationDate, tokenExpirationDateSec.count() } };
    auto data = getUnknownEndpointsData();
    if( !data.empty() ) {
        dbRecord[ "unknownEndpoints" ] = data;
    }
    return dbRecord;
}

const Endpoints& ControllerToken::getEndpoints() const {
    return m_endpoints;
}

Endpoints& ControllerToken::getEndpoints() {
    return m_endpoints;
}

bool ControllerToken::fetchDbRecord( IDbClient& dbClient, ControllerToken& controllerToken ) {
    Json record = dbClient.findOne( TempCloudStorageDBName, JSON_EMPTY_OBJ, nullptr );

    ControllerToken token;
    if( !record.empty() && token.fromDbRecord( record ) ) {
        controllerToken = std::move( token );
        return true;
    }
    return false;
}

bool ControllerToken::insertDbRecord( IDbClient& dbClient, const ControllerToken& controllerToken ) {
    Json tokenDbRecord = controllerToken.toDbRecord();

    using namespace Resources::DB;
    tokenDbRecord[ Fields::Id ] = controllerToken.getDbRecordId();

    dbClient.insert( TempCloudStorageDBName, tokenDbRecord );

    return true;
}

bool ControllerToken::updateDbRecord( IDbClient& dbClient, const ControllerToken& controllerToken ) {
    Json tokenDbRecord = controllerToken.toDbRecord();

    using namespace Resources::DB;
    const bool upsert = true, multi = false;
    dbClient.update( TempCloudStorageDBName,
                     JSON( Fields::Id << controllerToken.getDbRecordId() ),
                     JSON( "$set" << tokenDbRecord ),
                     upsert,
                     multi );

    return true;
}

bool ControllerToken::initDbRecord( IDbClient& dbClient ) {
    ControllerToken controllerToken;
    JsonObject dbRecord = controllerToken.makeDbRecord( "",
                                                        JSON_EMPTY_ARR,
                                                        JSON_EMPTY_ARR,
                                                        JSON_EMPTY_ARR,
                                                        JSON_EMPTY_ARR,
                                                        JSON_EMPTY_ARR,
                                                        JSON_EMPTY_ARR,
                                                        JSON_EMPTY_ARR,
                                                        "",
                                                        std::chrono::seconds( 0 ) );
    controllerToken.fromDbRecord( dbRecord );
    controllerToken.insertDbRecord( dbClient, controllerToken );

    return true;
}

const Json& ControllerToken::getRawData() const {
    return m_tokenData;
}

const char* ControllerToken::getDbRecordId() {
    return "storage_controller_token_record";
}

bool ControllerToken::fromDbRecord( const Json& source ) {
    m_tokenData = source;
    return m_endpoints.init( m_tokenData, m_tokenData[ "unknownEndpoints" ] );
}

ControllerToken::ExpirationTime ControllerToken::getExpirationTime() const {
    return ExpirationTime{ json_get_int( m_tokenData, Resources::DB::Fields::TokenExpirationDate ) };
}

std::string ControllerToken::getTokenValue() const {
    std::string encryptedValue = json_get_str( m_tokenData, Resources::DB::Fields::Token );
    auto& provision = ProvisionDataUtils::getManager().getProvisionData();
    return provision.decryptSensitiveData( encryptedValue );
}

std::string ControllerToken::getXAppValue() const {
    return json_get_str( m_tokenData, Resources::DB::Fields::Xapp );
}

bool Endpoints::hasUploadRedirectSupport() const {
    return !m_uploadRedirectUrls.empty();
}

bool Endpoints::hasDownloadRedirectSupport() const {
    return !m_downloadRedirectUrls.empty();
}

int Endpoints::shuffleSelect( const Endpoints::Urls& collection, std::string& url ) {
    if( collection.empty() ) {
        return -1;
    }
    else if( collection.size() == 1 ) {
        url = collection[ 0 ];
        return 0;
    }

    std::srand( time( nullptr ) );
    auto randomIndex = std::rand() % collection.size();
    url = collection[ randomIndex ];
    return 0;
}

int Endpoints::selectUploadUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_uploadUrls, selectedUrl );
}

int Endpoints::selectUploadLogsUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_uploadLogsUrls, selectedUrl );
}

int Endpoints::selectDownloadUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_downloadUrls, selectedUrl );
}

int Endpoints::selectUploadRedirectUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_uploadRedirectUrls, selectedUrl );
}

int Endpoints::selectUploadRedirectFinishedUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_uploadRedirectFinishedUrls, selectedUrl );
}

int Endpoints::selectDownloadRedirectUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_downloadRedirectUrls, selectedUrl );
}

bool Endpoints::init( const JsonArray& tokenDbRecord, const JsonObject& endpointsData ) {
    using namespace Resources::DB;

    clear();

    auto makeInserter = []( Urls& collection ) {
        auto fillCollection = [&]( const JsonArray& urls ) -> void {
            for( auto& url : urls )
                collection.push_back( url );
        };
        return fillCollection;
    };

    try {
        if( tokenDbRecord.count( Fields::UploadUrls ) )
            jsonCheckArray( tokenDbRecord, Fields::UploadUrls, makeInserter( m_uploadUrls ) );
        if( tokenDbRecord.count( Fields::UploadRedirectUrls ) )
            jsonCheckArray( tokenDbRecord, Fields::UploadRedirectUrls, makeInserter( m_uploadRedirectUrls ) );
        if( tokenDbRecord.count( Fields::UploadRedirectFinishedUrls ) )
            jsonCheckArray(
                tokenDbRecord, Fields::UploadRedirectFinishedUrls, makeInserter( m_uploadRedirectFinishedUrls ) );
        if( tokenDbRecord.count( Fields::DownloadUrls ) )
            jsonCheckArray( tokenDbRecord, Fields::DownloadUrls, makeInserter( m_downloadUrls ) );
        if( tokenDbRecord.count( Fields::DownloadRedirectUrls ) )
            jsonCheckArray( tokenDbRecord, Fields::DownloadRedirectUrls, makeInserter( m_downloadRedirectUrls ) );
        if( tokenDbRecord.count( Fields::CloudApiUrls ) )
            jsonCheckArray( tokenDbRecord, Fields::CloudApiUrls, makeInserter( m_cloudApi ) );
        m_unknownEndpoints = endpointsData;
        return true;
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR_RAW( cc::error( "storage_controller_token endpoints parsing error: " ) + cc::note( ex.what() ) );
    }
    return false;
}

void Endpoints::clear() {
    m_uploadUrls.clear();
    m_downloadUrls.clear();
    m_uploadLogsUrls.clear();
    m_uploadRedirectUrls.clear();
    m_downloadRedirectUrls.clear();
    m_cloudApi.clear();
}

int Endpoints::selectCloudApiUrl( std::string& selectedUrl ) const {
    return shuffleSelect( m_cloudApi, selectedUrl );
}

Endpoints::Urls Endpoints::fetchEndpointByName( const std::string& endpointsName ) const {
    using namespace Resources::Api;
    Endpoints::Urls endpointData;
    if( endpointsName == Fields::Download ) {
        endpointData = m_downloadUrls;
    }
    else if( endpointsName == Fields::RedirectDownload ) {
        endpointData = m_downloadRedirectUrls;
    }
    else if( endpointsName == Fields::RedirectUploadFinished ) {
        endpointData = m_uploadRedirectFinishedUrls;
    }
    else if( endpointsName == Fields::RedirectUpload ) {
        endpointData = m_uploadRedirectUrls;
    }
    else if( endpointsName == Fields::Upload ) {
        endpointData = m_uploadUrls;
    }
    else if( endpointsName == Fields::UploadLogs ) {
        endpointData = m_uploadLogsUrls;
    }
    else if( endpointsName == Fields::CloudApi ) {
        endpointData = m_cloudApi;
    }
    else if( !m_unknownEndpoints.empty() ) {
        JsonArray urls;
        json_get_arr( m_unknownEndpoints, endpointsName, urls );
        if( !urls.empty() ) {
            for( auto& singleUrl : urls ) {
                endpointData.push_back( singleUrl );
            }
        }
    }

    return endpointData;
}

JsonObject Endpoints::toJson() const {
    using namespace Resources::Api;

    JsonObject result;
    if( !m_downloadUrls.empty() )
        result[ Fields::Download ] = m_downloadUrls;

    if( !m_downloadRedirectUrls.empty() )
        result[ Fields::RedirectDownload ] = m_downloadRedirectUrls;

    if( !m_uploadRedirectFinishedUrls.empty() )
        result[ Fields::RedirectUploadFinished ] = m_uploadRedirectFinishedUrls;

    if( !m_uploadRedirectUrls.empty() )
        result[ Fields::RedirectUpload ] = m_uploadRedirectUrls;

    if( !m_uploadUrls.empty() )
        result[ Fields::Upload ] = m_uploadUrls;

    if( !m_uploadLogsUrls.empty() )
        result[ Fields::UploadLogs ] = m_uploadLogsUrls;

    if( !m_cloudApi.empty() )
        result[ Fields::CloudApi ] = m_cloudApi;

    if( !m_unknownEndpoints.empty() )
        processUnknownEndpointsData( result );

    return result;
}

int Endpoints::processUnknownEndpointsData( Json& endpointData ) const {
    auto ifArray = []( const Json& source ) -> JsonArray {
        if( !source.is_array() )
            return JSON_EMPTY_ARR;
        return source;
    };

    if( !m_unknownEndpoints.empty() ) {
        for( auto it = m_unknownEndpoints.begin(); it != m_unknownEndpoints.end(); ++it ) {
            endpointData[ it.key() ] = ifArray( it.value() );
            break;
        }
    }
    return 0;
}

void ControllerToken::processUnknownEndpoints( const Json& endpointData ) {
    std::vector< std::string > keys;
    for( auto it = endpointData.begin(); it != endpointData.end(); ++it ) {
        keys.push_back( it.key() );
    }
    for( auto& singleKey : keys ) {
        auto it = std::find( m_staticEndpoints.begin(), m_staticEndpoints.end(), singleKey );
        if( it != m_staticEndpoints.end() ) {
            continue;
        }
        m_unknownEndpoints.push_back( JSON( singleKey << endpointData[ singleKey ] ) );
    }
}

JsonObject ControllerToken::getUnknownEndpointsData() const {
    Json data;
    auto ifArray = []( const Json& source ) -> JsonArray {
        if( !source.is_array() )
            return JSON_EMPTY_ARR;
        return source;
    };

    if( !m_unknownEndpoints.empty() ) {
        for( auto& singleEndpoint : m_unknownEndpoints ) {
            for( auto it = singleEndpoint.begin(); it != singleEndpoint.end(); ++it ) {
                data[ it.key() ] = ifArray( it.value() );
                break;
            }
        }
    }
    return data;
}

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
