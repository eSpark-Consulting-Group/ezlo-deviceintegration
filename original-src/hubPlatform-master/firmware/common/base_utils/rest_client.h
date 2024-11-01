#pragma once
#include <curl/curl.h>

#include <memory>
#include <string>
#include <future>
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <map>
#include <stdexcept>

namespace HubPlatform {

class RestClientException : public std::runtime_error {
public:
    RestClientException( CURLcode curlCode );
    const CURLcode m_curlCode;
};

namespace UT {
class RestClientSessionTestSuite;
}  // namespace UT

///
/// \brief The RestClientSession class
/// NOT THREAD SAFE
/// Class uses curl inside. Before using, curl_global_init() should be called in you process.
/// An instance of the class uses one curl handle to make requests to one remote host, so that
/// several requests may use single connection.
/// One may use sync or async requests (sendRequest and sendRequestAsync methods). Async version
/// is executed in separate thread. Futher async calls will push request in queue. If there is any
/// executing async request, then calling sync version pushes request to the queue.
class RestClientSession {
    friend class UT::RestClientSessionTestSuite;

public:
    RestClientSession( std::string baseUrl /* scheme + host + port */ );
    ~RestClientSession();

    struct Request;
    using RequestPtr = std::shared_ptr< Request >;
    using QueryParams = std::map< std::string, std::string >;
    using Headers = std::map< std::string, std::string >;

    struct Request {
        enum class HttpMethod { GET, PUT, DELETE };

        Request( HttpMethod _method,
                 std::string _path,
                 QueryParams _queryParams,
                 std::string _body,
                 Headers _headers,
                 int _timeoutSeconds = 5 )
        : method( _method )
        , path( std::move( _path ) )
        , queryParams( std::move( _queryParams ) )
        , body( std::move( _body ) )
        , headers( _headers )
        , timeoutSeconds( _timeoutSeconds ) {}

        // Simple request
        Request( HttpMethod _method, std::string _path, Headers _headers = {}, int _timeoutSeconds = 5 )
        : Request( _method, std::move( _path ), {}, {}, std::move( _headers ), _timeoutSeconds ) {}

        // Simple GET request
        Request( std::string _path, QueryParams _queryParams = {}, int _timeoutSeconds = 5 )
        : Request( HttpMethod::GET, std::move( _path ), std::move( _queryParams ), {}, {}, _timeoutSeconds ) {}

        HttpMethod method;
        std::string path;
        QueryParams queryParams;
        std::string body;
        Headers headers;
        int timeoutSeconds;
    };

    struct Response {
        int status = 400;
        std::string body;
    };

    ///
    /// \brief throws RestClientException if curl returns code other than CURLE_OK
    /// \param request
    /// \return Response struct
    ///
    Response sendRequest( const RequestPtr& request );

    ///
    /// \brief sets RestClientException exception in returned future
    /// if curl returns code other than CURLE_OK
    /// \param request
    /// \return future with Response struct
    ///
    std::future< Response > sendRequestAsync( const RequestPtr& request );

private:
    void startBackgroundWorker();
    Response curlPerform( const RequestPtr& request );
    std::string formUrl( const RequestPtr& request );

    struct RequestContext {
        RequestContext( const RequestPtr& req = {} )
        : request( req ) {}
        RequestPtr request;
        std::promise< Response > responsePromise;
    };

    struct ReadDataStruct {
        ReadDataStruct( std::string* _body, size_t _pos )
        : body( _body )
        , pos( _pos ) {}
        const std::string* body;
        size_t pos;
    };

    static size_t readDataFunc( char* buffer, size_t size, size_t nitems, void* userdata );
    static size_t writeDataFunc( void* ptr, size_t size, size_t nitems, void* stream );

private:
    const std::string m_baseUrl;

    CURL* m_curl;
    std::queue< RequestContext > m_requestsQueue;
    std::mutex m_queueMutex;
    std::thread m_worker;
    std::atomic_bool m_cancelled{ false };
    std::atomic_bool m_workerInProgress{ false };
};

}  // namespace HubPlatform
