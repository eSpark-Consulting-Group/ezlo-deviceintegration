#include "logcmd/filesystem/filesystem.h"

#include "logcmd/log_exceptions.h"
#include "system_utils/filesystem.h"

namespace HubPlatform {
namespace LogCmd {

void Filesystem::createDirectory( const std::string& path, bool recursive ) {
    if( path.empty() )
        throw FilesystemError( "Failed to create directory: empty directory path" );

    std::string cmd = std::string( "mkdir " ) + ( ( recursive ) ? "-p " : "" ) + path;
    int result = std::system( cmd.c_str() );
    if( result != 0 )
        throw FilesystemError( "Failed to create directory: " + path );
}

void Filesystem::moveFile( const std::string& srcPath, const std::string& dstPath ) {
    if( srcPath.empty() )
        throw FilesystemError( "Failed to move: empty src path" );

    if( dstPath.empty() )
        throw FilesystemError( "Failed to move: empty dst path" );

    int result = std::system( ( "mv " + srcPath + " " + dstPath ).c_str() );
    if( result != 0 )
        throw FilesystemError( "Failed to move: " + srcPath + " to " + dstPath );
}

void Filesystem::copyFile( const std::string& srcPath, const std::string& dstPath ) {
    if( srcPath.empty() )
        throw FilesystemError( "Failed to move: empty src path" );

    if( dstPath.empty() )
        throw FilesystemError( "Failed to move: empty dst path" );

    int result = std::system( ( "cp " + srcPath + " " + dstPath ).c_str() );
    if( result != 0 )
        throw FilesystemError( "Failed to copy: " + srcPath + " to " + dstPath );
}

void Filesystem::truncateFile( const std::string& path ) {
    filesystem::truncateFile( path, 0 );
}

void Filesystem::removeFile( const std::string& path ) {
    filesystem::removeFile( path );
}

uint64_t Filesystem::getFileSize( const std::string& path ) {
    return filesystem::getFileSize( path );
}

bool Filesystem::isDirectoryExist( const std::string& path ) {
    return filesystem::isDirectoryExist( path );
}

std::vector< std::string > Filesystem::findFiles( const std::string& directory ) {
    return filesystem::findAllFiles( directory );
}

}  // namespace LogCmd
}  // namespace HubPlatform
