#pragma once

#include <string>
#include <vector>

namespace HubPlatform {
namespace LogCmd {

class IFilesystem {
public:
    virtual void createDirectory( const std::string& path, bool recursive ) = 0;
    virtual void moveFile( const std::string& srcPath, const std::string& dstPath ) = 0;
    virtual void copyFile( const std::string& srcPath, const std::string& dstPath ) = 0;
    virtual void truncateFile( const std::string& path ) = 0;
    virtual void removeFile( const std::string& path ) = 0;
    virtual uint64_t getFileSize( const std::string& path ) = 0;
    virtual bool isDirectoryExist( const std::string& path ) = 0;
    virtual std::vector< std::string > findFiles( const std::string& directory ) = 0;
};

}  // namespace LogCmd
}  // namespace HubPlatform
