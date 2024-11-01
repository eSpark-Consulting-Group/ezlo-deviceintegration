#include "ram_usage.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/exception_utils.h"
#include <tiny/tiny_consolecolors.h>
#include <fnmatch.h>
#include <fstream>
#include <map>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <stddef.h>

//#include <mntent.h>	/* for getmntent(), et al. */

namespace HubPlatform {
namespace SystemUtils {
namespace ProcFS {

#include <mntent.h> /* for getmntent(), et al. */
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <dirent.h>

namespace {

long getEntry( const char* name, const char* buf ) {
    const char* hit;
    hit = strstr( buf, name );
    if( hit == nullptr ) {
        return -1;
    }

    errno = 0;
    long val = strtol( hit + strlen( name ), nullptr, 10 );
    if( errno != 0 ) {
        Log::error( cc::error( "get_entry: strtol() failed" ) );
        return -1;
    }
    return val;
}

/* Like get_entry(), but exit if the value cannot be found */
long getEntryFatal( const char* name, const char* buf ) {
    long val = getEntry( name, buf );
    if( val == -1 ) {
        Log::error( cc::error( "could not find entry '%s' in /proc/meminfo" ) );
    }
    return val;
}

long calculateAvailableGuesstimate( const char* buf ) {
    long cached = getEntryFatal( "Cached:", buf );
    long memFree = getEntryFatal( "MemFree:", buf );
    long buffers = getEntryFatal( "Buffers:", buf );
    long shmem = getEntryFatal( "Shmem:", buf );

    return ( memFree + cached + buffers - shmem );
}

}  // namespace

MemoryInfo::MemoryInfo()
: memTotalKib( 0 )
, memAvailableKib( 0 )
, memAvailablePercent( 0 ) {}

int MemoryInfo::parseMeminfo() {
    FILE* fd = nullptr;
    char buf[ 8192 ];
    int guesstimateWarned = 0;
    if( fd == nullptr )
        fd = fopen( "/proc/meminfo", "r" );
    if( fd == nullptr ) {
        Log::error( cc::error( "could not open /proc/meminfo: %s" ) );
        fclose( fd );
        return 0;
    }

    rewind( fd );

    size_t len = fread( buf, 1, sizeof( buf ) - 1, fd );
    if( len == 0 ) {
        Log::error( cc::error( "could not read /proc/meminfo: %s" ) );
        fclose( fd );
        return 0;
    }
    buf[ len ] = 0;  // Make sure buf is zero-terminated

    this->memTotalKib = getEntryFatal( "MemTotal:", buf );

    long memAvailable = getEntry( "MemAvailable:", buf );
    if( memAvailable == -1 ) {
        memAvailable = calculateAvailableGuesstimate( buf );
        if( guesstimateWarned == 0 ) {
            Log::error( cc::error( "Warning: Your kernel does not provide MemAvailable data (needs 3.14+)" ) );
            guesstimateWarned = 1;
        }
    }

    // Calculate percentages
    this->memAvailablePercent = static_cast< int >( memAvailable * 100 / this->memTotalKib );

    this->memAvailableKib = memAvailable;

    fclose( fd );
    return 1;
}

bool MemoryInfo::hasFlashInfo() {
    constexpr char filename[] = "/etc/mtab";
    struct mntent* fs;

    FILE* fp = setmntent( filename, "r" ); /* read only */
    if( fp == nullptr ) {
        Log::error( cc::error( "Could not open file for flash memory checking" ) );
        endmntent( fp );
        return false;
    }

    while( ( fs = getmntent( fp ) ) != nullptr )
        calculateVolumeSize( fs );

    endmntent( fp );
    return true;
}

/* calculateVolumeSize --- Use statvfs and fill info */

void MemoryInfo::calculateVolumeSize( const struct mntent* fs ) {
    struct statvfs vfs = {};

    if( fs->mnt_fsname[ 0 ] != '/' ) /* skip nonreal filesystems */
        return;

    if( statvfs( fs->mnt_dir, &vfs ) != 0 ) {
        Log::error( cc::error( "statvfs failed" ) );
        return;
    }

    auto convertCoefficient = 1024 * 1024;  // Bytes to Mb
    unsigned long totalFlash = 1.0f * vfs.f_blocks * vfs.f_frsize / convertCoefficient;
    unsigned long availableFlash = 1.0f * vfs.f_bavail * vfs.f_frsize / convertCoefficient;
    unsigned long freeFlash = 1.0f * vfs.f_bfree * vfs.f_frsize / convertCoefficient;
    unsigned long usedFlash = ( totalFlash ) - ( freeFlash );

    // Calculate % available flash rom based on f_bavail and f_blocks.
    unsigned long availableFlashPercent = ( vfs.f_bavail / (double) ( vfs.f_blocks ) * 100.0 );

    std::vector< unsigned long > flashUsage;

    flashUsage.push_back( totalFlash );
    flashUsage.push_back( availableFlash );
    flashUsage.push_back( freeFlash );
    flashUsage.push_back( usedFlash );
    flashUsage.push_back( availableFlashPercent );

    flashDirectories.insert(
        std::pair< std::string, std::vector< unsigned long > >( std::string( fs->mnt_fsname ), flashUsage ) );
}

std::map< std::string, std::string > MemoryInfo::parseProcStatus( std::istream& statusFile ) {
    std::map< std::string, std::string > statusMap;

    std::string line;
    while( std::getline( statusFile, line ) ) {
        std::istringstream iss( line );
        std::string key, value;
        if( std::getline( iss, key, ':' ) && std::getline( iss, value ) ) {
            statusMap[ key ] = value.erase( 0, 1 );
        }
    }
    return statusMap;
}

/** Fetches process status by pid - process identifier.
 * Filters out processes with memory usage lower than he limit or processes that doesn't have ha- prefix or
 *  an 'uninterruptible sleep' state.
 *  Places process status to an inner processList structure.
 *  @returns (status: bool, processInfo) - true if successfully fetched. false - if some error with fetching occurred.
 */
bool MemoryInfo::getProcessMemoryUsage( const char* pid, size_t filterOutLowMemoryUsageKb ) {
    bool fetchResult;
    ProcessStatus processStatus;
    std::tie( fetchResult, processStatus ) = fetchProcessMemoryUsage( pid );

    if( fetchResult ) {
        auto noLowMemoryProcess = processStatus.vmRss > filterOutLowMemoryUsageKb;

        if( processStatus.name.rfind( "ha-", 0 ) == 0 || processStatus.state.rfind( "D ", 0 ) == 0 ||
            processStatus.state.rfind( "R ", 0 ) == 0 || noLowMemoryProcess ) {
            this->processList.insert( { pid, processStatus } );
        }
    }

    return fetchResult;
}

std::tuple< bool, MemoryInfo::ProcessStatus > MemoryInfo::fetchProcessMemoryUsage( const char* pid ) {
    const std::string procPath = "/proc/" + std::string( pid ) + "/status";

    std::ifstream statusFile( procPath );

    if( !statusFile.is_open() ) {
        Log::error( cc::error( "Error opening status file for PID " ) + cc::error( pid ) );
        return std::make_tuple( false, ProcessStatus() );
    }

    return fetchProcessMemoryUsage( pid, statusFile );
}

std::tuple< bool, MemoryInfo::ProcessStatus > MemoryInfo::fetchProcessMemoryUsage( const char* pid,
                                                                                   std::istream& procStream ) {
    auto fields = parseProcStatus( procStream );
    auto findProperty = [&fields]( const std::string& key ) -> std::string {
        auto foundIt = fields.find( key );
        if( foundIt != fields.end() ) {
            return foundIt->second;
        }
        return "";
    };

    auto name = getProcessName( pid );
    auto data = findProperty( "VmData" );
    auto stack = findProperty( "VmStk" );
    auto rssAnon = findProperty( "RssAnon" );
    auto vmRss = findProperty( "VmRSS" );
    auto state = findProperty( "State" );

    ProcessStatus processStatus;
    processStatus.name = name;
    try {
        processStatus.memoryTotal = std::stoi( data ) + std::stoi( stack );
        processStatus.rssAnon = std::stoi( rssAnon );
        processStatus.vmRss = std::stoi( vmRss );
        processStatus.state = state;
        return std::make_tuple( true, processStatus );
    }
    catch( const std::invalid_argument& ex ) {
        Log::error( cc::error( "fetchProcessMemoryUsage: parsing error invalid argument: " ) + ex.what() );
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( "fetchProcessMemoryUsage: parsing error: " ) + CommonUtils::whatError( ex ) );
    }

    return std::make_tuple( false, MemoryInfo::ProcessStatus() );
}

void MemoryInfo::processDir( const struct dirent* dir ) {
    puts( dir->d_name );
    getProcessMemoryUsage( dir->d_name );
}

int filter( const struct dirent* dir ) {
    uid_t user;
    struct stat dirInfo = {};
    auto len = strlen( dir->d_name ) + 7;
    char path[ len ];

    strcpy( path, "/proc/" );
    strcat( path, dir->d_name );
    user = getuid();
    if( stat( path, &dirInfo ) < 0 ) {
        Log::error( cc::error( "processdir() ==> stat()" ) );
        return 0;
    }
    return !fnmatch( "[1-9]*", dir->d_name, 0 ) && user == dirInfo.st_uid;
}

bool MemoryInfo::hasProcessInfo() {
    struct dirent** namelist;
    int n = 0;

    n = scandir( "/proc", &namelist, filter, 0 );
    if( n < 0 ) {
        Log::error( cc::error( "Not enough memory." ) );
        return false;
    }
    else {
        while( n-- ) {
            processDir( namelist[ n ] );
            free( namelist[ n ] );
        }
        free( namelist );
    }
    return true;
}

std::string MemoryInfo::getProcessName( const std::string& pid ) {
    std::string path = "/proc/" + pid + "/exe";
    std::vector< char > buffer( 4096 );
    ssize_t len = readlink( path.c_str(), buffer.data(), buffer.size() - 1 );

    if( len != -1 ) {
        buffer[ len ] = '\0';
        std::string exePath( buffer.data() );
        size_t lastSlashPos = exePath.rfind( '/' );
        if( lastSlashPos != std::string::npos && lastSlashPos < exePath.size() - 1 ) {
            return exePath.substr( lastSlashPos + 1 );
        }
        return exePath;
    }

    return "";
}

}  // namespace ProcFS
}  // namespace SystemUtils
}  // namespace HubPlatform
