#pragma once

#include "json_utils/json_fwd.h"
#include <chrono>

class IDbClient;

namespace HubPlatform {
namespace Common {
namespace CloudStorageInterface {

class Endpoints final {
public:
    using Urls = std::vector< std::string >;

    void clear();
    bool init( const JsonObject& tokenDbRecord, const JsonObject& endpointsData );

    bool hasUploadRedirectSupport() const;
    bool hasDownloadRedirectSupport() const;
    /** @returns >= 0 if an url is selected, -1 if there is no urls in the collection. */
    int selectUploadUrl( std::string& selectedUrl ) const;
    int selectUploadLogsUrl( std::string& selectedUrl ) const;
    int selectDownloadUrl( std::string& selectedUrl ) const;

    int selectUploadRedirectUrl( std::string& selectedUrl ) const;
    int selectUploadRedirectFinishedUrl( std::string& selectedUrl ) const;
    int selectDownloadRedirectUrl( std::string& selectedUrl ) const;
    int selectCloudApiUrl( std::string& selectedUrl ) const;
    Urls fetchEndpointByName( const std::string& endpointsName ) const;
    JsonObject toJson() const;

    /** @returns >= 0 if an url is selected, -1 if there is no urls in the collection. */
    static int shuffleSelect( const Urls& collection, std::string& url );

private:
    int processUnknownEndpointsData( Json& endpointData ) const;

private:
    Urls m_uploadUrls;
    Urls m_downloadUrls;
    Urls m_uploadLogsUrls;
    Urls m_uploadRedirectUrls;
    Urls m_uploadRedirectFinishedUrls;
    Urls m_downloadRedirectUrls;
    Urls m_cloudApi;
    JsonObject m_unknownEndpoints;
};

class ControllerToken {
public:
    using Token = std::string;
    using Xapp = std::string;
    using Path = std::string;
    using ExpirationTime = std::chrono::seconds;

public:
    bool fromApiRecord( const Json& source );
    bool fromDbRecord( const Json& source );
    JsonObject toDbRecord() const;
    const Json& getRawData() const;
    ExpirationTime getExpirationTime() const;
    std::string getTokenValue() const;
    std::string getXAppValue() const;

    const Endpoints& getEndpoints() const;
    Endpoints& getEndpoints();

    static bool fetchDbRecord( IDbClient& dbClient, ControllerToken& controllerToken );
    static bool insertDbRecord( IDbClient& dbClient, const ControllerToken& controllerToken );
    static bool updateDbRecord( IDbClient& dbClient, const ControllerToken& controllerToken );
    static bool initDbRecord( IDbClient& dbClient );

    static JsonObject makeDbRecord( const std::string& xapp,
                                    const Json& download,
                                    const Json& upload,
                                    const Json& uploadLogs,
                                    const JsonArray& downloadRedirect,
                                    const JsonArray& uploadRedirect,
                                    const JsonArray& uploadRedirectFinished,
                                    const JsonArray& cloudApi,
                                    const std::string& tokenStr,
                                    std::chrono::seconds tokenExpirationDateSec );
    static const char* getDbRecordId();

private:
    void processUnknownEndpoints( const Json& endpointData );
    JsonObject getUnknownEndpointsData() const;

private:
    Json m_tokenData;
    Endpoints m_endpoints;
    std::vector< Json > m_unknownEndpoints;
    std::vector< std::string > m_staticEndpoints = { "redirect_download", "download",
                                                     "upload_logs",       "redirect_upload_finished",
                                                     "redirect_upload",   "upload",
                                                     "cloud_api" };
};
}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform
