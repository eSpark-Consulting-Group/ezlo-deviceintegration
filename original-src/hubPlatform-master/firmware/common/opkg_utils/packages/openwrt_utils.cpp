#include "opkg_utils/packages/openwrt_utils.h"

#include "system_utils/filesystem.h"
#include "system_utils/system.h"

#include <fstream>
#include <unistd.h>

namespace HubPlatform {
namespace OpenwrtUtils {

StdOutWithReturnCode downloadImage( const Downloader& downloader,
                                    const std::string& imageUrl,
                                    const std::string& directory ) {
    if( directory.empty() ) {
        return { "\n", 0 };
    }

    auto imageFilePath = directory + "/sysupgrade.img";
    auto checksumFile = imageFilePath + ".md5";

    {
        auto result = downloader( imageUrl + ".md5", checksumFile );
        if( result.second != 0 ) {
            result.first = "Unable to download MD5 checksum: " + result.first;
            return result;
        }
    }

    {
        auto result = replaceMD5ChecksumPath( checksumFile, imageFilePath );
        if( result.second != 0 ) {
            return result;
        }
    }

    {
        auto result = downloader( imageUrl, imageFilePath );
        if( result.second != 0 ) {
            result.first = "Unable to download image file: " + result.first;
            return result;
        }
    }

    {
        auto result = verifyMD5Checksum( checksumFile );
        if( result.second != 0 ) {
            HubPlatform::filesystem::removeFile( imageFilePath );
            HubPlatform::filesystem::removeFile( checksumFile );
            return result;
        }
    }

    return { "\n", 0 };
}

StdOutWithReturnCode replaceMD5ChecksumPath( const std::string& checksumFilePath, const std::string& targetFilePath ) {
    const size_t md5len = 32;

    if( !filesystem::isFileExist( checksumFilePath ) ) {
        return { "MD5 checksum file not found: " + checksumFilePath, 1 };
    }

    std::string hash;
    {
        std::ifstream file( checksumFilePath );
        if( !std::getline( file, hash ) || hash.size() < md5len ) {
            return { "Invalid MD5 checksum file: " + checksumFilePath, 1 };
        }
        hash.resize( md5len );
    }

    std::ofstream file( checksumFilePath, std::ofstream::out );
    file << hash << "  " << targetFilePath;
    file.flush();
    file.close();

    return { "\n", 0 };
}

StdOutWithReturnCode verifyMD5Checksum( const std::string& checksumFilePath ) {
    const std::string cmd = "md5sum -c " + checksumFilePath;
    auto result = System::getCommandOutput( cmd.c_str(), true /*catchStderr*/ );
    if( result.second != 0 ) {
        result.first = "MD5 checksum verification failed: " + result.first;
    }
    return result;
}

}  // namespace OpenwrtUtils
}  // namespace HubPlatform
