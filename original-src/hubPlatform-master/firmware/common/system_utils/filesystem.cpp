#include "system_utils/filesystem.h"
#include "system_utils/logger.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <cassert>
#include <memory>
#include <vector>
#include <array>

#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/statvfs.h>

namespace HubPlatform {
namespace filesystem {
namespace {

bool isValidPath( const char* strPath ) {
    return ( strPath != nullptr ) && ( strPath[ 0 ] != '\0' );
}

bool fillSystemInfoForPath( const char* strPath, struct stat& info ) {
    return isValidPath( strPath ) && ( stat( strPath, &info ) == 0 );
}

std::unique_ptr< DIR, int ( * )( DIR* ) > openDirectory( const std::string& destinationPath ) {
    return std::unique_ptr< DIR, int ( * )( DIR * dirp ) >(::opendir( destinationPath.c_str() ), &::closedir );
}

bool isSystemFolders( struct dirent& folderEntry ) {
    return folderEntry.d_type == DT_DIR &&
           ( ( strcmp( folderEntry.d_name, "." ) == 0 ) || ( strcmp( folderEntry.d_name, ".." ) == 0 ) );
}

}  // namespace

bool isDirectoryExist( const char* strPath ) {
    struct stat buffer;
    return fillSystemInfoForPath( strPath, buffer ) && ( S_ISDIR( buffer.st_mode ) != 0 );
}

bool isDirectoryExist( const std::string& strPath ) {
    return strPath.empty() ? false : isDirectoryExist( strPath.c_str() );
}

bool isFileExist( const char* strPath ) {
    struct stat buffer;
    return fillSystemInfoForPath( strPath, buffer ) && ( S_ISREG( buffer.st_mode ) != 0 );
}

bool isFileExist( const std::string& strPath ) {
    return strPath.empty() ? false : isFileExist( strPath.c_str() );
}

uint64_t getFileSize( const char* strPath ) {
    std::ifstream ifstr( strPath, std::ios::binary | std::ios::ate );
    if( ifstr.is_open() )
        return ifstr.tellg();
    return 0U;
}

uint64_t getFileSize( const std::string& strPath ) {
    return getFileSize( strPath.c_str() );
}

bool isLinkExist( const char* strPath ) {
    if( strPath == nullptr || strPath[ 0 ] == '\0' )
        return false;
    struct stat buffer;
    bool statIsFine = lstat( strPath, &buffer ) == 0;
    bool isLink = ( S_ISLNK( buffer.st_mode ) != 0 );
    bool res = statIsFine && isLink;
    return res;
}

bool isLinkExist( const std::string& strPath ) {
    return strPath.empty() ? false : isLinkExist( strPath.c_str() );
}

bool isBlockDeviceExist( const char* strPath ) {
    if( strPath == nullptr || strPath[ 0 ] == '\0' )
        return false;
    struct stat buffer;
    bool statIsFine = lstat( strPath, &buffer ) == 0;
    bool isBlk = ( S_ISBLK( buffer.st_mode ) != 0 );
    bool res = statIsFine && isBlk;
    return res;
}

bool isBlockDeviceExist( const std::string& strPath ) {
    return strPath.empty() ? false : isBlockDeviceExist( strPath.c_str() );
}

bool isCharacterDeviceExist( const char* strPath ) {
    if( strPath == nullptr || strPath[ 0 ] == '\0' )
        return false;
    struct stat buffer;
    bool statIsFine = lstat( strPath, &buffer ) == 0;
    bool isChr = ( S_ISCHR( buffer.st_mode ) != 0 );
    bool res = statIsFine && isChr;
    return res;
}

bool isCharacterDeviceExist( const std::string& strPath ) {
    return strPath.empty() ? false : isCharacterDeviceExist( strPath.c_str() );
}

bool isExist( const char* strPath ) {
    if( !isValidPath( strPath ) )
        return false;

    return ::access( strPath, F_OK ) == 0;
}

bool isExist( const std::string& strPath ) {
    return strPath.empty() ? false : isExist( strPath.c_str() );
}

std::string getSymlinkValue( const char* path ) {
    std::string result;
    constexpr const std::string::size_type PREALLOCATE_SIZE = 1024;
    result.resize( PREALLOCATE_SIZE );
    std::size_t bytesWritten = ::readlink( path, &result[ 0 ], result.size() );
    if( bytesWritten == -1 ) {
        return std::string();
    }
    result.resize( bytesWritten );
    return result;
}

std::string getSymlinkValue( const std::string& path ) {
    return getSymlinkValue( path.c_str() );
}

bool createSymLink( const std::string& from, const std::string& to ) {
    int ret = ::symlink( from.c_str(), to.c_str() );
    return ret == 0;
}

bool removeFile( const char* path ) {
    return remove( path ) == 0;
}

bool removeFile( const std::string& filePath ) {
    return removeFile( filePath.c_str() );
}

bool deleteFile( const std::string& filePath ) {
    return removeFile( filePath );
}

void truncateFile( const std::string& filePath, size_t size ) {
    auto ret = truncate( filePath.c_str(), size );
    if( ret != 0 )
        throw Exception( "Fail to truncate file (" + filePath + ") : " + std::string( std::strerror( errno ) ) );
}

bool clearFolder( const std::string& folderPath ) {
    struct dirent* entry = nullptr;

    auto dir = openDirectory( folderPath );
    if( !dir ) {
        return false;
    }

    while( ( entry = readdir( dir.get() ) ) ) {
        if( !isSystemFolders( *entry ) ) {
            std::string fullPath = folderPath + "/" + std::string( entry->d_name );
            if( isLinkExist( fullPath ) ) {
                if( remove( fullPath.c_str() ) != 0 ) {
                    return false;
                }
            }
            else if( isDirectoryExist( fullPath ) ) {
                if( !deleteFolder( fullPath ) ) {
                    return false;
                }
            }
            else if( isFileExist( fullPath ) ) {
                if( !removeFile( fullPath.c_str() ) ) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool deleteFolder( const std::string& folderPath ) {
    if( !isDirectoryExist( folderPath ) ) {
        return true;
    }

    if( !clearFolder( folderPath ) ) {
        return false;
    }

    return remove( folderPath.c_str() ) == 0;
}

bool createFolder( const std::string& folderPath ) {
    if( isDirectoryExist( folderPath ) ) {
        return true;
    }

    auto tryToCreateFolder = []( const std::string& path ) -> bool {
        if( isDirectoryExist( path ) ) {
            return true;
        }

        return mkdir( path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH ) == 0;
    };

    std::string creatingPath;
    std::size_t delimiterPosition = 0, previousDelimiterPosition = 0;

    do {
        delimiterPosition = folderPath.find_first_of( '/', delimiterPosition );
        if( delimiterPosition == std::string::npos ) {
            creatingPath +=
                folderPath.substr( previousDelimiterPosition, folderPath.size() - previousDelimiterPosition ) + "/";
        }
        else if( previousDelimiterPosition == delimiterPosition ) {
            creatingPath += '/';
        }
        else {
            creatingPath +=
                folderPath.substr( previousDelimiterPosition, delimiterPosition - previousDelimiterPosition ) + "/";
        }

        if( !tryToCreateFolder( creatingPath ) ) {
            return false;
        }

        delimiterPosition = folderPath.find_first_not_of( '/', delimiterPosition );
        previousDelimiterPosition = delimiterPosition;

    } while( delimiterPosition != std::string::npos );

    return true;
}

bool createAndClearFolder( const std::string& folderPath, ErrorReporter errorReporter ) {
    if( isDirectoryExist( folderPath ) ) {
        bool res = clearFolder( folderPath );
        if( !res ) {
            if( errorReporter )
                errorReporter( errno, "Can't clear folder", folderPath );
            return false;
        }
    }

    bool res = createFolder( folderPath );
    if( !res ) {
        if( errorReporter )
            errorReporter( errno, "Can't create folder", folderPath );
        return false;
    }

    return true;
}

std::string createRandomFolder( const std::string& fixedPrefixName ) {
    const auto templateString = fixedPrefixName + "/XXXXXX";
    std::vector< char > templ( templateString.begin(), templateString.end() );
    templ.push_back( '\0' );

    if( !createFolder( fixedPrefixName ) ) {
        throw Exception( "Failed to create top folders, " + fixedPrefixName );
    }

    if( !mkdtemp( templ.data() ) ) {
        std::array< char, 512 > buffer;
        buffer[ 0 ] = '\0';
#if defined( __HUB_TYPE_LOCAL__ )
        const char* errnoDescription = strerror_r( errno, buffer.data(), buffer.size() );
        throw Exception( "Failed to generate a random folder, " + templateString + ", " + errnoDescription );
#else
        strerror_r( errno, buffer.data(), buffer.size() );
        throw Exception( "Failed to generate a random folder, " + templateString + ", " + buffer.data() );
#endif
    }

    return templ.data();
}

bool copyFile( const std::string& srcPath, const std::string& dstPath ) {
    std::string errorPath;
    return copyFile( srcPath, dstPath, errorPath );
}

std::streampos fileSize( const std::string& filePath ) {
    std::ifstream myfile( filePath );
    auto begin = myfile.tellg();
    myfile.seekg( 0, std::ios::end );
    auto end = myfile.tellg();
    myfile.close();

    return end - begin;
}

bool copyFile( const std::string& srcPath, const std::string& dstPath, std::string& errorPath ) {
    int srcFileSize = fileSize( srcPath );
    std::ifstream srcFile( srcPath );
    std::ofstream dstFile( dstPath, std::ios::out );

    if( srcFileSize != 0 && !srcFile.good() ) {
        errorPath = srcPath;
    }
    else if( !dstFile.good() ) {
        errorPath = dstPath;
    }

    if( ( srcFile.good() || srcFileSize == 0 ) && dstFile.good() ) {
        dstFile << srcFile.rdbuf();
        int dstFileSize = fileSize( dstPath );
        if( ( srcFileSize == 0 && dstFileSize == 0 ) || dstFile.good() ) {
            return true;
        }
        else {
            errorPath = dstPath;
        }
    }
    return false;
}

bool createSymbolicLink( const char* source, const char* linkName ) {
    if( !isValidPath( source ) || !isValidPath( linkName ) )
        return false;

    return symlink( source, linkName ) == 0;
}

bool createSymbolicLink( const std::string& source, const std::string& linkName ) {
    return createSymbolicLink( source.c_str(), linkName.c_str() );
}

bool copyFolder( const std::string& sourcePath, const std::string& destinationPath, ErrorReporter errorReporter ) {
    using namespace std;

    const std::string Prefix = "Copy folder: ";

    if( sourcePath.empty() || destinationPath.empty() || !isDirectoryExist( sourcePath ) ||
        !createFolder( destinationPath ) ) {
        if( errorReporter )
            if( sourcePath.empty() ) {
                errorReporter( errno, Prefix + "source path is empty", "" );
            }
            else if( destinationPath.empty() ) {
                errorReporter( errno, Prefix + "destination path is empty", "" );
            }
            else if( !isDirectoryExist( sourcePath ) ) {
                errorReporter( errno, Prefix + "directory not exist", sourcePath );
            }
            else if( !createFolder( destinationPath ) ) {
                errorReporter( errno, Prefix + "can't create folder", destinationPath );
            }

        return false;
    }

    struct dirent* sourceEntry = nullptr;
    auto sourceDir = openDirectory( sourcePath );
    if( !sourceDir ) {
        if( errorReporter )
            errorReporter( errno, Prefix + "can't open folder", sourcePath );
        return false;
    }

    while( ( sourceEntry = readdir( sourceDir.get() ) ) ) {
        if( isSystemFolders( *sourceEntry ) ) {
            continue;
        }

        const auto sourceFullPath = sourcePath + "/" + std::string( sourceEntry->d_name );
        const auto destinationFullPath = destinationPath + "/" + std::string( sourceEntry->d_name );

        if( isDirectoryExist( sourceFullPath ) ) {
            if( !copyFolder( sourceFullPath, destinationFullPath, nullptr ) ) {
                if( errorReporter )
                    errorReporter( errno, Prefix + "can't copy folder", sourceFullPath );
                return false;
            }
        }
        else if( isFileExist( sourceFullPath ) ) {
            std::string errorPath;
            if( !copyFile( sourceFullPath, destinationFullPath, errorPath ) ) {
                if( errorReporter )
                    errorReporter( errno, Prefix + "can't copy file", errorPath );
                return false;
            }
        }
    }

    return true;
}

bool iterateFileSystemEntry( const std::string& path, EntryHandler func ) {
    return iterateFileSystemEntry(
        path, [func]( const std::string& basePath, const std::string& entryName, unsigned char /*entryType*/ ) {
            return func( basePath, entryName );
        } );
}

bool iterateFileSystemEntry( const std::string& path, EntryHandler1 func ) {
    struct dirent* entry = nullptr;

    std::unique_ptr< DIR, int ( * )( DIR * dirp ) > dir( opendir( path.c_str() ), &closedir );
    if( !dir ) {
        return true;
    }

    while( entry = readdir( dir.get() ) ) {
        const auto isSystemFolders = ( strcmp( entry->d_name, "." ) == 0 ) || ( strcmp( entry->d_name, ".." ) == 0 );
        if( !isSystemFolders ) {
            std::string entryStr( entry->d_name );
            if( !func( path, entryStr, entry->d_type ) )
                return false;
        }
    }
    return true;
}

namespace {
thread_local RecursivelyEntryHandler currentRecursivelyEntryHandler;

int wrapperRecursivelyEntryHandler( const char* entryPath, const struct stat* status, int typeFlag, FTW* info ) {
    return currentRecursivelyEntryHandler( entryPath, status, typeFlag, info );
}
}  // namespace

bool iterateFileSystemEntryRecursively( const std::string& path,
                                        RecursivelyEntryHandler func,
                                        int flags,
                                        int numOpenFd ) {
    /* reserve fds for objects managed by the calling process */
    assert( numOpenFd < ( sysconf( _SC_OPEN_MAX ) - 50 ) );

    currentRecursivelyEntryHandler = func;
    return nftw( path.c_str(), wrapperRecursivelyEntryHandler, numOpenFd, flags ) != 0 ? false : true;
}

File::File getFilenameAndExtension( const std::string& strPath ) {
    auto extensionPos = strPath.find_last_of( '.' ) + 1;
    if( extensionPos == std::string::npos ) {
        throw Exception( "Wrong file format" );
    }
    auto filePos = strPath.find_last_of( '/' ) + 1;
    if( filePos == std::string::npos ) {
        throw Exception( "Wrong path format" );
    }
    auto filename = strPath.substr( filePos, extensionPos - filePos - 1 );
    auto extension = strPath.substr( extensionPos );
    return { filename, extension };
}

File::FullName getFilenameWithExtension( const std::string& strPath ) {
    auto filePos = strPath.find_last_of( '/' );
    if( filePos == std::string::npos ) {
        throw Exception( "Wrong path format" );
    }
    return strPath.substr( filePos + 1 );
}

std::string getPath( const std::string& filePath ) {
    auto filePos = filePath.find_last_of( '/' );
    if( filePos == std::string::npos ) {
        throw Exception( "Wrong path format" );
    }
    return filePath.substr( 0, filePos );
}

std::vector< std::string > findAllFiles( const std::string& folderPath ) {
    std::vector< std::string > filePaths;
    if( !isDirectoryExist( folderPath ) ) {
        throw Exception( "could not open directory" );
    }
    iterateFileSystemEntry(
        folderPath,
        [&filePaths]( const std::string& /*basePath*/, const std::string& entryName, unsigned char entryType ) {
            if( entryType == DT_REG ) {
                filePaths.push_back( entryName );
            }
            return true;
        } );
    return filePaths;
}

time_t fileModificationTime( const std::string& path ) {
    if( path.empty() || !filesystem::isFileExist( path ) )
        return 0;
    struct stat info = {};
    if( 0 != ::stat( path.c_str(), &info ) )
        return 0;
    return info.st_mtime;
}

uint64_t getFilesystemSize( const char* folder ) {
    if( !folder || !*folder )
        return 0;

    struct statvfs fiData = {};
    if( ( statvfs( folder, &fiData ) ) < 0 ) {
        SystemUtils::logError() << "Could not get information about filesystem '" << folder
                                << "', error: " << strerror( errno ) << '\n';
        throw Exception( "Could not get filesystem information: '" + std::string( folder ) +
                         "', error: " + std::string( std::strerror( errno ) ) );
    }
    return fiData.f_blocks * fiData.f_frsize;
}

std::vector< std::string > getDirectoryFileNames( const std::string& path ) {
    std::vector< std::string > fileList;
    DIR* dir = opendir( path.c_str() );
    if( dir == nullptr ) {
        return fileList;
    }

    struct dirent* entry;
    while( ( entry = readdir( dir ) ) != nullptr ) {
        std::string fileName( entry->d_name );
        if( fileName != "." && fileName != ".." ) {
            fileList.push_back( fileName );
        }
    }
    closedir( dir );
    return fileList;
}

uint64_t getFolderSize( const std::string& folderPath ) {
    uint64_t totalSize = 0;
    std::vector< std::string > filePaths = findAllFiles( folderPath );

    for( const std::string& filePath : filePaths ) {
        std::string fullPath = folderPath;
        fullPath.append( "/" ).append( filePath );
        totalSize += getFileSize( fullPath );
    }

    return totalSize;
}

}  // namespace filesystem
}  // namespace HubPlatform
