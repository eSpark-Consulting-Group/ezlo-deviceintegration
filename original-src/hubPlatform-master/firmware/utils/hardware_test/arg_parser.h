#pragma once

#include <string>
#include <map>
#include <vector>

#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HardwareTest {
namespace ArgUtils {
bool isNumber( const std::string& str );
bool isIpAddr( const std::string& str );
}  // namespace ArgUtils

class ArgumentError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class ArgParser {
public:
    ArgParser( const std::vector< std::string >& args, const std::vector< std::string >& knownTests );
    void help() const;
    bool hasArg( const std::string& argName ) const;
    std::string getValue( const std::string& argName ) const;
    std::vector< std::string > getArray( const std::string& argName ) const;

private:
    std::map< std::string, std::vector< std::string > > m_parsedArgs;
    std::vector< std::string > m_knownTests;
    void parseArgs( const std::vector< std::string >& args );
};

}  // namespace HardwareTest
}  // namespace HubPlatform
