#pragma once

#include "cloud_storage/cloud_storage_type.h"
#include "json_utils/json_fwd.h"
#include "http/http.h"
#include "upload_context.h"
#include <utility>

namespace HubPlatform {

namespace CommonHttp {
struct HttpOptions;
}  // namespace CommonHttp

namespace Common {
namespace CloudStorageInterface {

class HttpResponse {
public:
    void setId( std::string id );
    const std::string& getId() const;
    void setMetadata( Json metadata );
    const Json& getMetadata() const;

    void appendData( const std::int8_t* data, unsigned size );

    void setErrorReason( int code, const std::string& description );
    int getErrorReasonCode() const;
    const std::string& getErrorReasonText() const;

    void setHttpStatus( int httpStatus, const std::string& error );
    int getHttpStatus() const;
    const std::string& getHttpStatusText() const;

    bool hasError() const;
    bool hasHttpStatusError() const;
    bool hasHttpStatusNot200() const;

    void setFinished();
    bool getFinished() const;

    using Body = std::vector< char >;
    const Body& getBody() const;
    Body& getBody();

private:
    std::string m_id;
    Json m_metadata;

    Body m_body;
    int m_reasonCode{ 0 };
    std::string m_reasonText;
    int m_httpStatus{ 0 };
    std::string m_httpStatusText;
    std::string m_httpError;
    bool m_finished{ false };
};

class Dispatcher {
public:
    using RequestId = std::string;
    using OnRequestFinishedCallback = std::function< void( HttpResponse response ) >;

public:
    Dispatcher() = default;

    void registerRequest( const RequestId& requestId,
                          const Json& metadata,
                          OnRequestFinishedCallback callback,
                          UploadContextPtr uploadContextPtr );
    void dispatchDataReceived( const CommonHttp::EventPtr& event );
    void dispatchDataSend( const CommonHttp::EventPtr& event, CommonHttp::Http& http );
    void dispatchConnectionClosed( const CommonHttp::EventPtr& event );

    bool findRequest( const RequestId& requestId, HttpResponse& httpResponse ) const;

private:
    struct InnerResponse {
        HttpResponse m_response;
        OnRequestFinishedCallback m_onRequestFinished;
        UploadContextPtr m_uploadContext;
    };
    using Responses = std::map< RequestId, InnerResponse >;
    using Locker = std::unique_lock< std::mutex >;

private:
    bool popHttpResponse( const RequestId& requestId, InnerResponse& innerResponse );

private:
    mutable std::mutex m_requestsMutex;
    Responses m_httpRequests;
};

}  // namespace CloudStorageInterface
}  // namespace Common
}  // namespace HubPlatform