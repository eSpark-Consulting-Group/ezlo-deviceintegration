#include "database_client/dbtools.h"
#include "database_client/dbclient_limits.h"

#include "database_client/db_tables_names.h"
#include "system_utils/environment.h"
#include "system_utils/system.h"
#include "system_utils/logger.h"
#include "system_utils/filesystem.h"
#include <ostream>

std::string getStorageFilePath() {
    return HubPlatform::Environment::getDataFolder() + "/" + StorageFileName;
}

std::string getTemporaryStorageFilePath() {
    return HubPlatform::Environment::getTempDataFolder() + "/" + TemporaryStorageFileName;
}

std::string getPluginDBName( const std::string& pluginId ) {
    return "db.plugins." + pluginId + ".storage";
}

namespace HubPlatform {
namespace DatabaseClient {

/* Get an amount of storage for main database map:
 *      - get storage path
 *      - get size of the flash drive
 *      - calculate 30% of it, but not less than 10Mb, but not greater than 2Gb
 *          - for H2_secure, 16Gb Flash drive: 16 Gb * 0.3 > 2 Gb, set to 2Gb.
 */
uint64_t getStorageMapSizeDefault() {
    auto storageFolder = HubPlatform::Environment::getDataFolder();

    const size_t osPageSize = System::getMemoryPageSize();

    uint64_t filesystemSize = 0;

    try {
        filesystemSize = filesystem::getFilesystemSize( Environment::getDataFolder().c_str() );
    }
    catch( const filesystem::Exception& ex ) {
        SystemUtils::logError() << "Could not get filesystem size: " << ex.what() << "\n";
    }
    uint64_t storageMapSize = filesystemSize * Limits::mapSizeMainThresholdPercent / 100;

    // Should be multiple of OS page size
    if( ( storageMapSize % osPageSize ) != 0 )
        storageMapSize += osPageSize - storageMapSize % osPageSize;

    if( storageMapSize < Limits::mapSizeDefault )
        return Limits::mapSizeDefault;

    if( storageMapSize > Limits::mapSizeMax )
        storageMapSize = Limits::mapSizeMax;

    return storageMapSize;
}

/** Calculates 30% of RAM, but at least 10Mb to use as a map size for temporary database storage. */
uint64_t getTempStorageMapSizeDefault() {
    System::MemoryInfo memoryInfo = System::getMemoryInfo();
    const size_t osPageSize = System::getMemoryPageSize();
    // Reserve percent of available RAM for temporary database tables, but not less than 10Mb.

    uint64_t storageMapSize = memoryInfo.totalram * Limits::mapSizeRamThresholdPercent / 100;

    // Should be multiple of OS page size
    if( storageMapSize % osPageSize != 0 )
        storageMapSize += osPageSize - storageMapSize % osPageSize;

    if( storageMapSize < Limits::mapSizeDefault )
        return Limits::mapSizeDefault;

    if( storageMapSize > Limits::mapSizeMax )
        storageMapSize = Limits::mapSizeMax;

    return storageMapSize;
}

}  // namespace DatabaseClient
}  // namespace HubPlatform