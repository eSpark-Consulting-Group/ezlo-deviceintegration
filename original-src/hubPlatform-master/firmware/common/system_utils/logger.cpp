#include "system_utils/logger.h"
#include <memory.h>
#include <iostream>

namespace HubPlatform {
namespace SystemUtils {

std::string getTimeStamp() {
    time_t rawTime = 0;
    char buffer[ 80 ];
    memset( buffer, '\0', sizeof( buffer ) );

    time( &rawTime );
    struct tm timeInfo = *localtime( &rawTime );
    strftime( buffer, sizeof( buffer ), "%Y-%m-%d %H:%M:%S", &timeInfo );
    return buffer;
}

std::ostream& logMessage( std::ostream* os = &std::cerr, const char* logLevel = "DEBUG" ) {
    if( !os )
        os = &std::cerr;
    if( !logLevel || !*logLevel )
        logLevel = "DEBUG";
    *os << getTimeStamp() << " " << logLevel << ": ";
    return *os;
}

std::ostream& logError( std::ostream* os ) {
    return logMessage( os, "ERROR" );
}

std::ostream& logWarn( std::ostream* os ) {
    return logMessage( os, "WARN " );
}

std::ostream& logInfo( std::ostream* os ) {
    return logMessage( os, "INFO " );
}

std::ostream& logDebug( std::ostream* os ) {
    return logMessage( os, "DEBUG" );
}

std::ostream& logTrace( std::ostream* os ) {
    return logMessage( os, "TRACE" );
}

}  // namespace SystemUtils
}  // namespace HubPlatform
