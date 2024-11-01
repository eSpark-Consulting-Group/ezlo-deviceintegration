#include "base_utils/rest_client.h"
#include "log_utils/ezlo_log.h"
#include <cstring>

namespace HubPlatform {

RestClientSession::RestClientSession( std::string baseUrl )
: m_baseUrl( std::move( baseUrl ) )
, m_curl( curl_easy_init() ) {
    if( !m_curl )
        throw RestClientException( CURLE_FAILED_INIT );

    curl_easy_setopt( m_curl, CURLOPT_NOSIGNAL, 1L );
}

RestClientSession::~RestClientSession() {
    if( m_worker.joinable() ) {
        m_cancelled = true;
        m_worker.join();
    }
    curl_easy_cleanup( m_curl );
}

RestClientSession::Response RestClientSession::sendRequest( const RequestPtr& request ) {
    if( m_workerInProgress ) {
        return sendRequestAsync( request ).get();
    }
    return curlPerform( request );
}

std::future< RestClientSession::Response > RestClientSession::sendRequestAsync( const RequestPtr& request ) {
    auto requestContext = RequestContext( request );
    auto future = requestContext.responsePromise.get_future();
    {
        std::lock_guard< std::mutex > guard( m_queueMutex );
        m_requestsQueue.emplace( std::move( requestContext ) );
    }

    if( !m_workerInProgress ) {
        startBackgroundWorker();
    }

    return future;
}

void RestClientSession::startBackgroundWorker() {
    if( m_worker.joinable() )
        m_worker.join();

    m_workerInProgress = true;
    m_worker = std::thread( [this] {
        while( !m_cancelled ) {
            RequestContext requestContext;
            {
                std::lock_guard< std::mutex > guard( m_queueMutex );
                if( m_requestsQueue.empty() )
                    break;
                requestContext = std::move( m_requestsQueue.front() );
                m_requestsQueue.pop();
            }
            if( m_cancelled )
                break;

            try {
                auto response = curlPerform( requestContext.request );
                requestContext.responsePromise.set_value( std::move( response ) );
            }
            catch( ... ) {
                try {
                    requestContext.responsePromise.set_exception( std::current_exception() );
                }
                catch( ... ) {
                    Log::error( "Internal error in RestClientSession" );
                }
            }
        }
        m_workerInProgress = false;
    } );
}

size_t RestClientSession::readDataFunc( char* buffer, size_t size, size_t nitems, void* userdata ) {
    size_t curlSize = size * nitems;
    ReadDataStruct* data = reinterpret_cast< ReadDataStruct* >( userdata );
    size_t copySize = std::min( data->body->size() - data->pos, curlSize );
    std::memcpy( buffer, &( data->body[ data->pos ] ), copySize );
    data->pos += copySize;
    return copySize;
}

size_t RestClientSession::writeDataFunc( void* ptr, size_t size, size_t nitems, void* stream ) {
    reinterpret_cast< std::string* >( stream )->append( reinterpret_cast< char* >( ptr ), 0, size * nitems );
    return size * nitems;
}

RestClientSession::Response RestClientSession::curlPerform( const RequestPtr& request ) {
    Response response;
    curl_easy_reset( m_curl );
    curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, writeDataFunc );
    curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, &response.body );
    curl_easy_setopt( m_curl, CURLOPT_URL, formUrl( request ).c_str() );
    curl_easy_setopt( m_curl, CURLOPT_TIMEOUT, request->timeoutSeconds );

    curl_slist* headersList = nullptr;
    for( const auto pair : request->headers ) {
        const std::string headerString = pair.first + ": " + pair.second;
        headersList = curl_slist_append( headersList, headerString.c_str() );
    }

    if( request->method == Request::HttpMethod::PUT ) {
        curl_easy_setopt( m_curl, CURLOPT_UPLOAD, 1L );
        curl_easy_setopt( m_curl, CURLOPT_READFUNCTION, readDataFunc );
        ReadDataStruct readDataStruct( &request->body, 0 );
        curl_easy_setopt( m_curl, CURLOPT_READDATA, &readDataStruct );
        curl_easy_setopt( m_curl, CURLOPT_INFILESIZE, request->body.size() );
    }
    else if( request->method == Request::HttpMethod::DELETE ) {
        curl_easy_setopt( m_curl, CURLOPT_CUSTOMREQUEST, "DELETE" );
    }

    if( headersList )
        curl_easy_setopt( m_curl, CURLOPT_HTTPHEADER, headersList );

    CURLcode curlCode = curl_easy_perform( m_curl );
    curl_slist_free_all( headersList );
    if( curlCode == CURLE_OK ) {
        long statusCode = 0;
        curlCode = curl_easy_getinfo( m_curl, CURLINFO_RESPONSE_CODE, &statusCode );
        if( curlCode != CURLE_OK ) {
            Log::warn( "curl_easy_getinfo() with CURLINFO_RESPONSE_CODE returned " + std::to_string( curlCode ) );
        }
        response.status = statusCode ? statusCode : 400;
    }
    else {
        throw RestClientException( curlCode );
    }

    return response;
}

std::string RestClientSession::formUrl( const RequestPtr& request ) {
    std::string url = m_baseUrl + request->path;

    if( !request->queryParams.empty() ) {
        auto urlEncoder = [this]( const std::string& input ) -> std::string {
            char* encoded = curl_easy_escape( m_curl, input.c_str(), input.length() );
            if( encoded ) {
                const std::string result( encoded );
                curl_free( encoded );
                return result;
            }
            else {
                throw RestClientException( CURLE_BAD_CONTENT_ENCODING );
            }
        };

        auto it = request->queryParams.cbegin();
        auto itEnd = request->queryParams.cend();

        url += "?" + urlEncoder( it->first ) + "=" + urlEncoder( it->second );
        for( ++it; it != itEnd; ++it ) {
            url += "&" + urlEncoder( it->first ) + "=" + urlEncoder( it->second );
        }
    }

    return url;
}

RestClientException::RestClientException( CURLcode curlCode )
: std::runtime_error( "Curl returned with code " + std::to_string( ( curlCode ) ) )
, m_curlCode( curlCode ) {}

}  // namespace HubPlatform
