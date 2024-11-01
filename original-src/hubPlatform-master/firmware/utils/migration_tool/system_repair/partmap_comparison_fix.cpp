#include "migration_tool/system_repair/partmap_comparison_fix.h"

#include "tiny/tiny_consolecolors.h"
#include "base_utils/md5.h"
#include "system_utils/filesystem.h"
#include "log_utils/ezlo_log.h"

#include <fstream>

namespace HubPlatform {
namespace MigrationTool {

namespace {

static std::string FileToPatch = "/lib/upgrade/common.sh";
static std::string TmpFile = "/tmp/lib_upgrade_common.sh";
static std::string InitialHash = "B8B12411236607DF5DA29BC3A5572A94";
static std::string FixedHash = "9784BA7A3CEF4A505740B2DA053195FB";
}  // namespace

const char* PartmapComparisonFix::getName() const {
    return "partmap comparison fix";
}

bool PartmapComparisonFix::shouldApply() const {
    auto fileHash = getFileHash( FileToPatch );

    if( fileHash == InitialHash ) {
        Log::debug( cc::debug( "File " ) + cc::note( FileToPatch ) + cc::debug( " is eligible for fix (hash is " ) +
                    cc::note( fileHash ) + cc::debug( ")" ) );
        return true;
    }

    if( fileHash == FixedHash ) {
        Log::debug( cc::debug( "File " ) + cc::note( FileToPatch ) + cc::debug( " is already fixed (hash is " ) +
                    cc::note( fileHash ) + cc::debug( ")" ) );
        return false;
    }

    Log::debug( cc::debug( "File " ) + cc::note( FileToPatch ) + cc::debug( " have unsupported version (hash is " ) +
                cc::note( fileHash ) + cc::debug( ")" ) );
    return false;
}

bool PartmapComparisonFix::apply() {
    bool patched = false;

    {
        std::ifstream fileToPatch( FileToPatch );
        std::ofstream tmpFile( TmpFile );

        std::string line;
        while( std::getline( fileToPatch, line ) ) {
            if( line == "\t\tlocal part" ) {
                // remove this line
            }
            else if( line == "\t\tfor part in 1 2 3 4; do" ) {
                tmpFile << "\t\tlocal parts=\"1 2 3\"\n"
                           "\t\t[ -f /tmp/sysupgrade.brick.device ] && parts=\"1 2 3 4\"\n"
                           "\n"
                           "\t\tlocal part\n"
                           "\t\tfor part in $parts; do\n";
                patched = true;
            }
            else {
                tmpFile << line << "\n";
            }
        }
        tmpFile.flush();
    }

    if( !patched ) {
        Log::error( cc::error( "Failed to patch " ) + cc::note( FileToPatch ) +
                    cc::error( " file: unsupported version" ) );
        return false;
    }

    std::ifstream tmpFile( TmpFile );
    Md5 hashSum;
    hashSum.append( tmpFile );
    auto fileHash = hashSum.getHex();
    if( fileHash != FixedHash ) {
        Log::error( cc::error( "Failed to patch " ) + cc::note( FileToPatch ) + cc::error( " file: internal error" ) );
        return false;
    }

    if( !safeCopyFile( TmpFile, FileToPatch ) ) {
        Log::error( cc::error( "Failed to patch " ) + cc::note( FileToPatch ) +
                    cc::error( " file: error when overriding" ) );
        return false;
    }

    return true;
}

std::string PartmapComparisonFix::getFileHash( const std::string& fileName ) const {
    std::ifstream file( fileName, std::ios::binary );
    Md5 hashSum;
    hashSum.append( file );
    return hashSum.getHex();
}

bool PartmapComparisonFix::safeCopyFile( const std::string& fromFile, const std::string& toFile ) {
    auto backupFile = toFile + ".bak";
    if( !filesystem::copyFile( toFile, backupFile ) ) {
        return false;
    }
    if( !filesystem::copyFile( fromFile, toFile ) ) {
        filesystem::removeFile( fromFile.c_str() );
        filesystem::copyFile( backupFile, toFile );
        filesystem::removeFile( backupFile.c_str() );
        return false;
    }
    filesystem::removeFile( fromFile.c_str() );
    filesystem::removeFile( backupFile.c_str() );
    return true;
}

}  // namespace MigrationTool
}  // namespace HubPlatform
