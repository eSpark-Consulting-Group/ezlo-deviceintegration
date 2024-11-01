#pragma once

#include "http/http_options.h"
#include "http/http_events.h"
#include "http/event_emitter.h"
#include "http/http_response.h"
#include "http/multipart_boundary_processor.h"
#include "common_utils/cpp14.h"
#include <curl/curl.h>
#include <array>
#include <mutex>
#include <atomic>

namespace HubPlatform {
namespace CommonHttp {

using CurlDeleter = void ( * )( CURL* );
using CurlPtr = std::unique_ptr< CURL, CurlDeleter >;

using CurlHeadersDeleter = void ( * )( curl_slist* );
using CurlHeadersList = std::unique_ptr< curl_slist, CurlHeadersDeleter >;

using CurlMultiDeleter = CURLMcode ( * )( CURLM* );
using CurlMultiPtr = std::unique_ptr< CURLM, CurlMultiDeleter >;

using MimeDeleter = void ( * )( curl_mime* );
using MimePtr = std::unique_ptr< curl_mime, MimeDeleter >;

struct CallbackProgress {
    size_t offset = 0;
    size_t size = 0;
    time_t timepoint{ 0 };

public:
    void reset();
};

class Connection {
public:
    Connection( HttpOptions options, HttpEventEmitter& emitter );
    Connection() = delete;
    Connection( const Connection& ) = delete;
    Connection( Connection&& ) = delete;
    virtual ~Connection();

    Connection& operator=( const Connection& ) = delete;
    Connection& operator=( Connection&& ) = delete;

    void setId( const std::string& id );
    const std::string& getId() const;
    void setPluginId( const std::string& id );
    const std::string& getPluginId() const;
    const HttpOptions& getOptions() const;
    void updateOptions( const HttpOptions& newOptions );
    const std::string& getSign() const;
    bool getNoBody() const;
    static bool fromSign( const std::string& sign, std::string& pluginId, std::string& connectionId );
    std::string makeSign();

    bool prepare();
    bool resetHandle();

    CURL* getCURL();
    size_t writeBuffer( const char* ptr, size_t length );
    void writeFile( const std::string& path, const char* ptr, size_t length );
    size_t flushBuffer( bool forceFlush = false );
    HttpResponse& response();
    const HttpResponse& response() const;

    MultipartBoundaryProcessor& getMultipartBoundaryProcessor();
    const MultipartBoundaryProcessor& getMultipartBoundaryProcessor() const;

    bool inMulti() const;
    void inMulti( bool v );

    const char* errorBuffer() const;
    void emitDataSend();
    void emitDataReceived( bool last = false );
    void emitConnectionClosed( int reasonCode, const char* message );
    void emitProgress();

    void nextPortion( std::string data, bool last );

    void verbose( bool value );
    bool verbose() const;

private:
    int prepareHeaders();
    void prepareMime();

    void setString( int code, const char* opt );
    void setLong( int code, long opt );
    void setRequestType();
    void saveHeadersList( CurlHeadersList&& );
    void signConnection();

    static size_t writeCallbackSafe( char* rawData, size_t size, size_t count, void* userData );
    static size_t readCallbackSafe( char* rawData, size_t size, size_t count, void* userData );
    static size_t headerCallbackSafe( char* rawData, size_t size, size_t count, void* userData );

    static size_t writeCallback( char* rawData, size_t size, size_t count, void* userData );
    static size_t readCallback( char* rawData, size_t size, size_t count, void* userData );
    static size_t headerCallback( char* rawData, size_t size, size_t count, void* userData );

    CallbackProgress readProgress() const;
    CallbackProgress readProgressTotal() const;
    void addReadProgressOffset( size_t offset );

    bool hasNextPortion();
    static bool exitByMessageEndMark( const char* ptr, size_t size );
    void setLastProgressPrint( time_t timePoint );
    time_t getLastProgressPrint() const;

private:
    mutable std::mutex m_mutex;

    std::string m_id;
    std::string m_pluginId;
    std::string m_sign;
    HttpOptions m_options;
    CallbackProgress m_readProgress;       // coupled with m_options.data, indicates read progress for a single portion
    CallbackProgress m_readProgressTotal;  // coupled with data_send(), indicates read progress for all portions
    CurlPtr m_curl;
    CurlHeadersList m_implHeadersList;
    MimePtr m_mime;
    std::array< char, CURL_ERROR_SIZE > m_curlError{};
    HttpEventEmitter& m_eventEmitter;

    HttpResponse m_response;
    MultipartBoundaryProcessor m_multipartBoundaryProcessor;
    bool m_inMulti{ false };
    bool m_hasNextPortion{ false };
    std::atomic< bool > m_isClosed{ false };
    int m_progressPercent = 0;
    bool m_verbose{ false };

    time_t m_lastProgressPrint{ 0 };
};

using ConnectionPtr = std::shared_ptr< Connection >;

}  // namespace CommonHttp
}  // namespace HubPlatform
