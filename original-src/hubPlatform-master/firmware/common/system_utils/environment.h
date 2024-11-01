
#pragma once

#include <string>

namespace HubPlatform {
namespace Environment {

void setFirmwareRootFolder( std::string firmwarePath );
void setTempDataFolder( std::string tempFolderPath );
void setLogsFolder( std::string logsFolderPath );
void setProvisionDataFolder( std::string provisionFolderPath );
void setDataFolder( std::string dataFolderPath );

void setBusName( std::string busName );

std::string getFirmwareRootFolder();

std::string getBinariesFolder();
std::string getConfigsFolder();
std::string getSystemConfigsFolder();
std::string getDataFolder();
std::string getMediaFolder();
std::string getVideosFolder();
std::string getSnapshotsFolder();
std::string getLibrariesFolder();
std::string getCertificateFolder();
std::string getLogsFolder();
std::string getPostponedLogsFolder();
std::string getRotatedLogsFolder();
std::string getPendingLogsFolder();
std::string getLuaPackagesFolder();

std::string getAddonsFolder();
std::string getPluginsFolder();
std::string getCustomPluginsFolder();
std::string getImagesFolder();

std::string getTempDataFolder();
std::string getProvisionDataFolder();
std::string getPersistentFolder();

std::string getBusName();

const std::string& getCurrentProcessPath();
const std::string& getCurrentProcessName();

const std::string& getZwaveDevice();
const std::string& getZigbeeDevice();
const std::string& get2GigDevice();

std::string getCustomPackagesFolder();

/** Returns true if the hub type is a physical hub */
bool isPhysicalHub();
/** Returns true if the hub type is a not physical hub, hub type is one of: local, vhub. */
bool isLogicalHub();

}  // namespace Environment
}  // namespace HubPlatform
