#if( !defined __TINY_HTTP_H )
#define __TINY_HTTP_H 1
/*
#include <string>
#include <functional>
#include <tiny/tiny_asio.h>
*/

/// l_sergiy: HTTP server and client based on:
/// https://github.com/hxoht/nodeuv-http
/// https://github.com/hxoht/nodeuv-uri
/// https://github.com/hxoht/route
/// https://github.com/nodejs/http-parser

#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <functional>
#include <memory>
#include <regex>
#include <stdlib.h>

#if( !defined _WIN32 )
#include <unistd.h>
#endif

extern "C" {
#include <uv.h>
//
// notice: <tiny/http_parser/http_parser.h> header conflicts with libWebSockets`
struct http_parser_settings;
struct http_parser;
}

namespace tiny {
namespace http {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
        void replace_substring( std::string & str, const std::string & strWhat, const std::string & strWith, size_t
   nMaxReplacements = std::string::npos ); std::string replace_substring_copy( const std::string & strWhere, const
   std::string & strWhat, const std::string & strWith, size_t nMaxReplacements = std::string::npos ); std::string
   format( const std::string & strFormat, const std::string & strArg0 ); std::string format( const std::string &
   strFormat, const std::string & strArg0, const std::string & strArg1 ); std::string format( const std::string &
   strFormat, const std::string & strArg0, const std::string & strArg1, const std::string & strArg2 ); std::string
   format( const std::string & strFormat, const std::string & strArg0, const std::string & strArg1, const std::string &
   strArg2, const std::string & strArg3 ); std::string format( const std::string & strFormat, const std::string &
   strArg0, const std::string & strArg1, const std::string & strArg2, const std::string & strArg3, const std::string &
   strArg4 );

        class request {
            asio::io_service ioService_;
            asio::ip::tcp::resolver resolver_;
            asio::ip::tcp::socket socket_;
            asio::streambuf request_;
            asio::streambuf response_;
            std::string headers_;
            std::string body_;
        public:
            request(
                const std::string & server,
                const std::string & path,
                const std::string & service = std::string("http")
                );
            const std::string & getHeaders() const;
            const std::string & getBody() const;
        private:
            void onResolve( asio::ip::tcp::resolver::iterator endpointIterator );
            void onConnect();
            void onWriteRequest();
            void onReadStatusLine();
            void onReadHeaders();
            void onReadContent();
        }; /// class request

        class async_request {
            asio::ip::tcp::resolver resolver_;
            asio::ip::tcp::socket socket_;
            asio::streambuf request_;
            asio::streambuf response_;
        public:
            typedef std::function<void(async_request * sender)> fnCompleteHandler;
        private:
            fnCompleteHandler fnHandler_;
            std::string headers_;
            std::string body_;
        public:
            async_request(
                asio::io_service & ioService,
                const std::string & server,
                const std::string & service,
                const std::string & path,
                fnCompleteHandler handler
                );
            const std::string & getHeaders() const;
            const std::string & getBody() const;
        private:
            void onResolve( const std::error_code & err, asio::ip::tcp::resolver::iterator endpointIterator );
            void onConnect( const std::error_code & err );
            void onWriteRequest( const std::error_code & err );
            void onReadStatusLine( const std::error_code & err );
            void onReadHeaders( const std::error_code & err );
            void onReadContent( const std::error_code & err );
        private:
        }; /// class async_request
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace uri {
typedef std::map< std::string, std::string > qsmap;
typedef std::vector< std::string > split_t;
struct url {
    std::string protocol_;
    std::string user_;
    std::string password_;
    std::string host_;
    std::string path_;
    std::string search_;
    qsmap query_;
    int port_ = -1;  // not parsed
};                   /// struct url

// helper functions
namespace helper {
extern std::string slice_head( std::string& strSubject, const std::string& strDelimiter );
extern std::string slice_tail( std::string& strSubject, const std::string& strDelimiter, bool bKeepDelim = false );
extern tiny::http::uri::split_t split( const std::string& strInput,
                                       const std::string& strSeparators,
                                       bool bRemoveEmpty = true );
};  // namespace helper
// extractors
namespace extract {
extern int port( std::string& strHostPort );
extern uri::qsmap query( const std::string& strQuery );
inline std::string path( std::string& strInput ) {
    return tiny::http::uri::helper::slice_tail( strInput, "/", true );
}
inline std::string protocol( std::string& strInput ) {
    return tiny::http::uri::helper::slice_head( strInput, "://" );
}
inline std::string search( std::string& strInput ) {
    return tiny::http::uri::helper::slice_tail( strInput, "?" );
}
inline std::string password( std::string& strUserPass ) {
    return tiny::http::uri::helper::slice_tail( strUserPass, ":" );
}
inline std::string userpass( std::string& strInput ) {
    return tiny::http::uri::helper::slice_head( strInput, "@" );
}
};  // namespace extract
// public interface
extern url parse( std::string& in );
extern std::string uri_decode( const std::string& strSrc );
extern std::string uri_encode( const std::string& strSrc );
};  // namespace uri

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename T >
class stream_string_buffer;
template < typename T >
class stream_interface;
class request;
class response;
class server;
class client;
class context;

extern const std::string CRLF;
extern void free_uv_handle( uv_handle_t* );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename T >
class stream_string_buffer : public std::stringbuf {
public:
    T* stream_ = nullptr;
    stream_string_buffer() {}
    // stream_string_buffer < T > ( std::ostream & str ) { }
    virtual ~stream_string_buffer() {}
    virtual int sync() {
        std::string strOut = str();
        std::ostringstream oss;
        oss << strOut;
        strOut = oss.str();
        stream_->writeOrEnd( strOut, true );
        oss.flush();
        str( "" );
        return 0;
    }
    friend class request;
    friend class response;
};  /// template class stream_string_buffer

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template < typename T >
class stream_interface : virtual public std::ostream {
public:
    stream_interface() {}
    virtual ~stream_interface() {}
};  /// template class stream_interface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class request {
public:
    std::string url_;
    std::string method_;
    std::string status_code_;
    std::stringstream body_;
    std::map< const std::string, const std::string > headers;
    request();
    virtual ~request();
};  /// class request

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class response : public stream_interface< response > {
    std::stringstream stream_;
    stream_string_buffer< response > response_string_buffer_;
    int write_count_ = 0;
    bool writtenOrEnded_ = false;
    bool ended_ = false;
    bool headersSet_ = false;
    bool statusSet_ = false;
    bool contentLengthSet_ = false;

public:
    std::shared_ptr< http_parser > parser_;
    int statusCode_ = 200;
    std::string body_ = "";
    std::string statusAdjective_ = "OK";
    std::map< const std::string, const std::string > headers_;
    response();
    virtual ~response();
    void setHeader( const std::string& key, const std::string& val );
    void setStatus( int code );
    void setStatus( int code, const std::string& ad );
    void write( const std::string& s );
    void end( const std::string& s );
    void end();

private:
    void writeOrEnd( const std::string& a_str, bool bEnd );
    friend class stream_string_buffer< class response >;
    friend class server;
};  /// class response

// TO-DO: maybe, have each op call write
//	inline response & operator << ( response & res, std::string s ) {
//		res.write( s );
//		return res;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class context : public request {
public:
    std::map< int, uv_write_t > writes_;
    uv_tcp_t handle_;
    uv_connect_t connect_req_;
    uv_write_t write_req_;
    std::shared_ptr< http_parser > parser_;
    void* owner_ = nullptr;
    context( void* owner );
    virtual ~context();
};  /// class context

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class client {
public:
    typedef std::function< void( response& res ) > fn_listener_t;

protected:
    fn_listener_t listener_;
    uv_tcp_t socket_;
    std::shared_ptr< http_parser_settings > http_ps_;

protected:
    uv_getaddrinfo_t addr_req_;

public:
    struct options {
        int ipVersion_ = 4;          // 4 or 6
        uv_loop_t* loop_ = nullptr;  // NULL means used dispatch default loop
        std::string host_;
        int port_ = -1;  // unknown or not parsed
        std::string method_ = "PUT";
        std::string url_ = "/";
    };  /// struct Options
    options opts_;
    client( const options& opts, fn_listener_t listener );
    client( const std::string& strUrl,
            fn_listener_t listener,
            int ipVersion,
            uv_loop_t* loop = nullptr  // NULL means used dispatch default loop
    );
    virtual ~client();
    uv_loop_t* get_loop();

private:
    void connect();
    int complete( http_parser* parser, fn_listener_t fn );
    void on_connect_cb( uv_connect_t* req, int status );
    void on_read_cb( uv_stream_t* uvs, ssize_t nread, const uv_buf_t* uvb );
    void on_resolved_cb( uv_getaddrinfo_t* req, int status, struct addrinfo* res );
    template < typename T >
    friend void attach_http_parser_event_handlers( T* instance, http_parser_settings& settings );
    friend class response;
};  /// class client

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class server {
public:
    typedef std::function< void( request& req, response& res ) > fn_listener_t;
    typedef std::function< void( request& req ) > fn_on_chunk_t;

private:
    fn_listener_t listener_;
    fn_on_chunk_t on_chunk_;
    uv_tcp_t socket_;
    std::shared_ptr< http_parser_settings > http_ps_;

public:
    struct options {
        int ipVersion_ = 4;          // 4 or 6
        uv_loop_t* loop_ = nullptr;  // NULL means used dispatch default loop
    };                               /// struct Options
    options opts_;
    server( const options& opts, fn_listener_t listener, fn_on_chunk_t on_chunk = {} );
    virtual ~server();
    uv_loop_t* get_loop();
    int listen( const char* strIP, int port );

private:
    void on_connect_cb( uv_stream_t* uvs, int status );
    void on_read_cb( uv_stream_t* uvs, ssize_t nread, const uv_buf_t* uvb );
    int complete( http_parser* parser, fn_listener_t fn );
    template < typename T >
    friend void attach_http_parser_event_handlers( T* instance, http_parser_settings& settings );
    friend class response;
};  /// class server

};  // namespace http
};  // namespace tiny

#endif  /// (! defined __TINY_HTTP_H)
