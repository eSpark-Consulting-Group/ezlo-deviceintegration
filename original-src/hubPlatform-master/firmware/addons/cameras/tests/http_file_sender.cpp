#include "http_file_sender.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>
#include <cstring>

namespace HubPlatform {

size_t HttpFileSender::readCallback( char* buffer, size_t size, size_t nitems, void* userdata ) {
    auto* sender = reinterpret_cast< HttpFileSender* >( userdata );
    size_t sizeToSend = std::min( size * nitems, sender->m_memoryBuffer.size );
    Log::trace( "readCallback(), size: " + std::to_string( ( sizeToSend ) ) );
    if( sizeToSend ) {
        std::memcpy( buffer, sender->m_memoryBuffer.data, sizeToSend );
        sender->m_memoryBuffer.size -= sizeToSend;
        sender->m_memoryBuffer.data += sizeToSend;
    }
    return sizeToSend;
}

HttpFileSender::HttpFileSender( const std::string& url )
: m_curl( curl_easy_init() )
, m_multiCurl( curl_multi_init() )
, m_chunk( nullptr ) {
    curl_easy_setopt( m_curl, CURLOPT_POST, 1L );
    curl_easy_setopt( m_curl, CURLOPT_URL, url.c_str() );
    curl_easy_setopt( m_curl, CURLOPT_NOSIGNAL, 1 );
    curl_easy_setopt( m_curl, CURLOPT_READFUNCTION, readCallback );
    curl_easy_setopt( m_curl, CURLOPT_READDATA, this );

    m_chunk = curl_slist_append( m_chunk, "Transfer-Encoding: chunked" );
    m_chunk = curl_slist_append( m_chunk, "Expect:" );
    curl_easy_setopt( m_curl, CURLOPT_HTTPHEADER, m_chunk );

#ifdef _DEBUG
    curl_easy_setopt( m_curl, CURLOPT_VERBOSE, 1L );
#endif

    curl_multi_add_handle( m_multiCurl, m_curl );
}

HttpFileSender::~HttpFileSender() {
    curl_multi_cleanup( m_multiCurl );
    curl_easy_cleanup( m_curl );
    curl_slist_free_all( m_chunk );
}

bool HttpFileSender::finalizeFile() {
    // Zero chunk indicates the end of transfer.
    return sendData( nullptr, 0 );
}

bool HttpFileSender::sendData( uint8_t* data, size_t size ) {
    m_memoryBuffer.data = data;
    m_memoryBuffer.size = size;

    int runningCount = 0;
    curl_multi_perform( m_multiCurl, &runningCount );
    Log::trace( "curl_multi_perform(), " + std::to_string( runningCount ) + ", size: " + std::to_string( size ) );

    while( m_memoryBuffer.size && runningCount ) {
        struct timeval timeout = { 0, 0 };
        /* set a suitable timeout to play around with */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        long curlTimeo = -1;
        curl_multi_timeout( m_multiCurl, &curlTimeo );
        if( curlTimeo >= 0 ) {
            timeout.tv_sec = curlTimeo / 1000;
            if( timeout.tv_sec > 1 )
                timeout.tv_sec = 1;
            else
                timeout.tv_usec = ( curlTimeo % 1000 ) * 1000;
        }

        int maxfd = -1;
        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        FD_ZERO( &fdread );
        FD_ZERO( &fdwrite );
        FD_ZERO( &fdexcep );
        CURLMcode mc = curl_multi_fdset( m_multiCurl, &fdread, &fdwrite, &fdexcep, &maxfd );
        if( mc != CURLM_OK ) {
            Log::error( cc::error( "curl_multi_fdset() failed, code " + std::to_string( mc ) ) );
            return false;
        }

        int rc;
        if( maxfd == -1 ) {
            struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
            rc = select( 0, nullptr, nullptr, nullptr, &wait );
        }
        else {
            rc = select( maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout );
        }

        switch( rc ) {
        case -1:
            Log::warn( cc::warn( "select() failed (-1)" ) );
            return false;
        case 0:  /* timeout */
        default: /* action */
            curl_multi_perform( m_multiCurl, &runningCount );
            Log::trace( "curl_multi_perform(), " + std::to_string( runningCount ) +
                        ", size left: " + std::to_string( m_memoryBuffer.size ) );
            break;
        }
    }

    int messagesLeft;
    CURLMsg* msg = curl_multi_info_read( m_multiCurl, &messagesLeft );
    if( msg && msg->msg == CURLMSG_DONE ) {
        Log::info( cc::info( "Curl done with status " ) + cc::note( std::to_string( msg->data.result ) ) );
    }

    return true;
}

}  // namespace HubPlatform
