#include "http_bridge/command.h"
#include "http_bridge/errors.h"
#include "base_utils/ezlo_common.h"
#include "base_utils/utils.h"
#include <cstring>

namespace HubPlatform {
namespace HttpBridge {

/** Parses an url and makes list of parameters passed.
 * Throws ApiBridgeError in case of ill-formed /v1/method parameter
 */
bool HttpBridgeCommand::parse( const std::string& url ) {
    if( url.empty() )
        return false;

    static const char method[] = "/v1/method/";
    const unsigned shiftTokens = 2;  // number of lexemes to be skipped
    if( 0 != strncmp( method, url.c_str(), sizeof( method ) - 1 ) )
        return false;

    using namespace ezlo::common::APIs;
    auto version = split_string( url.c_str(), "#" );
    if( !version.empty() ) {
        auto pathParams = split_string( version.front(), "?" );
        if( !pathParams.empty() ) {
            if( !parseMethod( pathParams.front(), shiftTokens ) )
                return false;
            auto it = pathParams.begin();
            if( ++it != pathParams.end() ) {
                parseParams( *it );
            }
        }

        if( version.size() >= 2 ) {
            version.pop_front();

            const auto& value = version.front();
            if( !value.empty() )
                parseVersion( value );  // should match \d+_d+
        }
    }

    // throws ApiBridgeError exception on ill-formed parameters
    m_preparedParams = prepareParams();

    return !m_method.empty();
}

bool HttpBridgeCommand::parseMethod( const std::string& path, unsigned shift ) {
    using namespace ezlo::common::APIs;

    if( !path.empty() ) {
        auto list = split_string( path, "/" );
        for( unsigned i = 0; i < shift; i++ )
            list.pop_front();

        // make command from list
        for( const auto& item : list ) {
            if( !m_method.empty() )
                m_method += ".";
            m_method += item;
        }
    }
    return !m_method.empty();
}

bool HttpBridgeCommand::parseParams( const std::string& params ) {
    using namespace ezlo::common::APIs;
    auto pairs = split_string( params, "&" );

    for( const auto& pair : pairs ) {
        auto list = split_string( pair, "=" );
        std::string key = list.front();
        list.pop_front();
        std::string value = list.front();
        if( !key.empty() ) {
            m_params.emplace_back( std::move( key ), std::move( value ) );
        }
    }

    return !m_params.empty();
}

Json HttpBridgeCommand::toJson() const noexcept {
    Json t = Json::object();
    t[ "method" ] = m_method;
    t[ "api" ] = m_version;
    t[ "params" ] = m_preparedParams;
    return t;
}

Json HttpBridgeCommand::prepareParams() {
    Json params = Json::object();
    for( const auto& pair : m_params ) {
        const auto& key = pair.first;
        const auto& value = pair.second;
        if( !key.empty() ) {
            auto newKey = isBool( key );
            if( !newKey.empty() ) {
                bool boolValue = checkBoolValue( newKey, value );
                params[ newKey ] = boolValue;
                continue;
            }

            newKey = isInt( key );
            if( !newKey.empty() ) {
                int intValue = checkIntValue( newKey, value );
                params[ newKey ] = intValue;
                continue;
            }

            newKey = isFloat( key );
            if( !newKey.empty() ) {
                double floatValue = checkFloatValue( newKey, value );
                params[ newKey ] = floatValue;
                continue;
            }

            params[ key ] = value;
        }
    }
    return params;
}

void HttpBridgeCommand::appendParam( std::string key, std::string value ) {
    m_params.emplace_back( std::move( key ), std::move( value ) );
}

bool HttpBridgeCommand::parseVersion( std::string version ) {
    if( !version.empty() ) {
        replaceAll( version, "_", "." );
        m_version = version;
    }
    return false;
}

std::string HttpBridgeCommand::findParam( const std::string& key ) {
    auto searcher = [&key]( const HttpBridgeCommand::Param& x ) -> bool { return x.first == key; };

    const auto& foundIt = std::find_if( begin( m_params ), m_params.end(), searcher );
    if( foundIt != m_params.end() )
        return foundIt->second;

    return std::string();
}

std::string HttpBridgeCommand::stripKeySuffix( const std::string& key, const char* suffix ) {
    suffix = suffix ? suffix : "";
    const size_t position = key.rfind( suffix );
    if( std::string::npos != position && position + strlen( suffix ) == key.size() ) {
        return std::string{ key.c_str(), key.c_str() + position };
    }
    return "";
}

std::string HttpBridgeCommand::isBool( const std::string& key ) {
    static const char suffixBool[] = "_bool";
    return stripKeySuffix( key, suffixBool );
}

std::string HttpBridgeCommand::isInt( const std::string& key ) {
    static const char suffixInt[] = "_int";
    return stripKeySuffix( key, suffixInt );
}

std::string HttpBridgeCommand::isFloat( const std::string& key ) {
    static const char suffixFloat[] = "_float";
    return stripKeySuffix( key, suffixFloat );
}

bool HttpBridgeCommand::checkBoolValue( const std::string& key, const std::string& value ) {
    bool boolValue = false;
    if( value == "true" )
        boolValue = true;
    else if( value == "false" )
        boolValue = false;
    else {
        throw ApiBridgeError( std::string{ "Invalid format for field: '" } + key + "': '" + value + "'", 400 );
    }
    return boolValue;
}

int HttpBridgeCommand::checkIntValue( const std::string& key, const std::string& value ) {
    char* end = nullptr;
    errno = 0;  // clear errno
    auto number = strtol( value.c_str(), &end, 10 );
    bool rangeError = ERANGE == errno;

    if( 0 == number && end == value.c_str() )
        throw ApiBridgeError( std::string{ "No conversion can be performed for int value: " }.append( value ), 400 );
    if( rangeError )
        throw ApiBridgeError( std::string{ "Range error for int value: " }.append( value ), 400 );
    if( *end != '\0' )
        throw ApiBridgeError( std::string{ "Invalid format for key '" }
                                  .append( key )
                                  .append( "': value is not an int: " )
                                  .append( value ),
                              400 );
    return number;
}

float HttpBridgeCommand::checkFloatValue( const std::string& key, const std::string& value ) {
    char* end = nullptr;
    float number = strtof( value.c_str(), &end );
    bool rangeError = number == HUGE_VALF || number == HUGE_VALL;

    if( 0 == number && end == value.c_str() )
        throw ApiBridgeError( std::string{ "No conversion can be performed for float value: " }.append( value ), 400 );
    if( rangeError )
        throw ApiBridgeError( std::string{ "Range error for float value: " }.append( value ), 400 );
    if( *end != '\0' )
        throw ApiBridgeError( std::string{ "Invalid format for key '" }
                                  .append( key )
                                  .append( "': value is not a float: " )
                                  .append( value ),
                              400 );
    return number;
}

const std::string& HttpBridgeCommand::method() const {
    return m_method;
}

const HttpBridgeCommand::Params& HttpBridgeCommand::params() const {
    return m_params;
}

const std::string& HttpBridgeCommand::version() const {
    return m_version;
}

}  // namespace HttpBridge
}  // namespace HubPlatform
