#pragma once

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <chrono>
#include <ctime>

namespace HubPlatform {
namespace LogCmd {

std::string getHubSerial();
std::string generateTimeString( std::time_t timestamp = 0 );
std::string generateArchiveName( const std::string& userString = "", std::time_t timestamp = 0 );
std::array< std::string, 5 > generateCronSchedule( size_t interval, size_t seed );

void updateSystemLogs( const std::string& logDirectory );

bool waitForCondition( const std::function< bool() >& condition,
                       const std::chrono::milliseconds& timeout,
                       const std::chrono::milliseconds& checkInterval );

}  // namespace LogCmd
}  // namespace HubPlatform
