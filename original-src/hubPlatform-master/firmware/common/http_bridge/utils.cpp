#include "http_bridge/utils.h"

#include "base_utils/utils.h"
#include "tiny/tiny_utils.h"

#include <libwebsockets.h>

namespace HubPlatform {
namespace HttpBridge {
namespace Utils {

namespace {
bool isHeaderToken( lws_token_indexes index ) {
    switch( index ) {
    case WSI_TOKEN_GET_URI:               // "get "
    case WSI_TOKEN_POST_URI:              // "post "
    case WSI_TOKEN_OPTIONS_URI:           // "options "
    case WSI_TOKEN_CHALLENGE:             // "\x0d\x0a"
    case WSI_TOKEN_HTTP:                  // "http/1.1 "
    case WSI_TOKEN_HTTP_COLON_AUTHORITY:  // ":authority"
    case WSI_TOKEN_HTTP_COLON_METHOD:     // ":method"
    case WSI_TOKEN_HTTP_COLON_PATH:       // ":path"
    case WSI_TOKEN_HTTP_COLON_SCHEME:     // ":scheme"
    case WSI_TOKEN_HTTP_COLON_STATUS:     // ":status"
    case WSI_TOKEN_PATCH_URI:             // "patch"
    case WSI_TOKEN_PUT_URI:               // "put"
    case WSI_TOKEN_DELETE_URI:            // "delete"
    case WSI_TOKEN_HTTP_URI_ARGS:         // "uri-args"
    case WSI_TOKEN_PROXY:                 // "proxy "
    case WSI_TOKEN_HTTP1_0:               // "http/1.0 "
    case WSI_TOKEN_X_FORWARDED_FOR:       // "x-forwarded-for"
    case WSI_TOKEN_CONNECT:               // "connect "
    case WSI_TOKEN_HEAD_URI:              // "head "
    case WSI_TOKEN_COLON_PROTOCOL:        // ":protocol"
        return false;
    default:
        return true;
    }
}
}  // namespace

KeyValueVector extractUriParams( lws* wsi ) {
    KeyValueVector params;
    char buf[ 4086 ];
    int queryParams = 0;
    while( lws_hdr_copy_fragment( wsi, buf, sizeof( buf ), WSI_TOKEN_HTTP_URI_ARGS, queryParams ) > 0 ) {
        char* equalSign = strstr( buf, "=" );
        if( equalSign ) {
            std::string key{ buf, equalSign }, value{ equalSign + 1 };
            params.emplace_back( std::move( key ), std::move( value ) );
        }
        else {
            std::string key{ buf };
            params.emplace_back( std::move( key ), "" );
        }
        ++queryParams;
    }
    return params;
}

const char* extractMethod( lws* wsi ) {
    if( lws_hdr_total_length( wsi, WSI_TOKEN_GET_URI ) != 0 )
        return "GET";
    if( lws_hdr_total_length( wsi, WSI_TOKEN_POST_URI ) != 0 )
        return "POST";
    if( lws_hdr_total_length( wsi, WSI_TOKEN_PUT_URI ) != 0 )
        return "PUT";
    if( lws_hdr_total_length( wsi, WSI_TOKEN_PATCH_URI ) != 0 )
        return "PATCH";
    if( lws_hdr_total_length( wsi, WSI_TOKEN_DELETE_URI ) != 0 )
        return "DELETE";
    if( lws_hdr_total_length( wsi, WSI_TOKEN_OPTIONS_URI ) != 0 )
        return "OPTIONS";
    if( lws_hdr_total_length( wsi, WSI_TOKEN_HEAD_URI ) != 0 )
        return "HEAD";
    return "";
}

KeyValueVector extractHeaders( lws* wsi ) {
    KeyValueVector headers;

    int current = WSI_TOKEN_GET_URI, end = WSI_TOKEN_COUNT;
    // TODO: Avoid using fixed-length buffer for header extraction!
    char buf[ 4086 ];
    for( ; current != end; current++ ) {
        auto index = static_cast< lws_token_indexes >( current );

        // Do not extract authentication header, use extractAuthData function instead
        if( !isHeaderToken( index ) || index == WSI_TOKEN_HTTP_AUTHORIZATION )
            continue;

        auto copied = lws_hdr_copy( wsi, buf, sizeof( buf ), index );
        if( copied <= 0 )
            continue;

        const char* str = reinterpret_cast< const char* >( lws_token_to_string( index ) );
        if( str ) {
            // Remove trailing ':' symbol
            std::string header( str, strlen( str ) - 1 );
            headers.emplace_back( tiny::tools::to_lower( header ), buf );
        }
    }
    return headers;
}

KeyValue extractAuthData( lws* wsi ) {
    std::string user;
    std::string token;

    char buf[ 4086 ];
    auto copied = lws_hdr_copy( wsi, buf, sizeof( buf ), WSI_TOKEN_HTTP_AUTHORIZATION );
    if( copied > 0 ) {
        std::string basicAuth = buf;
        char pattern[] = "Basic ";
        auto pos = basicAuth.find( pattern );
        if( pos != std::string::npos ) {
            const std::string& s = basicAuth.substr( pos + sizeof( pattern ) - 1 );
            std::string pair = tiny::tools::base64::decode( s );

            auto firstColon = pair.find( ':' );
            if( firstColon != std::string::npos && firstColon > 0 ) {
                user = pair.substr( 0, firstColon );
                token = pair.substr( firstColon + 1 );
            }
        }
    }
    return std::make_pair( user, token );
}

}  // namespace Utils
}  // namespace HttpBridge
}  // namespace HubPlatform
