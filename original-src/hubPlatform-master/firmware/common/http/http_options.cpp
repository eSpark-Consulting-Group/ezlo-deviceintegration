#include "http/http_options.h"
#include "json_utils/json_utils.h"
#include <curl/curl.h>
#include <utility>

namespace HubPlatform {
namespace CommonHttp {

void HttpOptions::validate() {
    using namespace CommonHttp::Limits;

    if( type.empty() )
        type = "GET";
    if( redirection > MAX_REDIRECTION )
        redirection = MAX_REDIRECTION;

    const auto MAX_DATA_BUFFER = MAX_DATA_BUFFER_LINUX;

    if( 0 == newDataBuffer || newDataBuffer > MAX_DATA_BUFFER )
        newDataBuffer = MAX_DATA_BUFFER;
    else if( newDataBuffer < MIN_DATA_BUFFER )
        newDataBuffer = MIN_DATA_BUFFER;

    if( 0 == timeoutMs )
        timeoutMs = DEF_TIMEOUT_MS;
    else if( timeoutMs > MAX_TIMEOUT_MS )
        timeoutMs = MAX_TIMEOUT_MS;

    if( 0 == connectTimeoutMs )
        connectTimeoutMs = DEF_CONNECT_TIMEOUT_MS;
    else if( connectTimeoutMs > MAX_CONNECT_TIMEOUT_MS )
        connectTimeoutMs = MAX_CONNECT_TIMEOUT_MS;
}

const char* HttpOptions::commandName( ERequestType command ) {
    if( command < ERequestType::GET || command >= ERequestType::END_OF_COMMANDS )
        return "";
    static const char* commands[] = {
        "GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE",
    };
    return commands[ (unsigned) command ];
}

int HttpOptions::version() {
    return DOCS_VERSION;
}

template < class T >
bool setOptional( Json& j, const char* key, const T& value, const T& defaultValue = T() ) {
    if( !key || !*key || value == defaultValue )
        return false;
    j[ key ] = value;
    return true;
}

bool setOptional( Json& j, const char* key, const std::string& value, const std::string& defaultValue = "" ) {
    if( !key || !*key || value.empty() || value == defaultValue )
        return false;
    j[ key ] = value;
    return true;
}

void to_json( Json& j, const Certificates& certificates ) {
    setOptional( j, "ca", certificates.ca );
    setOptional( j, "key", certificates.privateKey );
    setOptional( j, "public", certificates.publicKey );
}

void from_json( const Json& j, Certificates& certificates ) {
    if( j.count( "ca" ) )
        certificates.ca = j.at( "ca" ).get< std::string >();
    if( j.count( "key" ) )
        certificates.privateKey = j.at( "key" ).get< std::string >();
    if( j.count( "public" ) )
        certificates.publicKey = j.at( "public" ).get< std::string >();
}

void to_json( Json& json, const HttpOptions& opts ) {
    Json j = Json{ { "url", opts.url }, { "handler", opts.handler } };
    // optional fields
    setOptional( j, "type", opts.type, "GET" );
    setOptional( j, "skip_security", opts.skipSecurity, false );
    if( !opts.skipSecurity && !opts.certs.empty() )
        to_json( j[ "certs" ], opts.certs );

    setOptional( j, "content_type", opts.contentType );
    setOptional( j, "user", opts.user );
    setOptional( j, "password", opts.password );

    setOptional( j, "keep_alive", opts.keepAlive, false );
    setOptional( j, "repeat_allowed", opts.repeatAllowed, true );
    setOptional( j, "contentLength", opts.contentLength, 0 );
    setOptional( j, "data", opts.data );
    setOptional( j, "last", opts.last, true );
    setOptional( j, "redirection", opts.redirection, (unsigned char) CommonHttp::Limits::DEF_REDIRECTION );
    setOptional( j, "user_data", opts.userData );
    setOptional( j, "new_data_buffer", opts.newDataBuffer, 0 );
    setOptional( j, "new_data_cache", opts.newDataCache, true );
    setOptional( j, "timeout", opts.timeoutMs, (int) CommonHttp::Limits::DEF_TIMEOUT_MS );
    setOptional( j, "connect_timeout", opts.connectTimeoutMs, (int) CommonHttp::Limits::DEF_CONNECT_TIMEOUT_MS );
    setOptional( j, "verbose", opts.verbose, false );
    setOptional( j, "fail_on_error", opts.failOnError, true );
    setOptional( j, "multiple_headers", opts.multipleHeaders, false );

    {
        Json headersTarget;
        to_json( headersTarget, opts.headers, opts.multipleHeaders );
        if( !headersTarget.empty() )
            j[ "headers" ] = std::move( headersTarget );
    }

    json = std::move( j );
}

/** According to https://confluence.mios.com/display/ELFD/Lua+HTTP v. 39 */
void from_json( const Json& j, HttpOptions& opts ) {
    json_get( j, "url", opts.url );
    json_get( j, "handler", opts.handler );
    json_get( j, "skip_security", opts.skipSecurity );
    json_get( j, "type", opts.type );
    json_get( j, "content_type", opts.contentType );
    json_get( j, "user", opts.user );
    json_get( j, "password", opts.password );
    json_get( j, "keep_alive", opts.keepAlive );
    json_get( j, "repeat_allowed", opts.repeatAllowed );
    json_get( j, "content_length", opts.contentLength );
    json_get( j, "data", opts.data );
    json_get( j, "last", opts.last );
    json_get( j, "redirection", opts.redirection );
    json_get( j, "user_data", opts.userData );
    json_get( j, "new_data_buffer", opts.newDataBuffer );
    json_get( j, "new_data_cache", opts.newDataCache );
    json_get( j, "timeout", opts.timeoutMs );
    json_get( j, "connect_timeout", opts.connectTimeoutMs );

    if( j.count( "certs" ) )
        from_json( j.at( "certs" ), opts.certs );

    if( j.count( "headers" ) ) {
        const auto& jHeaders = j[ "headers" ];
        from_json( jHeaders, opts.headers );
    }

    json_get( j, "verbose", opts.verbose );
    json_get( j, "fail_on_error", opts.failOnError );
    json_get( j, "multiple_headers", opts.multipleHeaders );

    // validate options here, set defaults or valid ranges limits
    opts.validate();
}

void to_json( Json& j, const Headers& headers, bool multipleHeaders ) {
    if( headers.empty() )
        return;

    if( multipleHeaders ) {
        // serialize headers to an array of (key, value) pairs (backward incompatible way)
        // Serialize it as an array of { key: kayValue, value: valueValue } objects
        JsonArray headersArr = Json::array();
        for( const auto& header : headers ) {
            headersArr.push_back( { { "key", header.first }, { "value", header.second } } );
        }
        j = std::move( headersArr );
    }
    else {
        j = headers;
    }
}

void from_json( const Json& jHeaders, Headers& headers ) {
    if( jHeaders.is_object() ) {
        for( auto it = jHeaders.begin(); it != jHeaders.end(); it++ ) {
            if( it.value().is_string() ) {
                headers.emplace( it.key(), it.value().get< std::string >() );
            }
            else if( it.value().is_number() ) {
                // TODO: check if this situation is possible
                headers.emplace( it.key(), it.value().dump() );
            }
            else {
                throw OptionsError( "from_json( Headers ): unexpected type in headers" );
            }
        }
    }
    else if( jHeaders.is_array() ) {
        try {
            for( auto& h : jHeaders ) {
                if( !h.is_object() )
                    continue;

                auto keyIt = h.find( "key" );
                auto valueIt = h.find( "value" );
                if( keyIt == h.end() || valueIt == h.end() || !keyIt->is_string() )
                    continue;

                if( valueIt->is_string() ) {
                    headers.emplace( json_get_str( *keyIt ), valueIt->get< std::string >() );
                }
                else if( valueIt->is_number() ) {
                    // TODO: check if this situation is possible
                    headers.emplace( json_get_str( *keyIt ), valueIt->dump() );
                }
                else {
                    throw OptionsError( "from_json( Headers ): unexpected type in headers" );
                }
            }
        }
        catch( const std::exception& ex ) {
            throw OptionsError( "from_json( Headers ): unexpected type in headers" );
        }
    }
}

Json ConnectionInfo::toJson() const {
    Json info = Json::object();
    json_add_str( info, "id", m_id );
    json_add_str( info, "url", m_url );
    if( !m_userData.empty() )
        info[ "user_data" ] = m_userData;
    json_add_int( info, "max_data_length", m_maxDataLength );

    return info;
}

ConnectionInfo::ConnectionInfo( std::string id, std::string url, Json userData, int maxDataLength )
: m_id( std::move( id ) )
, m_url( std::move( url ) )
, m_userData( std::move( userData ) )
, m_maxDataLength( maxDataLength ) {}

}  // namespace CommonHttp
}  // namespace HubPlatform
