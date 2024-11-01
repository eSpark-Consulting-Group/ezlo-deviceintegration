#pragma once

#include "logcmd/filesystem/filesystem_interface.h"

#include <functional>
#include <map>
#include <set>
#include <vector>

namespace HubPlatform {
namespace UT {

class FakeFilesystem : public LogCmd::IFilesystem {
public:
    void createDirectory( const std::string& path, bool recursive ) override;
    void moveFile( const std::string& srcPath, const std::string& dstPath ) override;
    void copyFile( const std::string& srcPath, const std::string& dstPath ) override;
    void truncateFile( const std::string& path ) override;
    void removeFile( const std::string& path ) override;
    size_t getFileSize( const std::string& path ) override;
    bool isDirectoryExist( const std::string& path ) override;
    std::vector< std::string > findFiles( const std::string& directory ) override;

public:
    using Directory = std::string;
    using Filename = std::string;
    using Files = std::vector< Filename >;
    using Path = std::string;

    void registerFileSizes( std::map< Path, size_t > sizeMap );
    void registerFiles( Directory dirname, Files files );
    void registerDirectory( Directory dirname );

public:
    std::map< Path, size_t > m_fileSizeMap;
    std::set< Path > m_isDirectoryExistsSet;
    std::map< Directory, Files > m_directoryFilesMap;
    std::set< Path > m_removedFilesSet;
};

}  // namespace UT
}  // namespace HubPlatform
