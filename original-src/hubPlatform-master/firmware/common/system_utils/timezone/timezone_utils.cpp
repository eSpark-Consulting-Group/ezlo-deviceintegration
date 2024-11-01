#include "system_utils/timezone/timezone_utils.h"

#include "system_utils/filesystem.h"

namespace HubPlatform {
namespace system_utils {

const char* const TimeZonesFolderPath = "/usr/share/zoneinfo/";

std::set< std::string > getAllSupportedTimeZones() {
    std::set< std::string > sortedList;
    std::string basePath( TimeZonesFolderPath );
    HubPlatform::filesystem::iterateFileSystemEntry(
        basePath, [&sortedList]( const std::string& basePath, const std::string& entryName ) -> bool {
            const auto fullPath = basePath + entryName;
            if( HubPlatform::filesystem::isDirectoryExist( fullPath ) ) {
                HubPlatform::filesystem::iterateFileSystemEntry(
                    fullPath,
                    [&sortedList, &entryName]( const std::string& fileBasePath,
                                               const std::string& timeZoneFile ) -> bool {
                        const auto fileFullPath = fileBasePath + "/" + timeZoneFile;
                        if( HubPlatform::filesystem::isFileExist( fileFullPath ) ) {
                            const auto entry = entryName + "/" + timeZoneFile;
                            sortedList.insert( entry );
                        }
                        return true;
                    } );
            }
            return true;
        } );

    return sortedList;
}

}  // namespace system_utils
}  // namespace HubPlatform
