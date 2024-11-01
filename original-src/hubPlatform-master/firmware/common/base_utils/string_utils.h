#pragma once

#include <string>
#include <vector>

namespace HubPlatform {
namespace StringUtils {

std::string strip( const std::string& str );
std::vector< std::string > split( const std::string& str, const std::string& delim );
std::vector< std::string > split( const std::string& str );
std::vector< std::string > splitSequence( const std::string& str, const std::vector< std::string >& delims );
std::vector< std::string > splitByLines( const std::string& str );
std::vector< std::string > splitByStrippedLinesWithText( const std::string& str );
size_t countTokens( const std::string& str, const std::string& token );
size_t replaceTokens( std::string& str, const std::string& token, const std::string& value );

}  // namespace StringUtils
}  // namespace HubPlatform
