#pragma once

#include <string>
#include <iosfwd>

namespace HubPlatform {
namespace SystemUtils {

std::string getTimeStamp();
std::ostream& logError( std::ostream* os = nullptr );
std::ostream& logWarn( std::ostream* os = nullptr );
std::ostream& logInfo( std::ostream* os = nullptr );
std::ostream& logDebug( std::ostream* os = nullptr );
std::ostream& logTrace( std::ostream* os = nullptr );

}  // namespace SystemUtils
}  // namespace HubPlatform
