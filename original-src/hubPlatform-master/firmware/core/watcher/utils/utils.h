#pragma once

#include "watcher/types/process_info.h"

#include "json_utils/json_fwd.h"

#include <string>
#include <vector>

namespace HubPlatform {

class ConfigReader;

namespace Watcher {
namespace Utils {

std::string readOpenwrtPsCommandOutput();
std::vector< Types::ProcessInfo > parseOpenwrtPsOutput( const std::string& output );

std::vector< Types::ProcessInfo > readProcessInfoListFromProcFs();
Types::ProcessInfo parseProcFsStatus( const std::string& output );

int readConfigIntValue( ConfigReader& reader, const std::string& path, int defaultValue );
JsonObject readConfigObjectValue( ConfigReader& reader, const std::string& path, const JsonObject& defaultValue );

void storeLogsStuckSafe( std::vector< std::string > args );

}  // namespace Utils
}  // namespace Watcher
}  // namespace HubPlatform
