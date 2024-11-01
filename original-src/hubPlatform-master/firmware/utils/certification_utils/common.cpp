#include "certification_utils/common.h"

#include "system_utils/environment.h"

#include <string>
#include <regex>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

namespace HubPlatform {
namespace CertificationUtils {

static const std::string StopScriptPath = Environment::getBinariesFolder().append( "/stop.sh" );
static const std::string MigrationToolPath = Environment::getBinariesFolder().append( "/migration_tool" );

void stopFirmware() {
    if( access( StopScriptPath.c_str(), X_OK ) != -1 ) {
        system( StopScriptPath.c_str() );
    }
    else {
        throw std::runtime_error( std::string( __PRETTY_FUNCTION__ ).append( ": stop script NOT found !" ) );
    }
    if( access( MigrationToolPath.c_str(), X_OK ) != -1 ) {
        system( MigrationToolPath.c_str() );
    }
    else {
        throw std::runtime_error( std::string( __PRETTY_FUNCTION__ ).append( ": migration binary NOT found !" ) );
    }
}

bool isNumber( const std::string& str ) {
    static const std::regex re( "[-]?\\d+" );
    return std::regex_match( str, re );
}

}  // namespace CertificationUtils
}  // namespace HubPlatform
