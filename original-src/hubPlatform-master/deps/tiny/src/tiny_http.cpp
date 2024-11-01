#include <tiny/tiny_http.h>
#include <tiny/tiny_network.h>
#include <tiny/tiny_dispatch.h>
/*
#include <tiny/tiny_process.h>
#include <iostream>
#include <exception>
#include <functional>
#include <sstream>
*/

extern "C" {
#include <uv.h>
// notice: <tiny/http_parser/http_parser.h> header conflicts with libWebSockets`
#include <tiny/http_parser/http_parser.h>
}

#define MAX_WRITE_HANDLES 1000

#define ASSERT_STATUS( status, msg )                                               \
    if( status != 0 ) {                                                            \
        std::cerr << msg << ": " << uv_err_name( status );                         \
        throw std::runtime_error( "Tiny http error " + std::to_string( status ) ); \
    }

namespace tiny {
namespace http {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
        static const std::string requestFormat =
            "GET {} HTTP/1.0\r\n"
            "Host: {}\r\n"
            "Accept: * / *\r\n"
            "Connection: close\r\n\r\n";

        void replace_substring( std::string & str, const std::string & strWhat, const std::string & strWith, size_t
   nMaxReplacements ) { size_t index = 0, done = 0, lenWhat = strWhat.length(), lenWith = strWith.length(); while (true)
   { if( nMaxReplacements != std::string::npos && done >= nMaxReplacements ) break; index = str.find( strWhat, index );
   // locate the substring to replace if( index == std::string::npos ) break;
                 ++ done;
                 str.replace( index, lenWhat, strWith ); // make the replacement
                 index += lenWith; // advance index forward so the next iteration doesn't pick it up as well.
            }
        }
        std::string replace_substring_copy( const std::string & strWhere, const std::string & strWhat, const std::string
   & strWith, size_t nMaxReplacements ) { std::string str = strWhere; replace_substring( str, strWhat, strWith,
   nMaxReplacements ); return str;
        }
        std::string format( const std::string & strFormat, const std::string & strArg0 ) {
            return replace_substring_copy( strFormat, "{}", strArg0, 1 );
        }
        std::string format( const std::string & strFormat, const std::string & strArg0, const std::string & strArg1 ) {
            return format( format( strFormat, strArg0 ), strArg1 );
        }
        std::string format( const std::string & strFormat, const std::string & strArg0, const std::string & strArg1,
   const std::string & strArg2 ) { return format( format( strFormat, strArg0, strArg1 ), strArg2 );
        }
        std::string format( const std::string & strFormat, const std::string & strArg0, const std::string & strArg1,
   const std::string & strArg2, const std::string & strArg3 ) { return format( format( strFormat, strArg0, strArg1,
   strArg2 ), strArg3 );
        }
        std::string format( const std::string & strFormat, const std::string & strArg0, const std::string & strArg1,
   const std::string & strArg2, const std::string & strArg3, const std::string & strArg4 ) { return format( format(
   strFormat, strArg0, strArg1, strArg2, strArg3 ), strArg4 );
        }

        request::request(
                const std::string & server,
                const std::string & path,
                const std::string & service
                )
                : ioService_()
                , resolver_( ioService_ )
                , socket_( ioService_ )
        {
            std::ostream requestStream( &request_ );
            requestStream << format( requestFormat, path, server );
            asio::ip::tcp::resolver::query query( server, service );
            asio::ip::tcp::resolver::iterator endpointIterator = resolver_.resolve( query );
            onResolve( endpointIterator );
        }

        void request::onResolve( asio::ip::tcp::resolver::iterator endpointIterator ) {
            asio::connect( socket_, endpointIterator );
            onConnect();
        }

        void request::onConnect() {
            asio::write( socket_, request_ );
            onWriteRequest();
        }

        void request::onWriteRequest() {
            static const std::string delimiter( "\r\n" );
            asio::read_until( socket_, response_, delimiter );
            onReadStatusLine();
        }

        void request::onReadStatusLine() {
            static const std::string delimiter( "\r\n\r\n" );
            std::istream responseStream( &response_ );
            std::string httpVersion;
            responseStream >> httpVersion;
            unsigned int statusCode;
            responseStream >> statusCode;
            std::string statusMessage;
            std::getline( responseStream, statusMessage );
            if( (!responseStream) || httpVersion.substr( 0, 5 ) != "HTTP/" )
                throw std::std::runtime_error( "invalid HTTP server response" );
            if( statusCode != 200 ) {
                std::stringstream ss;
                ss << "response returned with status code " << statusCode;
                throw std::std::runtime_error( ss.str() );
            }
            asio::read_until( socket_, response_, "\r\n\r\n" );
            onReadHeaders();
        }

        void request::onReadHeaders() {
            std::istream responseStream( &response_ );
            std::string header;
            while( std::getline( responseStream, header ) && header != "\r" )
                headers_ += header;
            // write whatever content we already have to body.
            if( response_.size() > 0 ) {
                std::ostringstream ss;
                ss << &response_;
                body_ += ss.str();
            }
            onReadContent();
        }

        void request::onReadContent() {
            std::error_code error;
            while( asio::read( socket_, response_, asio::transfer_at_least( 1 ), error ) ) {
                std::ostringstream ss;
                ss << &response_;
                body_ += ss.str();
            }
            if( error != asio::error::eof )
                throw std::system_error( error );
        }

        const std::string & request::getHeaders() const { return headers_; }
        const std::string & request::getBody() const { return body_; }

        async_request::async_request(
                asio::io_service & ioService,
                const std::string & server,
                const std::string & service,
                const std::string & path,
                fnCompleteHandler handler
                )
                : resolver_( ioService )
                , socket_( ioService )
                , fnHandler_( handler )
        {
            std::ostream requestStream( &request_ );
            requestStream << "GET " << path << " HTTP/1.0\r\n";
            requestStream << "Host: " << server << "\r\n";
            requestStream << "Accept: * / *\r\n";
            requestStream << "Connection: close\r\n\r\n";
            asio::ip::tcp::resolver::query query( server, service );
            resolver_.async_resolve(
                        query,
                        std::bind(
                            &async_request::onResolve,
                            this,
                            std::placeholders::_1, // std::placeholders::error,
                            std::placeholders::_2  // std::placeholders::iterator
                            )
                        );
        }

        void async_request::onResolve( const std::error_code & err, asio::ip::tcp::resolver::iterator endpointIterator )
   { if( ! err ) { asio::async_connect( socket_, endpointIterator, std::bind( &async_request::onConnect, this,
                        std::placeholders::_1 // std::placeholders::error
                        )
                    );
            } else
                throw std::std::runtime_error( format( "HTTP request error: {}", err.message() ) );
        }

        void async_request::onConnect( const std::error_code & err ) {
            if( ! err ) {
                asio::async_write(
                    socket_,
                    request_,
                    std::bind(
                        &async_request::onWriteRequest,
                        this,
                        std::placeholders::_1 // std::placeholders::error
                        )
                    );
            } else
                throw std::std::runtime_error( format( "HTTP request error: {}", err.message() ) );
        }

        void async_request::onWriteRequest( const std::error_code & err ) {
            static const std::string delimiter("\r\n");
            if( ! err ) {
                asio::async_read_until(
                    socket_,
                    response_,
                    delimiter,
                    std::bind(
                        &async_request::onReadStatusLine,
                        this,
                        std::placeholders::_1 // std::placeholders::error
                        )
                    );
            } else
                throw std::std::runtime_error( format( "HTTP request error: {}", err.message() ) );
        }

        void async_request::onReadStatusLine( const std::error_code & err ) {
            if( ! err ) {
                std::istream responseStream( &response_ );
                std::string httpVersion;
                responseStream >> httpVersion;
                unsigned int statusCode;
                responseStream >> statusCode;
                std::string statusMessage;
                std::getline( responseStream, statusMessage );
                if( (!responseStream) || httpVersion.substr( 0, 5 ) != "HTTP/" ) {
                    throw std::std::runtime_error( "HTTP request error: invalid server responce" );
                    return;
                }
                if( statusCode != 200 ) {
                    //std::cerr << format( "HTTP response returned with status code {}", statusCode );
                    return;
                }
                static const std::string delimiter("\r\n\r\n");
                // read the response headers, which are terminated by a blank line.
                asio::async_read_until(
                    socket_,
                    response_,
                    delimiter,
                    std::bind(
                        &async_request::onReadHeaders,
                        this,
                        std::placeholders::_1 // std::placeholders::error
                        )
                    );
            } else
                throw std::std::runtime_error( format( "HTTP request error: {}", err.message() ) );
        }

        void async_request::onReadHeaders( const std::error_code & err ) {
            if( ! err ) {
                std::istream responseStream( &response_ );
                std::string header;
                while( std::getline( responseStream, header ) && header != "\r" )
                    headers_ += header;
                //if( response_.size() > 0 ) {
                //    std::cerr << &response_;
                //}
                asio::async_read(
                    socket_,
                    response_,
                    asio::transfer_at_least(1),
                    std::bind(
                        &async_request::onReadContent,
                        this,
                        std::placeholders::_1 // std::placeholders::error
                        )
                    );
            } else
                throw std::std::runtime_error( format( "HTTP request error: {}", err.message() ) );
        }

        void async_request::onReadContent( const std::error_code & err) {
            if( ! err ) {
                std::ostringstream ss;
                ss << &response_;
                body_ += ss.str();
                asio::async_read(
                    socket_,
                    response_,
                    asio::transfer_at_least(1),
                    std::bind(
                        &async_request::onReadContent,
                        this,
                        std::placeholders::_1 // std::placeholders::error
                        )
                    );
            } else if( err != asio::error::eof ) {
                throw std::std::runtime_error( format( "HTTP request error: {}", err.message() ) );
            } else {
                tiny::tools::replace_all( body_, "\r\n", "\n" );
                fnHandler_( this );
            }
        }

        const std::string & async_request::getHeaders() const { return headers_; }

        const std::string & async_request::getBody() const { return body_; }
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace uri {
// helper functions
namespace helper {
std::string slice_head( std::string& strSubject, const std::string& strDelimiter ) {
    // chops off the delimiter and everything that precedes(destructively)
    // returns everthing before the delimeter
    auto delimiter_location = strSubject.find( strDelimiter );
    auto delimiter_length = strDelimiter.length();
    std::string output = "";
    if( delimiter_location < std::string::npos ) {
        output = strSubject.substr( 0, delimiter_location );
        strSubject = strSubject.substr( delimiter_location + delimiter_length,
                                        strSubject.length() - ( delimiter_location + delimiter_length ) );
    }
    return output;
}
std::string slice_tail( std::string& strSubject,
                        const std::string& strDelimiter,
                        bool bKeepDelim  // = false
) {
    // chops off the delimiter and everything that follows(destructively)
    // returns everything after the delimiter
    auto delimiter_location = strSubject.find( strDelimiter );
    auto delimiter_length = strDelimiter.length();
    std::string output = "";
    if( delimiter_location < std::string::npos ) {
        auto start = bKeepDelim ? delimiter_location : delimiter_location + delimiter_length;
        auto end = strSubject.length() - start;
        output = strSubject.substr( start, end );
        strSubject = strSubject.substr( 0, delimiter_location );
    }
    return output;
}
tiny::http::uri::split_t split( const std::string& strInput,
                                const std::string& strSeparators,
                                bool bRemoveEmpty  // = true
) {
    tiny::http::uri::split_t list;
    std::ostringstream word;
    for( size_t n = 0; n < strInput.size(); ++n ) {
        if( std::string::npos == strSeparators.find( strInput[ n ] ) )
            word << strInput[ n ];
        else {
            if( ( !word.str().empty() ) || ( !bRemoveEmpty ) )
                list.push_back( word.str() );
            word.str( "" );
        }
    }
    if( ( !word.str().empty() ) || ( !bRemoveEmpty ) )
        list.push_back( word.str() );
    return list;
}
};  // namespace helper

// extractors
namespace extract {
int port( std::string& strHostPort ) {
    int port;
    std::string portstring = tiny::http::uri::helper::slice_tail( strHostPort, ":" );
    try {
        port = atoi( portstring.c_str() );
    }
    catch( std::exception& ) {
        port = -1;
    }
    return port;
}

uri::qsmap query( const std::string& strQuery ) {
    uri::qsmap query;
    auto pairs = tiny::http::uri::helper::split( strQuery, "&" );
    for( auto& p : pairs ) {
        auto pair = tiny::http::uri::helper::split( p, "=" );
        if( pair.size() == 2 )
            query.insert( { pair[ 0 ], pair[ 1 ] } );
    }
    return query;
}
};  // namespace extract

tiny::http::uri::url parse( std::string& strInput ) {
    tiny::http::uri::url ret;
    ret.port_ = -1;
    ret.search_ = tiny::http::uri::extract::search( strInput );
    ret.protocol_ = tiny::http::uri::extract::protocol( strInput );
    ret.path_ = tiny::http::uri::extract::path( strInput );
    std::string userpass = tiny::http::uri::extract::userpass( strInput );
    ret.password_ = tiny::http::uri::extract::password( userpass );
    ret.user_ = userpass;
    ret.port_ = tiny::http::uri::extract::port( strInput );
    ret.host_ = strInput;
    ret.query_ = tiny::http::uri::extract::query( ret.search_ );
    return ret;
}

static const char HEX2DEC[ 256 ] = {
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 1 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 2 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 3 */ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1,

    /* 4 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 5 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 6 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 7 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    /* 8 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 9 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* A */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* B */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    /* C */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* D */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* E */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* F */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

std::string uri_decode( const std::string& strSrc ) {
    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"
    const unsigned char* pSrc = (const unsigned char*) strSrc.c_str();
    const int SRC_LEN = strSrc.length();
    const unsigned char* const SRC_END = pSrc + SRC_LEN;
    const unsigned char* const SRC_LAST_DEC = SRC_END - 2;  // last decodable '%'
    char* const pStart = new char[ SRC_LEN ];
    char* pEnd = pStart;
    while( pSrc < SRC_LAST_DEC ) {
        if( *pSrc == '%' ) {
            char dec1, dec2;
            if( -1 != ( dec1 = HEX2DEC[ *( pSrc + 1 ) ] ) && -1 != ( dec2 = HEX2DEC[ *( pSrc + 2 ) ] ) ) {
                *pEnd++ = ( dec1 << 4 ) + dec2;
                pSrc += 3;
                continue;
            }
        }
        *pEnd++ = *pSrc++;
    }
    // the last 2- chars
    while( pSrc < SRC_END )
        *pEnd++ = *pSrc++;
    std::string sResult( pStart, pEnd );
    delete[] pStart;
    return sResult;
}

// only alphanum is safe
static const char SAFE[ 256 ] = {
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 1 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 2 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

    /* 4 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    /* 6 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

    /* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    /* C */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* D */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* E */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* F */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

std::string uri_encode( const std::string& strSrc ) {
    const char DEC2HEX[ 16 + 1 ] = "0123456789ABCDEF";
    const unsigned char* pSrc = (const unsigned char*) strSrc.c_str();
    const int SRC_LEN = strSrc.length();
    unsigned char* const pStart = new unsigned char[ SRC_LEN * 3 ];
    unsigned char* pEnd = pStart;
    const unsigned char* const SRC_END = pSrc + SRC_LEN;
    for( ; pSrc < SRC_END; ++pSrc ) {
        if( SAFE[ *pSrc ] )
            *pEnd++ = *pSrc;
        else {  // escape this char
            *pEnd++ = '%';
            *pEnd++ = DEC2HEX[ *pSrc >> 4 ];
            *pEnd++ = DEC2HEX[ *pSrc & 0x0F ];
        }
    }
    std::string sResult( (char*) pStart, (char*) pEnd );
    delete[] pStart;
    return sResult;
}
};  // namespace uri

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string CRLF = "\r\n";

void free_uv_handle( uv_handle_t* handle ) {
    auto* ctx = reinterpret_cast< context* >( handle->data );
    ctx->writes_.clear();
    free( ctx );
}

// events
template < typename T >
void attach_http_parser_event_handlers( T* instance, http_parser_settings& http_ps ) {
    http_ps.on_url =
        []( http_parser* parser, const char* at, size_t len ) -> int {  // called after the url has been parsed
        context* ctx = static_cast< context* >( parser->data );
        if( at && ctx )
            ctx->url_ = std::string( at, len );
        return 0;
    };
    http_ps.on_header_field = []( http_parser* parser, const char* at, size_t length )
        -> int {  // called when there are either fields or values in the request
        return 0;
    };
    http_ps.on_header_value =
        []( http_parser* parser, const char* at, size_t length ) -> int {  // called when header value is given
        return 0;
    };
    http_ps.on_headers_complete =
        []( http_parser* parser ) -> int {  // called once all fields and values have been parsed
        context* ctx = static_cast< context* >( parser->data );
        ctx->method_ = std::string( http_method_str( (enum http_method) parser->method ) );
        return 0;
    };
    http_ps.on_body =
        []( http_parser* parser, const char* at, size_t len ) -> int {  // called when there is a body for the request
        context* ctx = static_cast< context* >( parser->data );
        if( at && ctx && (int) len > -1 )
            ctx->body_ << std::string( at, len );
        return 0;
    };
    http_ps.on_message_complete = []( http_parser* parser ) -> int {  // called after all other events
        // return on_message_complete( parser );
        context* ctx = static_cast< context* >( parser->data );
        T* p_instance = (T*) ctx->owner_;
        auto callback = p_instance->listener_;
        return p_instance->complete( parser, callback );
    };
}

template void attach_http_parser_event_handlers< client >( client* instance, http_parser_settings& http_ps );
template void attach_http_parser_event_handlers< server >( server* instance, http_parser_settings& http_ps );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

request::request() {}
request::~request() {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

response::response()
: stream_interface()
, std::ostream( &response_string_buffer_ )
// , response_string_buffer_( stream_ )
{
    response_string_buffer_.stream_ = this;
    parser_.reset( new http_parser );
}
response::~response() {
    parser_ = nullptr;
}

// response
void response::setHeader( const std::string& key, const std::string& val ) {
    headersSet_ = true;
    if( writtenOrEnded_ )
        throw std::runtime_error( "Can not set headers after write" );
    if( key == "Content-Length" )
        contentLengthSet_ = true;
    headers_.insert( { key, val } );
}

void response::setStatus( int code ) {
    statusSet_ = true;
    if( writtenOrEnded_ )
        throw std::runtime_error( "Can not set status after write" );
    statusCode_ = code;
}

void response::setStatus( int code, const std::string& ad ) {
    statusSet_ = true;
    if( writtenOrEnded_ )
        throw std::runtime_error( "Can not set status after write" );
    statusCode_ = code;
    statusAdjective_ = ad;
}

void response::writeOrEnd( const std::string& a_str, bool bEnd ) {
    std::string str = a_str;
    if( ended_ )
        throw std::runtime_error( "Can not write after end" );
    std::stringstream ss;
    if( !writtenOrEnded_ ) {
        ss << "HTTP/1.1 " << statusCode_ << " " << statusAdjective_ << CRLF;
        for( auto& header : headers_ ) {
            ss << header.first << ": " << header.second << CRLF;
        }
        ss << CRLF;
        writtenOrEnded_ = true;
    }
    bool isChunked = headers_.count( "Transfer-Encoding" ) && headers_[ "Transfer-Encoding" ] == "chunked";
    if( isChunked )
        ss << std::hex << str.size() << std::dec << CRLF << str << CRLF;
    else
        ss << str;
    if( isChunked && bEnd )
        ss << "0" << CRLF << CRLF;
    str = ss.str();
    // response buffer
    uv_buf_t uvb_response = { .base = (char*) str.c_str(), .len = str.size() };
    context* ctx = static_cast< context* >( this->parser_->data );
    auto id = write_count_++;
    uv_write_t write_req;
    ctx->writes_.insert( { id, write_req } );
    if( bEnd ) {
        ended_ = true;
        uv_write(
            &ctx->writes_.at( id ), (uv_stream_t*) &ctx->handle_, &uvb_response, 1, []( uv_write_t* req, int status ) {
                if( !uv_is_closing( (uv_handle_t*) req->handle ) )
                    uv_close( (uv_handle_t*) req->handle, free_uv_handle );
            } );
    }
    else
        uv_write( &ctx->writes_.at( id ), (uv_stream_t*) &ctx->handle_, &uvb_response, 1, nullptr );
}

void response::write( const std::string& s ) {
    this->writeOrEnd( s, false );
}

void response::end( const std::string& s ) {
    this->writeOrEnd( s, true );
}

void response::end() {
    this->writeOrEnd( "", true );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

context::context( void* owner )
: owner_( owner ) {
    parser_.reset( new http_parser );
    memset( parser_.get(), 0, sizeof( http_parser ) );
    parser_->data = (void*) this;
    //
    memset( &handle_, 0, sizeof( uv_tcp_t ) );
    handle_.data = (void*) this;
    //
    memset( &connect_req_, 0, sizeof( uv_connect_t ) );
    connect_req_.data = (void*) this;
    //
    memset( &write_req_, 0, sizeof( uv_write_t ) );
    write_req_.data = (void*) this;
}
context::~context() {
    parser_ = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

client::client( const client::options& opts, fn_listener_t fn )
: listener_( fn ) {
    http_ps_.reset( new http_parser_settings );
    memset( http_ps_.get(), 0, sizeof( http_parser_settings ) );
    // http_ps_->data = (void*)this;
    //
    opts_ = opts;
    listener_ = fn;
    connect();
}
client::client( const std::string& strUrl,
                fn_listener_t fn,
                int ipVersion,
                uv_loop_t* loop  // = nullptr // NULL means used dispatch default loop
                )
: listener_( fn ) {
    http_ps_.reset( new http_parser_settings );
    memset( http_ps_.get(), 0, sizeof( http_parser_settings ) );
    // http_ps_->data = (void*)this;
    //
    std::string strUrl2( strUrl );
    auto u = tiny::http::uri::parse( strUrl2 );
    opts_.host_ = u.host_;
    opts_.ipVersion_ = ipVersion;
    opts_.loop_ = loop;
    connect();
}
client::~client() {
    http_ps_ = nullptr;
}
uv_loop_s* client::get_loop() {
    if( opts_.loop_ )
        return opts_.loop_;
    return tiny::dispatch::default_domain()->get_loop()->internal_handle();
}

int client::complete( http_parser* parser, fn_listener_t cb ) {
    context* ctx = reinterpret_cast< context* >( parser->data );
    response res;
    res.body_ = ctx->body_.str();
    ( *( res.parser_.get() ) ) = ( *parser );  // copy
    cb( res );
    return 0;
}

void client::on_read_cb( uv_stream_t* uvs, ssize_t nread, const uv_buf_t* uvb ) {
    context* ctx = static_cast< context* >( uvs->data );
    if( nread >= 0 ) {
        auto parsed = (ssize_t) http_parser_execute( ctx->parser_.get(), http_ps_.get(), uvb->base, nread );
        if( parsed < nread )
            uv_close( (uv_handle_t*) &ctx->handle_, free_uv_handle );
        if( parsed != nread ) {
            // TODO
            // Error Callback
        }
    }
    else {
        if( nread != UV_EOF )
            return;  // maybe do something interesting here...
        uv_close( (uv_handle_t*) &ctx->handle_, free_uv_handle );
    }
    free( uvb->base );
}

void client::on_connect_cb( uv_connect_t* req, int status ) {
    context* ctx = reinterpret_cast< context* >( req->handle->data );
    attach_http_parser_event_handlers( this, *( http_ps_.get() ) );
    if( status == -1 ) {
        // TO-DO: error callback
        uv_close( (uv_handle_t*) req->handle, free_uv_handle );
        return;
    }
    uv_buf_t uvb_request;
    std::string reqstr = opts_.method_ + " " + opts_.url_ + " HTTP/1.1" + CRLF +
                         //
                         // TO-DO: add user's headers here
                         //
                         "Connection: keep-alive" + CRLF + CRLF;
    uvb_request.base = (char*) reqstr.c_str();
    uvb_request.len = reqstr.size();
    uv_read_start( req->handle,
                   []( uv_handle_t* handle, size_t suggested_size, uv_buf_t* uvb ) {
                       ( *uvb ) = uv_buf_init( (char*) malloc( suggested_size ), suggested_size );
                   },
                   []( uv_stream_t* uvs, ssize_t nread, const uv_buf_t* uvb ) {
                       context* ctx = static_cast< context* >( uvs->data );
                       client* p_instance = (client*) ctx->owner_;
                       p_instance->on_read_cb( uvs, nread, uvb );
                   } );
    uv_write( &ctx->write_req_, req->handle, &uvb_request, 1, nullptr );
}

void client::on_resolved_cb( uv_getaddrinfo_t* req, int status, struct addrinfo* res ) {
    char strAddrBuffer[ 512 ];
    memset( strAddrBuffer, 0, sizeof( strAddrBuffer ) );
    if( opts_.ipVersion_ == 6 )
        uv_ip6_name( (struct sockaddr_in6*) res->ai_addr, strAddrBuffer, sizeof( strAddrBuffer ) - 1 );
    else
        uv_ip4_name( (struct sockaddr_in*) res->ai_addr, strAddrBuffer, sizeof( strAddrBuffer ) - 1 );
    uv_freeaddrinfo( res );
    tiny::network::sockaddr46 dest;
    if( opts_.ipVersion_ == 6 )
        uv_ip6_addr( strAddrBuffer, 8000, &dest.sa6 );
    else
        uv_ip4_addr( strAddrBuffer, 8000, &dest.sa4 );
    context* ctx = new context( (void*) this );
    http_parser_init( ctx->parser_.get(), HTTP_RESPONSE );
    ctx->parser_->data = (void*) ctx;
    uv_tcp_init( get_loop(), &ctx->handle_ );
    ctx->handle_.data = (void*) this;
    // uv_tcp_keepalive( &ctx->handle, 1, 60 );
    uv_tcp_connect(
        &ctx->connect_req_, &ctx->handle_, (const struct sockaddr*) &dest, []( uv_connect_t* req, int status ) {
            context* ctx = static_cast< context* >( req->data );
            client* p_instance = (client*) ctx->owner_;
            p_instance->on_connect_cb( req, status );
        } );
}

void client::connect() {
    struct addrinfo ai;
    ai.ai_family = ( opts_.ipVersion_ == 6 ) ? PF_INET6 : PF_INET;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = IPPROTO_TCP;
    ai.ai_flags = 0;
    auto cb = []( uv_getaddrinfo_t* req, int status, struct addrinfo* res ) {
        client* p_instance = static_cast< client* >( req->data );
        p_instance->on_resolved_cb( req, status, res );
    };
    addr_req_.data = (void*) this;
    uv_getaddrinfo( get_loop(), &addr_req_, cb, "localhost", "8000", &ai );
    // uv_run( get_loop(), UV_RUN_DEFAULT );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

server::server( const server::options& opts, fn_listener_t fn, fn_on_chunk_t on_chunk )
: listener_( std::move( fn ) )
, on_chunk_( std::move( on_chunk ) ) {
    http_ps_.reset( new http_parser_settings );
    memset( http_ps_.get(), 0, sizeof( http_parser_settings ) );
    // http_ps_->data = (void*)this;
    //
    opts_ = opts;
}
server::~server() {
    http_ps_ = nullptr;
}
uv_loop_s* server::get_loop() {
    if( opts_.loop_ )
        return opts_.loop_;
    return tiny::dispatch::default_domain()->get_loop()->internal_handle();
}

int server::complete( http_parser* parser, fn_listener_t cb ) {
    context* ctx = reinterpret_cast< context* >( parser->data );
    request req;
    response res;
    req.url_ = ctx->url_;
    req.method_ = ctx->method_;
    ( *( res.parser_.get() ) ) = ( *parser );
    cb( req, res );
    return 0;
}

void server::on_read_cb( uv_stream_t* uvs, ssize_t nread, const uv_buf_t* uvb ) {  // called for every read
    ssize_t parsed;
    context* ctx = static_cast< context* >( uvs->data );
    if( nread >= 0 ) {
        parsed = (ssize_t) http_parser_execute( ctx->parser_.get(), http_ps_.get(), uvb->base, nread );
        if( parsed < nread ) {  // close handle
            uv_close( (uv_handle_t*) &ctx->handle_, free_uv_handle );
        }
    }
    else {
        if( nread != UV_EOF ) {
            // TO-DO: debug error
        }
        // close handle
        uv_close( (uv_handle_t*) &ctx->handle_, free_uv_handle );
    }
    // free request buffer data
    free( uvb->base );
}

void server::on_connect_cb( uv_stream_t* uvs, int status ) {  // called once a connection is made
    context* ctx = new context( (void*) this );
    uv_tcp_init( get_loop(), &ctx->handle_ );  // init tcp handle
    ctx->handle_.data = (void*) ctx;
    http_parser_init( ctx->parser_.get(), HTTP_REQUEST );  // init http parser
    ctx->parser_->data = ctx;                              // client reference for parser routines
    // accept connection passing in refernce to the client handle
    uv_accept( uvs, (uv_stream_t*) &ctx->handle_ );
    // allocate memory and attempt to read.
    uv_read_start( (uv_stream_t*) &ctx->handle_,
                   // allocator
                   []( uv_handle_t* handle, size_t suggested_size, uv_buf_t* uvb ) {
                       ( *uvb ) = uv_buf_init( (char*) malloc( suggested_size ), suggested_size );
                   },
                   // reader
                   []( uv_stream_t* uvs, ssize_t nread, const uv_buf_t* uvb ) {
                       context* ctx = static_cast< context* >( uvs->data );
                       server* p_instance = (server*) ctx->owner_;
                       p_instance->on_read_cb( uvs, nread, uvb );
                   } );
}

int server::listen( const char* strIP, int port ) {
    attach_http_parser_event_handlers( this, *( http_ps_.get() ) );

    if( on_chunk_ ) {
        http_ps_->on_chunk_complete = []( http_parser* parser ) -> int {
            context* ctx = static_cast< context* >( parser->data );
            server* p_instance = (server*) ctx->owner_;
            p_instance->on_chunk_( *ctx );
            return 0;
        };
    }

    int status = 0;
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    int cores = sysinfo.dwNumberOfProcessors;
#else
    int cores = sysconf( _SC_NPROCESSORS_ONLN );
#endif
    std::stringstream cores_string;
    cores_string << cores;

#ifdef _WIN32
    SetEnvironmentVariable( "UV_THREADPOOL_SIZE", cores_string );
#else
    setenv( "UV_THREADPOOL_SIZE", cores_string.str().c_str(), 1 );
#endif
    tiny::network::sockaddr46 address;
    uv_tcp_init( get_loop(), &socket_ );
    socket_.data = (void*) this;
    // TO-DO: uv_tcp_keepalive( &socket_, 1, 60 );
    if( opts_.ipVersion_ == 6 )
        status = uv_ip6_addr( strIP, port, &address.sa6 );
    else
        status = uv_ip4_addr( strIP, port, &address.sa4 );
    ASSERT_STATUS( status, "Resolve Address" );

    status = uv_tcp_bind( &socket_, (const struct sockaddr*) &address, 0 );
    ASSERT_STATUS( status, "Bind" );

    status = uv_listen( (uv_stream_t*) &socket_,
                        MAX_WRITE_HANDLES,
                        // listener
                        []( uv_stream_t* uvs, int status ) {
                            server* p_instance = static_cast< server* >( uvs->data );
                            p_instance->on_connect_cb( uvs, status );
                        } );
    ASSERT_STATUS( status, "Listen" );
    // uv_run( UV_LOOP, UV_RUN_DEFAULT );
    return 0;
}

};  // namespace http
};  // namespace tiny

/*
TO-DO: support SSL
https://gist.github.com/roxlu/3999827
https://github.com/deleisha/libuv-tls
https://github.com/indutny/uv_link_t
https://github.com/indutny/uv_ssl_t
https://github.com/indutny/uv_http_t
*/
