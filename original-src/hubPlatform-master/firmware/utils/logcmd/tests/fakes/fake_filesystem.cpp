#include "logcmd/tests/fakes/fake_filesystem.h"

namespace HubPlatform {
namespace UT {

void FakeFilesystem::createDirectory( const std::string& path, bool recursive ) {}
void FakeFilesystem::moveFile( const std::string& srcPath, const std::string& dstPath ) {}
void FakeFilesystem::copyFile( const std::string& srcPath, const std::string& dstPath ) {}
void FakeFilesystem::truncateFile( const std::string& path ) {}

void FakeFilesystem::removeFile( const std::string& path ) {
    m_removedFilesSet.emplace( path );
}

size_t FakeFilesystem::getFileSize( const std::string& path ) {
    auto it = m_fileSizeMap.find( path );
    if( it != m_fileSizeMap.end() )
        return it->second;
    return 0;
}

bool FakeFilesystem::isDirectoryExist( const std::string& path ) {
    return m_isDirectoryExistsSet.count( path );
}

std::vector< std::string > FakeFilesystem::findFiles( const std::string& directory ) {
    auto it = m_directoryFilesMap.find( directory );
    if( it != m_directoryFilesMap.end() )
        return it->second;
    return {};
}

void FakeFilesystem::registerFileSizes( std::map< Path, size_t > sizeMap ) {
    for( auto& element : sizeMap )
        m_fileSizeMap[ element.first ] = element.second;
}

void FakeFilesystem::registerFiles( Directory dirname, Files files ) {
    m_directoryFilesMap.emplace( std::move( dirname ), std::move( files ) );
}

void FakeFilesystem::registerDirectory( Directory dirname ) {
    m_isDirectoryExistsSet.emplace( std::move( dirname ) );
}

}  // namespace UT
}  // namespace HubPlatform
