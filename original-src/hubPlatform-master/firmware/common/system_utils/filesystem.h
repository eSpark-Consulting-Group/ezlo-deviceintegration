
#pragma once

#include <string>
#include <functional>
#include <vector>

#include <ftw.h>

namespace HubPlatform {
namespace filesystem {

namespace File {
using Name = std::string;
using Extension = std::string;
using FullName = std::string;
using File = std::pair< Name, Extension >;
}  // namespace File

class Exception : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

using ErrorReporter = std::function< void( int, const std::string&, const std::string& ) >;

bool isDirectoryExist( const char* strPath );
bool isDirectoryExist( const std::string& strPath );

bool isFileExist( const char* strPath );
bool isFileExist( const std::string& strPath );

uint64_t getFileSize( const char* strPath );
uint64_t getFileSize( const std::string& strPath );

File::File getFilenameAndExtension( const std::string& strPath );
File::FullName getFilenameWithExtension( const std::string& strPath );
std::string getPath( const std::string& filePath );

bool isLinkExist( const char* strPath );
bool isLinkExist( const std::string& strPath );

bool isBlockDeviceExist( const char* strPath );
bool isBlockDeviceExist( const std::string& strPath );

bool isCharacterDeviceExist( const char* strPath );
bool isCharacterDeviceExist( const std::string& strPath );

bool isExist( const char* strPath );
bool isExist( const std::string& strPath );

std::string getSymlinkValue( const std::string& path );
std::string getSymlinkValue( const char* path );

bool createSymLink( const std::string& from, const std::string& to );

bool removeFile( const char* path );
bool removeFile( const std::string& filePath );
bool deleteFile( const std::string& filePath );

void truncateFile( const std::string& filePath, size_t size );

bool clearFolder( const std::string& folderPath );
bool deleteFolder( const std::string& folderPath );

bool createFolder( const std::string& folderPath );
bool createAndClearFolder( const std::string& folderPath, ErrorReporter errorReporter = nullptr );
std::string createRandomFolder( const std::string& fixedPrefixName );

bool copyFile( const std::string& srcPath, const std::string& dstPath );
bool copyFile( const std::string& srcPath, const std::string& dstPath, std::string& errorPath );

bool createSymbolicLink( const char* source, const char* linkName );
bool createSymbolicLink( const std::string& source, const std::string& linkName );

bool copyFolder( const std::string& sourcePath,
                 const std::string& destinationPath,
                 ErrorReporter errorReporter = nullptr );

/* order of entries is not specified - depends on a filesystem being iterated */
using EntryHandler = std::function< bool( const std::string& basePath, const std::string& entryName ) >;
using EntryHandler1 =
    std::function< bool( const std::string& basePath, const std::string& entryName, unsigned char entryType ) >;
bool iterateFileSystemEntry( const std::string& path, EntryHandler func );
bool iterateFileSystemEntry( const std::string& path, EntryHandler1 func );

/* this handler will be called for all entries in a walked tree,
 * order of a walk isn't specified, look at https://linux.die.net/man/3/nftw,
 * return a nonzero value if you want to stop walking immediately, otherwise zero */
using RecursivelyEntryHandler =
    std::function< int( const std::string& entryPath, const struct stat* status, int typeFlag, FTW* info ) >;
/* this function returns true if a whole tree was walked */
bool iterateFileSystemEntryRecursively( const std::string& path,
                                        RecursivelyEntryHandler func,
                                        int flags = FTW_PHYS,
                                        int numOpenFd = 15 );

std::vector< std::string > findAllFiles( const std::string& folderPath );

time_t fileModificationTime( const std::string& path );

uint64_t getFilesystemSize( const char* filePath );

std::vector< std::string > getDirectoryFileNames( const std::string& path );

uint64_t getFolderSize( const std::string& folderPath );
}  // namespace filesystem
}  // namespace HubPlatform
