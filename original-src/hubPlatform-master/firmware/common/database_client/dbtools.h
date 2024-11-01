
#pragma once

#include <string>

std::string getStorageFilePath();

std::string getTemporaryStorageFilePath();

std::string getPluginDBName( const std::string& pluginId );

namespace HubPlatform {
namespace DatabaseClient {
uint64_t getStorageMapSizeDefault();

uint64_t getTempStorageMapSizeDefault();
}  // namespace DatabaseClient
}  // namespace HubPlatform
