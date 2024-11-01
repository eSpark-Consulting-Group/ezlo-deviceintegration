#include "http/connection.h"
#include "http/http_error.h"
#include "http/http_response.h"
#include "log_utils/ezlo_log.h"
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <tiny/tiny_consolecolors.h>
#include <cassert>
#include <cstring>
#include <thread>
#include <chrono>
#include <fstream>
#include <utility>

namespace HubPlatform {
namespace CommonHttp {

using UniqueLock = std::unique_lock< std::mutex >;

Connection::Connection( HttpOptions options, HttpEventEmitter& emitter )
: m_options( std::move( options ) )
, m_curl(::curl_easy_init(), &curl_easy_cleanup )
, m_implHeadersList( nullptr, curl_slist_free_all )
, m_mime( curl_mime_init( m_curl.get() ), &curl_mime_free )
, m_eventEmitter( emitter )
, m_verbose( options.verbose ) {
    m_readProgress.size = m_options.data.size();
    curl_easy_setopt( m_curl.get(), CURLOPT_NOSIGNAL, 1L );
}

Connection::~Connection() = default;

void Connection::setId( const std::string& id ) {
    m_id = id;
}

const std::string& Connection::getId() const {
    return m_id;
}

void Connection::setPluginId( const std::string& id ) {
    m_pluginId = id;
}
const std::string& Connection::getPluginId() const {
    return m_pluginId;
}

CURL* Connection::getCURL() {
    return m_curl.get();
}

const HttpOptions& Connection::getOptions() const {
    UniqueLock lock( m_mutex );
    return m_options;
}

void Connection::setString( int code, const char* opt ) {
    if( opt )
        ::curl_easy_setopt( m_curl.get(), static_cast< CURLoption >( code ), opt );
}

void Connection::setLong( int code, long opt ) {
    ::curl_easy_setopt( m_curl.get(), static_cast< CURLoption >( code ), opt );
}

bool Connection::prepare() {
    UniqueLock lock( m_mutex );
    m_options.validate();

    if( m_options.url.empty() )
        throw OptionsError( std::string( Resources::HTTP_ERROR_PARAM_NOT_DEFINED ) + ": url" );
    if( m_options.handler.empty() )
        throw OptionsError( std::string( Resources::HTTP_ERROR_PARAM_NOT_DEFINED ) + ": handler" );
    setString( CURLOPT_URL, m_options.url.c_str() );

    auto setNotEmpty = [this]( CURLoption option, const char* value ) {
        if( !value || !*value )
            return;
        setString( option, value );
    };

    if( m_options.skipSecurity ) {
        setLong( CURLOPT_SSL_VERIFYPEER, 0 );
        setLong( CURLOPT_SSL_VERIFYHOST, 0 );
    }
    else {
        setNotEmpty( CURLOPT_CAINFO, m_options.certs.ca.c_str() );

        setNotEmpty( CURLOPT_SSLCERT, m_options.certs.publicKey.c_str() );
        setNotEmpty( CURLOPT_SSLKEY, m_options.certs.privateKey.c_str() );
    }

    setRequestType();
    prepareHeaders();
    prepareMime();

    setNotEmpty( CURLOPT_USERNAME, m_options.user.c_str() );
    setNotEmpty( CURLOPT_PASSWORD, m_options.password.c_str() );

    if( m_options.redirection > 0 ) {
        setLong( CURLOPT_FOLLOWLOCATION, 1L );
        setLong( CURLOPT_MAXREDIRS, m_options.redirection );
    }
    setLong( CURLOPT_TIMEOUT_MS, m_options.timeoutMs );
    setLong( CURLOPT_CONNECTTIMEOUT_MS, m_options.connectTimeoutMs );
    signConnection();

    curl_easy_setopt( m_curl.get(), CURLOPT_HEADERFUNCTION, &Connection::headerCallbackSafe );
    curl_easy_setopt( m_curl.get(), CURLOPT_HEADERDATA, this );
    curl_easy_setopt( m_curl.get(), CURLOPT_WRITEFUNCTION, &Connection::writeCallbackSafe );
    curl_easy_setopt( m_curl.get(), CURLOPT_WRITEDATA, this );
    curl_easy_setopt( m_curl.get(), CURLOPT_READFUNCTION, &Connection::readCallbackSafe );
    curl_easy_setopt( m_curl.get(), CURLOPT_READDATA, this );

    if( Log::getLogLevel() >= Log::PriorityLevel::DEBUG && verbose() )
        setLong( CURLOPT_VERBOSE, 1L );

    /** FAILONERROR:
     *      1: Exit with error on http status codes >= 400
     *      0: allow access to a failed report body
     */
    setLong( CURLOPT_FAILONERROR, ( m_options.failOnError ) ? 1L : 0 );

    curl_easy_setopt( m_curl.get(), CURLOPT_ERRORBUFFER, m_curlError.begin() );

    if( !m_options.receivedDataFilePath.empty() ) {
        std::ofstream( m_options.receivedDataFilePath, std::ofstream::trunc );  // erase file
    }

    auto interface = m_options.interface;
    if( !interface.empty() ) {
        std::string interfaceParam = std::string{ "if!" }.append( interface );
        curl_easy_setopt( m_curl.get(), CURLOPT_INTERFACE, interfaceParam.c_str() );
    }

    return true;
}

void Connection::setRequestType() {
    const auto& type = m_options.type;
    if( type == "GET" )
        setLong( CURLOPT_HTTPGET, 1L );
    else if( type == "PUT" )
        setLong( CURLOPT_PUT, 1L );
    else if( type == "POST" )
        setLong( CURLOPT_POST, 1L );
    else {
        setString( CURLOPT_CUSTOMREQUEST, type.c_str() );
        if( type == "HEAD" ) {
            setLong( CURLOPT_NOBODY, 1l );
        }
        else if( type == "PATCH" ) {
            setLong( CURLOPT_UPLOAD, 1L );
        }
    }
}

size_t Connection::writeCallbackSafe( char* rawData, size_t size, size_t count, void* userData ) {
    try {
        return writeCallback( rawData, size, count, userData );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG_RAW( cc::error( "Common/http: write callback: " ) + cc::attention( ex.what() ) );
        return 0;
    }
}

size_t Connection::writeCallback( char* rawData, size_t size, size_t count, void* userData ) {
    auto* connPtr = static_cast< Connection* >( userData );
    if( connPtr == nullptr )
        throw HttpConnectionError( "Connection::writeCallback: connection user data is empty!" );

    Connection& conn = *connPtr;
    auto sign = conn.getSign();

    if( conn.verbose() ) {
        Log::print( Log::PriorityLevel::TRACE,
                    "CURL: Write callback for id: %s, size: %u, count: %u",
                    sign.c_str(),
                    size,
                    count );
    }

    auto& response = conn.response();
    if( response.getHttpStatus() == 0 || response.getHttpStatus() == 100 ) {
        long httpCode = 0L;
        if( conn.getCURL() ) {
            curl_easy_getinfo( conn.getCURL(), CURLINFO_RESPONSE_CODE, &httpCode );
            response.setHttpStatus( static_cast< int >( httpCode ) );
        }
    }
    bool isRepeatAllowed = conn.getOptions().repeatAllowed;

    const size_t bytesTaken = count * size;
    auto& mbp = conn.getMultipartBoundaryProcessor();
    mbp.processBytes( rawData, bytesTaken );
    auto& filePath = conn.getOptions().receivedDataFilePath;
    if( filePath.empty() ) {
        conn.writeBuffer( rawData, bytesTaken );
    }
    else {
        conn.writeFile( filePath, rawData, bytesTaken );
    }
    bool shouldFlush = false;
    if( conn.getOptions().newDataCache ) {
        bool exitByContentLength =
            response.getContentLength() > 0 && response.getContentLength() <= response.getBytesSaved();

        if( exitByContentLength || exitByMessageEndMark( rawData, bytesTaken ) ) {
            // if repeat expected, don't flush here exit from receiving loop by content-length header
            shouldFlush = true;
        }
        else if( !mbp.isLastBoundaryEndHandled() && mbp.getLastBoundaryEnd() <= response.getBytesSaved() ) {
            mbp.setLastBoundaryEndHandled();
            shouldFlush = true;
        }
    }
    else {
        shouldFlush = true;
    }

    if( shouldFlush ) {
        // If repeat_allowed and body is complete, buffer will be flushed later
        if( response.getContentLength() != response.getBytesSaved() || !isRepeatAllowed ) {
            conn.flushBuffer();
        }
    }

    return bytesTaken;
}

bool Connection::exitByMessageEndMark( const char* ptr, size_t size ) {
    static constexpr char messageEndMark[] = "\n\n";
    static constexpr size_t markLength = sizeof( messageEndMark ) - 1;

    bool longEnough = size > markLength;
    if( longEnough ) {
        auto offset = size - markLength;
        const char* messageEnd = ptr + offset;

        return 0 == memcmp( messageEnd, messageEndMark, markLength );
    }
    return false;
}

size_t Connection::readCallbackSafe( char* rawData, size_t size, size_t count, void* userData ) {
    try {
        return readCallback( rawData, size, count, userData );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG_RAW( cc::error( "Common/http: Read callback: " ) + cc::attention( ex.what() ) );
        return CURL_READFUNC_ABORT;
    }
}

size_t Connection::readCallback( char* rawData, size_t size, size_t count, void* userData ) {
    auto* connPtr = static_cast< Connection* >( userData );
    if( connPtr == nullptr )
        throw HttpConnectionError( "Connection::readCallback: connection user data is empty!" );

    Connection& conn = *connPtr;
    auto sign = conn.getSign();

    const HttpOptions& options = conn.getOptions();
    const auto& data = options.data;
    if( data.empty() && options.last )
        return 0;  // end of stream, only for last = true

    CallbackProgress readProgress = conn.readProgress();
    assert( readProgress.size == options.data.size() );
    while( readProgress.offset < readProgress.size && readProgress.size == options.data.size() ) {
        size_t portion = size * count;
        size_t bytesLeft = readProgress.size - readProgress.offset;
        if( bytesLeft <= portion ) {
            // send all portion
            portion = bytesLeft;
        }
        else {
            // send part of portion and move offset
        }
        memcpy( rawData, data.data() + readProgress.offset, portion );
        // readProgress.offset += portion;
        conn.addReadProgressOffset( portion );

        if( conn.verbose() ) {
            CallbackProgress totalProgress = conn.readProgressTotal();

            // Print progress message: once a second, but print last record every time
            const auto deltaPrintInterval = 10;
            auto printDelta = std::abs( totalProgress.timepoint - conn.getLastProgressPrint() );
            if( printDelta >= deltaPrintInterval ) {
                Log::print( Log::PriorityLevel::TRACE,
                            "CURL: Read callback for id: %s, read buffer size: %u, portion to write: %u,"
                            " offset: %u/%u (%.3g%%), total: %u/%u",
                            sign.c_str(),
                            size * count,
                            portion,
                            readProgress.offset + portion,
                            readProgress.size,
                            readProgress.offset * 100.0f / readProgress.size,
                            totalProgress.offset + readProgress.offset + portion,
                            totalProgress.size + readProgress.size );
                conn.setLastProgressPrint( time( nullptr ) );
            }
        }
        return portion;
    }

    if( !options.last ) {
        // raise http_data_send and loop till it received (or timeout on multi-handle will arrive)
        conn.emitDataSend();

        while( true ) {
            // TODO: check timeout here  for the case of http_data_send event handler is crashed to
            //      prevent infinite loop here.
            if( conn.hasNextPortion() ) {
                // continue reading loop, go deep in recursion
                return Connection::readCallback( rawData, size, count, userData );
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

            // Exit if connection closed or timeout
            if( !conn.inMulti() )
                break;
        }
    }
    return 0;  // end of read
}

size_t Connection::headerCallbackSafe( char* rawData, size_t size, size_t count, void* userData ) {
    try {
        return headerCallback( rawData, size, count, userData );
    }
    catch( const std::exception& ex ) {
        LOGGER_DEBUG_RAW( cc::error( "Common/http: Headers callback: " ) + cc::attention( ex.what() ) );
        return 0;
    }
}

/** A callback function to pass to the curl library.
 * @param rawData - raw pointed to data passed, can have no zero termination character.
 * @return number of bytes taken
 * */
size_t Connection::headerCallback( char* rawData, size_t size, size_t count, void* userData ) {
    auto* connPtr = static_cast< Connection* >( userData );
    if( connPtr == nullptr )
        throw HttpConnectionError( "Connection::headerCallback: connection user data is empty!" );
    auto& response = connPtr->response();
    auto& mbp = connPtr->getMultipartBoundaryProcessor();

    const size_t bytesTaken = size * count;
    std::string s( rawData, bytesTaken );
    const auto infinity = std::string::npos;
    char delimiter[] = ": ";
    char rn[] = "\r\n";
    auto delimiterPos = s.find( delimiter );
    auto rnPos = s.find( rn, delimiterPos < infinity ? delimiterPos : 0 );

    if( delimiterPos != infinity && rnPos != infinity ) {
        const char contentLength[] = "content-length";
        if( strncasecmp( rawData, contentLength, sizeof( contentLength ) - 1 ) == 0 ) {
            auto* value = s.data() + delimiterPos + sizeof( delimiter ) - 1;
            char* pEnd = nullptr;
            response.setContentLength( strtoul( value, &pEnd, 10 ) );
            response.setBytesSaved( 0 );
        }

        std::string name{ s.data(), delimiterPos };
        std::string value{ s.data() + delimiterPos + sizeof( delimiter ) - 1, s.data() + rnPos };
        mbp.processHeader( name, value );
        response.addHeader( name, std::move( value ) );
    }
    else if( rnPos != infinity ) {
        // Parse header value like
        // "HTTP/1.1 200 OK\r\n"
        unsigned offset = sizeof( "HTTP/" ) - 1;
        auto spacePos = s.find( ' ', offset );

        if( spacePos != std::string::npos ) {
            // clear previously collected data (like headers) for the connection
            if( !response.getHeaders().empty() ) {
                LOGGER_DEBUG( cc::debug( "Clear existing headers for connection" ) );
                response.clear();
            }
            HttpStatus httpStatus;
            if( httpStatus.parse( s.c_str() + spacePos + 1, "" ) ) {
                response.setHttpStatus( httpStatus.code(), httpStatus.reason() );
            }
        }
    }

    return bytesTaken;
}

void Connection::emitDataReceived( bool last ) {
    auto& response = this->response();
    last |= response.getContentLength() == response.getBytesSaved();
    auto& buffer = response.getBuffer();
    if( verbose() ) {
        Log::print( Log::TRACE, "emitDataReceived: %u, last: %s", buffer.size(), last ? "true" : "false" );
    }

    auto eventPtr =
        std::make_unique< DataReceivedEvent >( getId(), m_options.url, m_options.userData, m_options.multipleHeaders );
    if( !eventPtr )
        throw HttpError( "DataReceivedEvent could not be allocated" );

    auto& event = *eventPtr;
    {
        event.code( response.getHttpStatus() );
        event.headers( response.getHeaders() );
        if( !buffer.empty() )
            event.data( std::string( reinterpret_cast< char* >( buffer.data() ), buffer.size() ) );
        event.last( last );
    }

    EventPtr ptr( eventPtr.release() );
    m_eventEmitter.dataReceived( this->m_options.handler, std::move( ptr ) );

    // free buffer
    buffer.resize( 0 );
}

void Connection::emitDataSend() {
    auto eventPtr = std::make_unique< DataSendEvent >( getId(), m_options.url, m_options.userData );
    if( !eventPtr )
        throw HttpError( "DataReceivedEvent could not be allocated" );
    eventPtr->maxDataLength( Limits::SEND_DATA_MAX_DATA_LENGTH );

    EventPtr ptr( eventPtr.release() );
    m_eventEmitter.dataSendEvent( m_options.handler, std::move( ptr ) );
}

void Connection::emitConnectionClosed( int reasonCode, const char* message ) {
    if( m_isClosed )  // can be emitted only once!
        return;

    m_isClosed = true;

    auto eventPtr = std::make_unique< ConnectionClosedEvent >( getId(), m_options.url, m_options.userData );
    if( !eventPtr )
        throw HttpError( "DataReceivedEvent could not be allocated" );

    auto& event = *eventPtr;
    auto value = static_cast< Reason >( reasonCode );
    event.setReason( value );
    bool searchHttpStatus =
        ( value == Reason::UNKNOWN_ERROR || value == Reason::CLOSED_BY_SERVER || value == Reason::NONE );
    if( searchHttpStatus && message && *message ) {
        event.reasonText( message );

        // TODO: extract this to an own method
        // "The requested URL returned error: 404 Not Found"
        std::string pattern = "The requested URL returned error: ";
        HttpStatus httpStatus;
        if( httpStatus.parse( message, pattern ) ) {
            event.httpStatus( httpStatus.code(), httpStatus.reason() );
        }
    }
    else {
        LOGGER_ERROR( cc::error( "event.http_connection_closed.httpStatus not set, some other Http status: " ) +
                      cc::num10( static_cast< int >( value ) ) );
    }

    if( event.httpCode() != response().getHttpStatus() ) {
        event.httpStatus( response().getHttpStatus(), response().getHttpReason() );
    }

    EventPtr ptr( eventPtr.release() );
    m_eventEmitter.connectionClosed( m_options.handler, std::move( ptr ) );
}

void Connection::emitProgress() {
    auto& response = this->response();
    auto contentLength = response.getContentLength();
    size_t progressPercent = 0;
    if( contentLength != 0 ) {
        progressPercent = response.getBytesSaved() * 100 / contentLength;
    }
    if( progressPercent == 100 ) {
        m_progressPercent = progressPercent;
        auto eventPtr = std::make_unique< ProgressEvent >( getId(), m_progressPercent );
        m_eventEmitter.progressEvent( this->m_options.handler, std::move( eventPtr ) );
    }
    else if( progressPercent - m_progressPercent >= Limits::PROGRESS_PERCENT_PORTION ) {
        m_progressPercent = progressPercent;
        auto eventPtr = std::make_unique< ProgressEvent >( getId(), m_progressPercent );
        m_eventEmitter.progressEvent( this->m_options.handler, std::move( eventPtr ) );
    }
}

size_t Connection::writeBuffer( const char* ptr, size_t length ) {
    auto& response = this->response();
    auto& buffer = response.getBuffer();
    if( buffer.capacity() == 0 )
        buffer.reserve( m_options.newDataBuffer );

    if( verbose() ) {
        Log::print( Log::TRACE, "Connection::writeBuffer %u", length );
    }
    size_t bytesLeft = length;
    size_t offset = 0;
    while( bytesLeft > 0 ) {
        size_t freeBytes = buffer.capacity() - buffer.size();
        size_t portion = bytesLeft > freeBytes ? freeBytes : bytesLeft;
        buffer.insert( end( buffer ), ptr + offset, ptr + offset + portion );

        bytesLeft -= portion;
        offset += portion;
        response.appendBytesSaved( portion );

        if( buffer.size() == m_options.newDataBuffer ) {
            // If repeat_allowed and body is complete, buffer will be flushed later
            if( response.getContentLength() != response.getBytesSaved() || !getOptions().repeatAllowed ) {
                emitDataReceived();
                buffer.resize( 0 );
            }
        }
    }
    emitProgress();
    return buffer.size();
}

void Connection::writeFile( const std::string& path, const char* ptr, size_t length ) {
    std::ofstream file( path, std::ios::app | std::ios::binary );
    file.write( ptr, length );
    auto& response = this->response();
    response.appendBytesSaved( length );
    emitProgress();
}

/** Flushes buffer by sending its data to http_data_received event.
 *  If forceFlush is true, emits event even with empty data/length to indicate that connection is
 *      ready to repeat_request() call.
 * @param forceFlush - it true, emits event even if the buffer is empty
 * */
size_t Connection::flushBuffer( bool forceFlush ) {
    auto& buffer = response().getBuffer();
    size_t portion = buffer.size();

    bool doFlush = forceFlush || portion > 0 || getNoBody();
    if( doFlush ) {
        emitDataReceived( forceFlush );
        buffer.resize( 0 );
        buffer.shrink_to_fit();
    }
    return portion;
}

int Connection::prepareHeaders() {
    curl_slist* slist = nullptr;
    int counter = 0;

    const std::string headerContentType = "Content-Type";
    const std::string headerContentLength = "Content-Length";
    Headers customHeaders;

    if( !m_options.contentType.empty() ) {
        customHeaders.emplace( headerContentType, m_options.contentType );
    }
    if( m_options.contentLength > 0 ) {
        customHeaders.emplace( headerContentLength, std::to_string( m_options.contentLength ) );
    }
    if( m_options.keepAlive ) {
        customHeaders.emplace( "Connection", "Keep-Alive" );
    }

    if( m_options.type == "POST" && !m_options.data.empty() ) {
        /* Uncomment the next line to enable HTTP POST with body by default.
         * Now it requires to add a transfer-encoding: chunked to the headers. */
        // customHeaders.emplace( "Transfer-Encoding", "chunked" );
    }

    auto appendHeader = [&slist, &counter]( const Headers::value_type& header ) -> void {
        auto const& name = header.first;
        auto const& value = header.second;
        if( name.empty() )
            return;

        std::string headerStr{ name + ": " + value };
        curl_slist* tmp = ::curl_slist_append( slist, headerStr.c_str() );
        if( nullptr == tmp ) {
            // throw memory error here
            curl_slist_free_all( slist );
            slist = nullptr;
            throw OptionsError( "Could not allocate HTTP header for " + name );
        }
        slist = tmp;
        counter++;
    };

    for( const auto& header : m_options.headers ) {
        appendHeader( header );
    }
    for( const auto& header : customHeaders ) {
        appendHeader( header );
    }

    saveHeadersList( CurlHeadersList( slist, ::curl_slist_free_all ) );

    return counter;
}

void Connection::prepareMime() {
    if( m_options.mimeParts.empty() ) {
        return;
    }
    for( const auto& part : m_options.mimeParts ) {
        curl_mimepart* partPtr = curl_mime_addpart( m_mime.get() );
        switch( part.dataType ) {
        case Mime::DataType::FILE:
            curl_mime_name( partPtr, part.name.c_str() );
            curl_mime_filedata( partPtr, part.data.c_str() );
            break;
        case Mime::DataType::RAW:
            curl_mime_name( partPtr, part.name.c_str() );
            curl_mime_data( partPtr, part.data.c_str(), part.data.size() );
            break;
        }
    }
    curl_easy_setopt( m_curl.get(), CURLOPT_MIMEPOST, m_mime.get() );
}

void Connection::saveHeadersList( CurlHeadersList&& newHeaders ) {
    m_implHeadersList = std::move( newHeaders );
    curl_easy_setopt( m_curl.get(), CURLOPT_HTTPHEADER, m_implHeadersList.get() );
}

void Connection::signConnection() {
    m_sign = makeSign();

    setString( CURLOPT_PRIVATE, m_sign.c_str() );
}

std::string Connection::makeSign() {
    return m_pluginId + ":" + m_id;
}

bool Connection::fromSign( const std::string& sign, std::string& pluginId, std::string& connectionId ) {
    const auto npos = std::string::npos;
    auto pos = sign.find( ':' );
    if( npos != pos ) {
        pluginId = sign.substr( 0, pos );
        connectionId = sign.substr( pos + 1, npos );
        return !pluginId.empty() && !connectionId.empty();
    }
    return false;
}

const std::string& Connection::getSign() const {
    return m_sign;
}

HttpResponse& Connection::response() {
    return m_response;
}

const HttpResponse& Connection::response() const {
    return m_response;
}

MultipartBoundaryProcessor& Connection::getMultipartBoundaryProcessor() {
    return m_multipartBoundaryProcessor;
}

const MultipartBoundaryProcessor& Connection::getMultipartBoundaryProcessor() const {
    return m_multipartBoundaryProcessor;
}

const char* Connection::errorBuffer() const {
    return m_curlError.begin();
}

CallbackProgress Connection::readProgress() const {
    UniqueLock lock( m_mutex );
    return m_readProgress;
}

bool Connection::getNoBody() const {
    UniqueLock lock( m_mutex );
    return m_options.type == "HEAD";
}

bool Connection::inMulti() const {
    UniqueLock lock( m_mutex );
    return m_inMulti;
}

void Connection::inMulti( bool value ) {
    UniqueLock lock( m_mutex );
    m_inMulti = value;
}

void Connection::nextPortion( std::string data, bool last ) {
    UniqueLock lock( m_mutex );

    m_options.data = std::move( data );
    m_options.last = last;

    m_readProgressTotal.offset += m_readProgress.offset;

    m_readProgress.reset();
    m_readProgress.size = m_options.data.size();
    m_readProgress.timepoint = time( nullptr );

    m_readProgressTotal.size += m_readProgress.size;
    m_readProgressTotal.timepoint = time( nullptr );

    m_hasNextPortion = true;
}

bool Connection::hasNextPortion() {
    UniqueLock lock( m_mutex );
    bool value = m_hasNextPortion;

    if( value && m_readProgress.offset > 0 )
        m_hasNextPortion = false;
    return value && m_readProgress.offset == 0;
}

void Connection::addReadProgressOffset( size_t offset ) {
    UniqueLock lock( m_mutex );
    m_readProgress.offset += offset;
    m_readProgress.timepoint = time( nullptr );
}

void setNotEmpty( std::string& to, const std::string& from ) {
    if( !from.empty() )
        to = from;
}
template < typename T >
void setNotEmpty( T& to, const T& from, T empty = T() ) {
    if( from != empty )
        to = from;
}

void Connection::updateOptions( const HttpOptions& newOptions ) {
    UniqueLock lock( m_mutex );

    m_options.type = newOptions.type;
    m_options.contentType = newOptions.contentType;
    m_options.headers = newOptions.headers;
    m_options.keepAlive = newOptions.keepAlive;
    m_options.contentLength = newOptions.contentLength;

    m_options.data = newOptions.data;
    m_readProgress.reset();
    m_readProgress.size = newOptions.data.size();

    m_options.last = newOptions.last;
    m_options.userData = newOptions.userData;
    setNotEmpty( m_options.handler, newOptions.handler );
    m_options.timeoutMs = newOptions.timeoutMs;
    m_options.connectTimeoutMs = newOptions.connectTimeoutMs;
    m_options.repeatAllowed = newOptions.repeatAllowed;
}

bool Connection::resetHandle() {
    UniqueLock lock( m_mutex );
    if( !m_inMulti && getCURL() ) {
        // Using the hack from curl_wrapper:
        static std::mutex globalInitLock;

        {
            std::lock_guard< std::mutex > scopeLock( globalInitLock );

            // Curl calls EVP_cleanup in different strange cases. EVP_cleanup removes all SSL digests and curl
            // doesn't work. It's a dirty fix of this case.
            const auto sslDigestAlgorithm = EVP_get_digestbynid( NID_ecdsa_with_SHA1 );
            if( sslDigestAlgorithm == nullptr ) {
                if( verbose() )
                    LOGGER_DEBUG( cc::debug( "curl : didn't find SHA1 SSL digest. All SSL digests will be added." ) );
                OpenSSL_add_all_digests();
            }

            curl_global_init( CURL_GLOBAL_DEFAULT );
        }

        m_curl = CurlPtr{ curl_easy_init(), ::curl_easy_cleanup };
        curl_easy_setopt( m_curl.get(), CURLOPT_NOSIGNAL, 1L );

        return true;
    }
    return false;
}

CallbackProgress Connection::readProgressTotal() const {
    UniqueLock lock( m_mutex );
    return m_readProgressTotal;
}

void Connection::verbose( bool value ) {
    m_verbose = value;
}

bool Connection::verbose() const {
    return m_verbose;
}

void Connection::setLastProgressPrint( time_t timePoint ) {
    m_lastProgressPrint = timePoint;
}

time_t Connection::getLastProgressPrint() const {
    return m_lastProgressPrint;
}

void CallbackProgress::reset() {
    size = 0;
    offset = 0;
    timepoint = 0;
}

}  // namespace CommonHttp
}  // namespace HubPlatform
