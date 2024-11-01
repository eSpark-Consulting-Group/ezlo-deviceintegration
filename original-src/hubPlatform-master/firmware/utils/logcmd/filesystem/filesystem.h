#pragma once

#include "logcmd/filesystem/filesystem_interface.h"

namespace HubPlatform {
namespace LogCmd {

class Filesystem : public IFilesystem {
public:
    void createDirectory( const std::string& path, bool recursive ) override;
    void moveFile( const std::string& srcPath, const std::string& dstPath ) override;
    void copyFile( const std::string& srcPath, const std::string& dstPath ) override;
    void truncateFile( const std::string& path ) override;
    void removeFile( const std::string& path ) override;
    uint64_t getFileSize( const std::string& path ) override;
    bool isDirectoryExist( const std::string& path ) override;
    std::vector< std::string > findFiles( const std::string& directory ) override;
};

}  // namespace LogCmd
}  // namespace HubPlatform
