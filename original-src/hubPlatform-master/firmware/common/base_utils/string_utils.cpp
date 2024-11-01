#include "base_utils/string_utils.h"

#include <cctype>
#include <sstream>

namespace HubPlatform {
namespace StringUtils {

std::string strip( const std::string& str ) {
    if( str.empty() )
        return {};

    size_t posBeg = 0;
    size_t posEnd = str.size();

    for( auto it = str.begin(); it != str.end() && std::isspace( *it ); ++it ) {
        ++posBeg;
    }

    for( auto it = str.rbegin(); it != str.rend() && std::isspace( *it ); ++it ) {
        --posEnd;
    }

    if( posBeg >= posEnd )
        return {};

    return str.substr( posBeg, posEnd - posBeg );
}

std::vector< std::string > split( const std::string& str ) {
    if( str.empty() )
        return {};

    std::vector< std::string > result;
    result.reserve( 2 );

    auto beg = str.cbegin();
    auto end = str.cend();
    while( beg != str.cend() ) {
        for( ; beg != str.cend() && std::isspace( *beg ); ++beg )
            ;
        end = beg;
        for( ; end != str.cend() && !std::isspace( *end ); ++end )
            ;
        if( beg != end ) {
            result.emplace_back( beg, end );
        }
        beg = end;
    }

    return result;
}

std::vector< std::string > split( const std::string& str, const std::string& delim ) {
    if( str.empty() )
        return {};

    if( delim.empty() )
        return { str };

    std::vector< std::string > result;
    result.reserve( 2 );
    size_t posBeg = 0;
    size_t posEnd = str.find( delim, posBeg );

    while( posEnd != std::string::npos ) {
        result.emplace_back( str.substr( posBeg, posEnd - posBeg ) );
        posBeg = posEnd + delim.size();
        posEnd = str.find( delim, posBeg );
    }
    result.emplace_back( str.substr( posBeg ) );
    return result;
}

std::vector< std::string > splitSequence( const std::string& str, const std::vector< std::string >& delims ) {
    if( str.empty() || delims.empty() )
        return {};

    std::vector< std::string > result;
    result.reserve( delims.size() + 1 );

    size_t posBeg = 0;
    size_t posEnd = 0;
    for( auto& delim : delims ) {
        if( delim.empty() )
            continue;

        posEnd = str.find( delim, posBeg );
        if( posEnd == std::string::npos )
            return {};

        result.emplace_back( str.substr( posBeg, posEnd - posBeg ) );
        posBeg = posEnd + delim.size();
    }
    result.emplace_back( str.substr( posBeg ) );
    return result;
}

std::vector< std::string > splitByLines( const std::string& str ) {
    std::vector< std::string > result;

    std::stringstream ss( str );
    std::string line;

    while( std::getline( ss, line ) ) {
        result.emplace_back( line );
    }

    return result;
}

std::vector< std::string > splitByStrippedLinesWithText( const std::string& str ) {
    std::vector< std::string > result;

    size_t posBeg = 0;
    size_t posEnd = 0;

    while( posEnd != std::string::npos ) {
        posEnd = str.find( '\n', posBeg );
        auto substr = strip( str.substr( posBeg, posEnd - posBeg ) );
        if( !substr.empty() )
            result.emplace_back( std::move( substr ) );
        posBeg = posEnd + 1;
    }
    return result;
}

size_t countTokens( const std::string& str, const std::string& token ) {
    if( token.empty() ) {
        return str.empty();
    }

    size_t matches = 0;
    auto pos = str.find( token );
    while( pos != str.npos ) {
        ++matches;
        pos = str.find( token, pos + token.size() );
    }
    return matches;
}

size_t replaceTokens( std::string& str, const std::string& token, const std::string& value ) {
    if( token.empty() || str.empty() )
        return 0;
    const size_t tokenSize = token.size();
    const size_t valueSize = value.size();
    size_t count = 0;

    auto pos = str.find( token );
    while( pos != std::string::npos ) {
        str.replace( pos, tokenSize, value );
        ++count;
        pos = str.find( token, pos + valueSize );
    }
    return count;
}

}  // namespace StringUtils
}  // namespace HubPlatform
