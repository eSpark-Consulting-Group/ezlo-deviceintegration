#include "http_file_server.h"
#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {

using namespace tiny::http;

HttpFileServer::HttpFileServer()
: m_loop( (uv_loop_t*) malloc( sizeof( uv_loop_t ) ) ) {
    uv_loop_init( m_loop );
    server::options opt;
    opt.loop_ = m_loop;
    m_server =
        new server( opt,
                    std::bind( &HttpFileServer::onRequestComplete, this, std::placeholders::_1, std::placeholders::_2 ),
                    std::bind( &HttpFileServer::onChunkComplete, this, std::placeholders::_1 ) );
}

HttpFileServer::~HttpFileServer() {
    delete m_server;

    uv_stop( m_loop );
    uv_loop_close( m_loop );
    free( m_loop );
}

int HttpFileServer::start( const std::string& addr, int port ) {
    m_server->listen( addr.c_str(), port );
    return uv_run( m_loop, UV_RUN_DEFAULT );
}

void HttpFileServer::onRequestComplete( request& req, response& res ) {
    if( !req.url_.empty() && req.method_ == "POST" ) {
        if( req.url_[ 0 ] == '/' ) {
            std::string fileName = std::string( req.url_.begin() + 1, req.url_.end() );
            auto it = m_outputs.find( fileName );
            if( it != m_outputs.end() ) {
                Log::info( cc::info( "[HttpFileServer] Closing file" ) );
                m_outputs.erase( it );
            }
        }
    }
    res.setStatus( 200 );
    res.end();
}

void HttpFileServer::onChunkComplete( request& req ) {
    if( req.url_.empty() ) {
        Log::warn( cc::warn( "[HttpFileServer] Ignoring empty url" ) );
        return;
    }
    if( req.url_[ 0 ] == '/' ) {
        std::string fileName = std::string( req.url_.begin() + 1, req.url_.end() );
        auto it = m_outputs.find( fileName );
        MemoryFileSender* sender = nullptr;
        if( it == m_outputs.end() ) {
            auto i = m_outputs.emplace(
                std::piecewise_construct, std::forward_as_tuple( fileName ), std::forward_as_tuple( fileName ) );
            if( i.second ) {
                sender = &i.first->second;
            }
        }
        else {
            sender = &it->second;
        }
        if( sender ) {
            std::string bodyStr = req.body_.str();
            Log::trace( "[HttpFileServer] Received chunk, size: " + std::to_string( bodyStr.size() ) );
            if( !bodyStr.empty() ) {
                sender->sendData( reinterpret_cast< uint8_t* >( &bodyStr[ 0 ] ), bodyStr.size() );
            }
        }
    }

    // Clear the body, because it accumulates chunk data.
    req.body_.str( std::string() );
    req.body_.clear();
}
}  // namespace HubPlatform
