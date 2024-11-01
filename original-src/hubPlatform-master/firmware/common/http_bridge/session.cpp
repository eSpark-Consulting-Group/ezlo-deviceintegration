#include "http_bridge/session.h"
#include "http_bridge/resources.h"
#include "http_bridge/utils.h"
#include "http_bridge/errors.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "common_utils/streambuf_from_vector.h"
#include "tiny/tiny_consolecolors.h"
#include "tiny/tiny_utils.h"
#include "database_client/dbid.h"
#include "json_utils/json_utils.h"
#include <ctime>

namespace HubPlatform {
namespace HttpBridge {

SessionData::SessionData( lws* wsi )
: m_wsi( wsi )
, m_created( time( nullptr ) ) {
    m_id = db::DbId::genOID();
}

HttpBridgeCommand& SessionData::getCommand() {
    UniqueLock lock( m_mutex );
    return m_command;
}

Bytes& SessionData::getPostData() {
    UniqueLock lock( m_mutex );
    return m_postData;
}

/** Try to parse JSON from a buffer (can not be null-terminated) */
bool SessionData::parsePostData() {
    UniqueLock lock( m_mutex );

    if( !m_postData.empty() ) {
        try {
            CommonUtils::streambufFromVector< char > streamBuf( this->m_postData );
            std::istream is( &streamBuf );

            m_postDataJson = Json::parse( is );
            return true;
        }
        catch( const std::exception& e ) {
            Log::error( cc::error( "SessionData::parsePostData failed: " ) + e.what() );
        }
        catch( ... ) {
            Log::error( cc::error( "SessionData::parsePostData failed: ..." ) );
        }
    }
    return false;
}

const Json& SessionData::postDataJson() const {
    UniqueLock lock( m_mutex );
    return m_postDataJson;
}

lws* SessionData::getWsi() {
    UniqueLock lock( m_mutex );
    return m_wsi;
}

void SessionData::setAnswer( Json answer ) {
    UniqueLock lock( m_mutex );
    m_answer = std::move( answer );
}

Json SessionData::getAnswer() const {
    UniqueLock lock( m_mutex );
    return m_answer;
}

bool SessionData::notify() {
    UniqueLock lock( m_mutex );

    // Sometimes answer data isn't written before HTTP_DROP_PROTOCOL is received. Use m_isFinished to determine.
    if( m_wsi ) {
        if( !m_isFinished ) {
            lws_callback_on_writable( m_wsi );
            return true;
        }
        else {
            LOGGER_ERROR_PRINT( "Connection already finished for lws: %p", m_wsi );
        }
    }
    return false;
}

bool SessionData::isPending() const {
    return m_isPending;
}

void SessionData::isPending( bool v ) {
    m_isPending.exchange( v );
}

/** Build serialized version of an answer and return next portion from it; return empty string for the last portion.
 */
std::string SessionData::getSerializedAnswer( size_t portionSize ) {
    UniqueLock lock( m_mutex );

    if( portionSize == 0 )
        return m_serializedAnswer;
    if( m_portionStart >= m_portionEnd )
        return {};

    std::string data = m_serializedAnswer.substr( m_portionStart, portionSize );
    // move stream pointer
    m_portionStart += portionSize;

    return data;
}

void SessionData::setSerializedAnswer( std::string value ) {
    UniqueLock lock( m_mutex );

    m_serializedAnswer = std::move( value );
    m_portionStart = 0;
    m_portionEnd = m_serializedAnswer.size();
}

void SessionData::buildResponse( int indent ) {
    Json result = Json::object();
    indent = indent < -1 || indent > 8 ? 4 : indent;
#ifdef USE_VERBOSE_DEBUG_OUTPUT
    time_t t = time( nullptr );
    Json headers = Json::object();

    for( const auto& it : this->knownHeaders() ) {
        headers[ it.first ] = it.second;
    }

    result[ "headers" ] = headers;
    result[ "request" ] = getCommand().toJson();

    result[ "request" ][ "post_data_json" ] = postDataJson();

    char timeBuf[ 64 ];
    strftime( timeBuf, sizeof( timeBuf ), "%T", localtime( &t ) );
    result[ "now" ] = timeBuf;

    result[ "answer" ] = getAnswer();
#else
    result = getAnswer();
#endif

    setSerializedAnswer( result.dump( indent ) );  // serialized here
}

bool SessionData::hasSerializedAnswer() const {
    UniqueLock lock( m_mutex );
    return !m_serializedAnswer.empty();
}

void SessionData::finished( bool value ) {
    m_isFinished.exchange( value );
}

bool SessionData::isFinished() const {
    return m_isFinished;
}

std::string& SessionData::getPath() {
    return m_path;
}

SessionData::Headers& SessionData::knownHeaders() {
    return m_knownHeaders;
}

void SessionData::addHeader( const std::string& name, const std::string& value ) {
    auto header = Header( name, value );
    m_knownHeaders.push_back( header );
}

const SessionData::Headers& SessionData::knownHeaders() const {
    return m_knownHeaders;
}

Json SessionData::dumpHeaders( bool hidePrivateHeaders ) {
    JsonArray result = JSON_EMPTY_ARR;
    for( const auto& header : m_knownHeaders ) {
        const auto& name = header.first;
        auto value = header.second;
        if( hidePrivateHeaders && name == "authorization:" ) {
#ifndef _DEBUG
            value = "[hidden]";
#endif
        }
        result.push_back( { { name, value } } );
    }
    return result;
}

bool SessionData::findAuthHeaders( std::string& user, std::string& token ) {
    UniqueLock lock( m_mutex );
    std::tie( user, token ) = Utils::extractAuthData( m_wsi );
    return !user.empty() && !token.empty();
}

time_t SessionData::created() const {
    return m_created;
}

void SessionData::appendBroadcast( std::string msg ) {
    if( msg.empty() )
        return;
    UniqueLock lock( m_mutex );
    m_broadcasts.push_back( std::move( msg ) );
}

bool SessionData::nextBroadcast() {
    UniqueLock lock( m_mutex );
    if( !m_broadcasts.empty() ) {
        setSerializedAnswer( m_broadcasts.front() );
        m_broadcasts.pop_front();
        return true;
    }
    return false;
}

Json SessionData::getRequestByHandler( const std::string& handler ) {
    Json request;
    if( handler == Resources::httpMethodGet )
        request = getCommand().toJson();
    else if( handler == Resources::httpMethodPost )
        request = postDataJson();

    assignMessageId( request, m_id );
    return request;
}

void SessionData::isLoggedIn( bool value ) {
    m_isLoggedIn = value;
}

bool SessionData::isLoggedIn() const {
    return m_isLoggedIn;
}

void SessionData::addBytesWritten( size_t value ) {
    UniqueLock lock( m_mutex );
    m_bytesWritten += value;
}

size_t SessionData::bytesWritten() const {
    UniqueLock lock( m_mutex );
    return m_bytesWritten;
}

void SessionData::headersSent( bool value ) {
    UniqueLock lock( m_mutex );
    m_headersSent = value;
}

bool SessionData::headersSent() const {
    UniqueLock lock( m_mutex );
    return m_headersSent;
}

std::pair< bool, std::string > SessionData::parseGetCommand( const std::string& url ) {
    try {
        if( m_command.parse( url ) )
            return { true, std::string{} };
    }
    catch( const HubPlatform::HttpBridge::ApiBridgeError& e ) {
        Log::error( cc::error( "Api bridge command error: " ) + e.what() );
        return { false, e.what() };
    }
    catch( const std::runtime_error& e ) {
        Log::error( cc::error( "Api bridge command error: " ) + e.what() );
        return { false, e.what() };
    }
    return { false, std::string{} };
}

int SessionData::delayed() const {
    UniqueLock lock( m_mutex );
    return m_delayed;
}

void SessionData::delayed( int value ) {
    UniqueLock lock( m_mutex );
    m_delayed = value;
}

void SessionData::assignMessageId( Json& request, const std::string& id ) {
    if( request.count( "id" ) == 0 )
        request[ "id" ] = id;
}

/** Returns true if a header is found and has a value specified. Case-insensitive string comparison is used.
 * @param name - name of a header, followed by : (example: expect:, Host:)
 * @param value - value of a header to compare; if empty, no value comparison will be made.
 * @returns - true if the header is found
 * */
bool SessionData::hasHeader( const std::string& name, const std::string& value ) const {
    const std::string headerToFind = tiny::tools::to_lower( name );
    const std::string valueToFind = tiny::tools::to_lower( value );
    const auto& collection = knownHeaders();
    return std::any_of( std::begin( collection ), std::end( collection ), [&]( const SessionData::Header& header ) {
        return header.first == headerToFind && ( valueToFind.empty() || header.second == valueToFind );
    } );
}

std::string SessionData::getHeaderValue( const std::string& name ) const {
    std::string headerToFind = tiny::tools::to_lower( name );
    headerToFind += ":";
    auto& collection = knownHeaders();
    std::string value;

    const auto it = std::find_if( collection.begin(), collection.end(), [&]( const SessionData::Header& header ) {
        if( header.first == headerToFind ) {
            value = header.second;
            return true;
        }
        return false;
    } );

    return value;
}

Session::Session( Session::SessionType type )
: m_type( type ) {}

Session::~Session() {
    if( m_data ) {
        m_data.reset();
    }
    clear();
}

void Session::init( lws* wsi ) {
    if( !m_data ) {
        m_data = std::make_unique< SessionData >( wsi );
    }
}

SessionData* Session::getData() {
    return m_data.get();
}

void Session::clear() {
    m_data.reset();
}

Session::SessionType Session::sessionType() const {
    return m_type;
}
void Session::sessionType( Session::SessionType value ) {
    m_type = value;
}

void Session::postMessage( const std::string& msg ) {
    if( !m_data || msg.empty() )
        return;
    m_data->appendBroadcast( msg );
    m_data->notify();
}

unsigned Session::extractHeaders( lws* wsi ) {
    if( getData() == nullptr )
        init( wsi );

    SessionData& sessionData = *getData();
    extractHeaderTokens( wsi, sessionData );

    extractUriParams( wsi, sessionData.getCommand() );
    return sessionData.knownHeaders().size();
}

int Session::extractHeaderTokens( lws* wsi, SessionData& sessionData ) {
    int counter = 0;
    int current = WSI_TOKEN_GET_URI, end = WSI_TOKEN_COUNT;
    // TODO: Avoid using fixed-length buffer for header extraction!
    std::array< lws_token_indexes, 7 > httpMethods = { WSI_TOKEN_GET_URI,   WSI_TOKEN_POST_URI, WSI_TOKEN_OPTIONS_URI,
                                                       WSI_TOKEN_PATCH_URI, WSI_TOKEN_PUT_URI,  WSI_TOKEN_DELETE_URI,
                                                       WSI_TOKEN_HEAD_URI };

    char buf[ 4086 ];
    for( ; current != end; current++ ) {
        auto index = static_cast< lws_token_indexes >( current );

        auto copied = lws_hdr_copy( wsi, buf, sizeof( buf ) - 1, index );
        if( copied <= 0 )
            continue;

        const char* str = reinterpret_cast< const char* >( lws_token_to_string( index ) );
        std::string header( str ? str : "" );
        tiny::tools::rtrim( header );

        auto foundIt = std::find( httpMethods.begin(), httpMethods.end(), index );
        if( foundIt != httpMethods.end() ) {
            method = index;
        }

        // Convert header names to lowercase to avoid problems with http/2
        sessionData.knownHeaders().emplace_back( std::make_pair( tiny::tools::to_lower( header ), buf ) );
        counter++;
    }
    return counter;
}

int Session::extractUriParams( lws* wsi, HttpBridgeCommand& command ) {
    char buf[ 4086 ];
    int queryParams = 0, extracted = 0;
    while( lws_hdr_copy_fragment( wsi, buf, sizeof( buf ), WSI_TOKEN_HTTP_URI_ARGS, queryParams ) > 0 ) {
        char* equalSign = strstr( buf, "=" );
        if( equalSign ) {
            std::string key{ buf, equalSign }, value{ equalSign + 1 };
            lwsl_notice( "URI Arg %d: %s\n", ++queryParams, buf );
            command.appendParam( std::move( key ), std::move( value ) );
        }
        else {
            lwsl_notice( "URI Arg %d: %s (equal sign not found)\n", ++queryParams, buf );
            std::string key{ buf };
            command.appendParam( std::move( key ), "" );
        }
        extracted++;
    }
    return extracted;
}

std::string SessionData::peerDebugInfo() {
    std::string result;
    try {
        auto headers = knownHeaders();

        std::string parameter = "user-agent";
        const auto& userAgent =
            std::find_if( begin( headers ), headers.end(), [parameter]( SessionData::Header& header ) {
                return header.first.find( parameter ) != std::string::npos;
            } );

        if( userAgent != headers.end() ) {
            result += parameter + ": " + userAgent->second;
        }
        auto requestId = getCommand().findParam( "random" );
        if( requestId.empty() )
            requestId = getId();
        if( !result.empty() )
            result += ", ";
        result += "etag: " + requestId;

        Json headersArray = dumpHeaders( true );
        result += ", headers: " + headersArray.dump();
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( "peerDebugInfo error: " + ex.what() );
    }
    return result;
}

void SessionData::setId( const std::string& id ) {
    addHeader( "etag:", id );
}

std::string SessionData::getId() const {
    return getHeaderValue( "etag" );
}

}  // namespace HttpBridge
}  // namespace HubPlatform
