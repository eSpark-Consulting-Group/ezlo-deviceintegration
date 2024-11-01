#include "hardware_manager/diagnostics_dumper.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/mount_point.h"
#include "log_utils/ezlo_log.h"

#include "tiny/include/tiny/tiny_consolecolors.h"

#include <ctime>

namespace HubPlatform {
namespace HardwareManager {

namespace {

static const std::string MountPoint = "/mnt/diagnostics";
static const std::string DiagnosticLogsDirectory = "/var/log/diagnostics";
static const std::string UsbDriveDev = "/dev/sda1";
static const std::string DiagnosticsDirectoryPrefix = "diagnostics";
static const std::string CoresArchiveName = "cores.tgz";
static const std::string LogsArchiveName = "logs.tgz";

#if( defined( __HUB_TYPE_G150__ ) )
bool dumpG150( const std::string& archivePath ) {
    Log::info( cc::info( "Dumping diagnostic info to usb drive" ) );

    std::unique_ptr< filesystem::MountPoint > mountPoint;
    try {
        mountPoint = filesystem::MountPoint::mount( UsbDriveDev, MountPoint );
    }
    catch( filesystem::MountPointError& e ) {
        Log::error( cc::error( "Can not dump diagnostic info: " + std::string( e.what() ) ) );
        return false;
    }

    if( filesystem::isExist( archivePath ) ) {
        Log::warn( cc::warn( "Archive with diagnostics already exist, removing..." ) );
        if( filesystem::isDirectoryExist( archivePath ) ) {
            filesystem::deleteFolder( archivePath );
        }
        else {
            filesystem::removeFile( archivePath.c_str() );
        }
    }

    if( filesystem::isDirectoryExist( DiagnosticLogsDirectory ) ) {
        Log::warn( cc::warn( "Directory for diagnostic logs already exist, removing..." ) );
        filesystem::deleteFolder( DiagnosticLogsDirectory );
    }

    if( !filesystem::createFolder( DiagnosticLogsDirectory ) ) {
        Log::error( cc::error( "Can not create directory for diagnostic logs" ) );
        return false;
    }

    bool error = false;

    error |= system( ( "ifconfig > " + DiagnosticLogsDirectory + "/ifconfig.log 2>&1" ).c_str() );
    error |= system( ( "iwconfig > " + DiagnosticLogsDirectory + "/iwconfig.log 2>&1" ).c_str() );
    error |= system( ( "ip address show > " + DiagnosticLogsDirectory + "/ip-address-show.log 2>&1" ).c_str() );
    error |= system( ( "ip route list > " + DiagnosticLogsDirectory + "/ip-route-list.log 2>&1" ).c_str() );
    error |=
        system( ( "ubus call network.interface dump > " + DiagnosticLogsDirectory + "/network-interface-dump.log 2>&1" )
                    .c_str() );
    error |= system( ( "ping -c 4 8.8.8.8 > " + DiagnosticLogsDirectory + "/ping-8.8.8.8.log 2>&1" ).c_str() );
    error |=
        system( ( "ping -c 4 209.160.32.104 > " + DiagnosticLogsDirectory + "/ping-209.160.32.104.log 2>&1" ).c_str() );
    error |= system(
        ( "ping -c 4 req-disp.mios.com > " + DiagnosticLogsDirectory + "/ping-req-disp.mios.com.log 2>&1" ).c_str() );
    error |= system( ( "traceroute 8.8.8.8 > " + DiagnosticLogsDirectory + "/traceroute-8.8.8.8.log 2>&1" ).c_str() );
    error |= system(
        ( "traceroute 209.160.32.104 > " + DiagnosticLogsDirectory + "/traceroute-209.160.32.104.log 2>&1" ).c_str() );
    error |= system( ( "mount > " + DiagnosticLogsDirectory + "/mount.log 2>&1" ).c_str() );

    error |= system( ( "tar -zcf" + archivePath + " " + DiagnosticLogsDirectory + " " + Environment::getLogsFolder() +
                       " " + Environment::getPersistentFolder() + "/cores" )
                         .c_str() );

    filesystem::deleteFolder( DiagnosticLogsDirectory );
    Log::info( cc::info( "Finished dumping diagnostics info" ) );

    return !error;
}
#endif

}  // namespace

std::string DiagnosticsDumper::generateDiagnosticsArchivePath() const {
    auto timestamp = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    auto local = localtime( &timestamp );
    char timeStr[ 32 ];
    strftime( timeStr, sizeof( timeStr ), "%Y%m%dT%H%M%S", local );
    return MountPoint + "/" + DiagnosticsDirectoryPrefix + "-" + timeStr + ".bin";
}

bool DiagnosticsDumper::dump() {
#if( defined( __HUB_TYPE_G150__ ) )
    return dumpG150( generateDiagnosticsArchivePath() );
#endif
    return true;
}

}  // namespace HardwareManager
}  // namespace HubPlatform
