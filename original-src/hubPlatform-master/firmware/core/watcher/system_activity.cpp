#include "system_activity.h"
#include "resources/watcher_resources.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "system_utils/environment.h"
#include "system_utils/filesystem.h"
#include "system_utils/system.h"
#include "ubus_interface/ubus_impl.h"
#include "system_utils/timezone/openwrt_timezone.h"

#include <fstream>
#include <sys/statvfs.h>

namespace HubPlatform {
SystemActivity::SystemActivity() {
    m_uBusObject = std::make_unique< Ubus::UBusImpl >();
    m_uBusObject->connect( nullptr );

    handleRebootStatus();
}

void SystemActivity::clearSystemInfo() {
    m_sysInfo.fileSystemInfo.clear();

    memset( &m_sysInfo.memInfo, 0, sizeof( m_sysInfo.memInfo ) );
    memset( &m_sysInfo.sysLoad, 0, sizeof( m_sysInfo.sysLoad ) );
    memset( &m_sysInfo.timeInfo, 0, sizeof( m_sysInfo.timeInfo ) );
}

void SystemActivity::collectMemory() {
    std::string memInfo;
    std::string line, column1, column2;
    std::stringstream memInfoStream;
    std::istringstream iss;
    uint memoryBuffers;
    uint memoryCached;
    uint byteUnit = 1024;

    memInfo = HubPlatform::System::readFile( "/proc/meminfo" );
    memInfoStream.str( memInfo );

    while( std::getline( memInfoStream, line ) ) {
        if( line.find( "MemTotal" ) != std::string::npos ) {
            iss.str( line );
            iss >> column1 >> column2;
            m_sysInfo.memInfo.totalBytes = std::stoi( column2 ) * byteUnit;
        }
        if( line.find( "MemFree" ) != std::string::npos ) {
            iss.str( line );
            iss >> column1 >> column2;
            m_sysInfo.memInfo.freeBytes = std::stoi( column2 ) * byteUnit;
        }

        if( line.find( "Buffers" ) != std::string::npos ) {
            iss.str( line );
            iss >> column1 >> column2;
            memoryBuffers = std::stoi( column2 ) * byteUnit;
        }
        if( line.find( "Cached" ) != std::string::npos ) {
            iss.str( line );
            iss >> column1 >> column2;
            if( column1.compare( "Cached:" ) == 0 ) {
                memoryCached = std::stoi( column2 ) * byteUnit;
            }
        }

        if( line.find( "MemAvailable" ) != std::string::npos ) {
            iss.str( line );
            iss >> column1 >> column2;
            m_sysInfo.memInfo.availableBytes = std::stoi( column2 ) * byteUnit;
        }
    }

    m_sysInfo.memInfo.cachedBytes = memoryBuffers + memoryCached;
}

void SystemActivity::collectLoad() {
    JsonObject val;
    JsonObject jsonResponse;
    std::string response;

    float f_load = 1.F / ( 1 << SI_LOAD_SHIFT );

    m_uBusObject->sendCommand(
        Watcher::Resources::SystemActivity::system, Watcher::Resources::SystemActivity::info, "", "", response );

    parseJson( response, jsonResponse );
    if( !json_get_arr( jsonResponse, Watcher::Resources::SystemActivity::load, val ) ) {
        LOGGER_DEBUG( "Could not get load information!" );
        return;
    }
    if( val.size() != 3 ) {
        LOGGER_DEBUG( "System load could not obtain correctly!" );
        return;
    }
    json_get( val[ 0 ], m_sysInfo.sysLoad.load1 );
    json_get( val[ 1 ], m_sysInfo.sysLoad.load5 );
    json_get( val[ 2 ], m_sysInfo.sysLoad.load15 );

    m_sysInfo.sysLoad.load1 = f_load * m_sysInfo.sysLoad.load1;
    m_sysInfo.sysLoad.load5 = f_load * m_sysInfo.sysLoad.load5;
    m_sysInfo.sysLoad.load15 = f_load * m_sysInfo.sysLoad.load15;
}

void SystemActivity::collectTime() {
    time_t rawtime = 0;
    struct tm* ptm = nullptr;
    std::string timezone;
    time( &rawtime );

    std::string response;
    m_uBusObject->sendCommand(
        Watcher::Resources::SystemActivity::system, Watcher::Resources::SystemActivity::info, "", "", response );

    JsonObject jsonResponse;
    parseJson( response, jsonResponse );
    json_get( jsonResponse, Watcher::Resources::SystemActivity::uptime, m_sysInfo.timeInfo.uptime );
    try {
        timezone = system_utils::OpenWrtTimeZone().getTimeZone();
        m_sysInfo.timeInfo.timezoneIdentifier = timezone;
    }
    catch( const std::exception& e ) {
        LOGGER_DEBUG( cc::debug( "Could not get OpenWrtTimeZone() information, reason: " ) + cc::note( e.what() ) );
    }
    ptm = localtime( &rawtime );
    m_sysInfo.timeInfo.timezone = ptm->tm_gmtoff;

    m_sysInfo.timeInfo.utctime = rawtime - m_sysInfo.timeInfo.timezone;

    char buffer[ 6 ];
    strftime( buffer, sizeof( buffer ), "%z", ptm );
    m_sysInfo.timeInfo.timezoneOffset = buffer;
    m_sysInfo.timeInfo.ntpLastSync =
        filesystem::fileModificationTime( HubPlatform::Environment::getTempDataFolder() + "/" + ntpSyncFile );
}

void SystemActivity::fillFileSystemInfo( const std::string& mountPoint, FileSystemInfo& fileSystemInfo ) {
    struct statvfs stat = { 0 };

    if( statvfs( mountPoint.c_str(), &stat ) == -1 ) {
        return;
    }
    fileSystemInfo.mountPoint = mountPoint;

    fileSystemInfo.totalBytes = static_cast< uint64_t >( stat.f_blocks ) * stat.f_frsize;
    fileSystemInfo.freeBytes = static_cast< uint64_t >( stat.f_bavail ) * stat.f_bsize;
    if( fileSystemInfo.totalBytes >= fileSystemInfo.freeBytes ) {
        fileSystemInfo.usedBytes = fileSystemInfo.totalBytes - fileSystemInfo.freeBytes;
    }
    else {
        fileSystemInfo.usedBytes = 0;
        LOGGER_WARN( cc::warn( "Invalid filesystem sizes: total bytes < free bytes" ) );
    }
}

void SystemActivity::collectFileSystem() {
    FileSystemInfo fileSystemInfo;

    std::string file = HubPlatform::System::readFile( mountsPath );
    std::stringstream fileStream;
    std::string line;
    std::string mountsColumn1;
    std::string mountsColumn2;
    std::istringstream iss;

    fileStream.str( file );

    while( std::getline( fileStream, line ) ) {
        iss.str( line );
        iss >> mountsColumn1 >> mountsColumn2;

        if( !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludeProc ) != std::string::npos ) &&
            !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludeMnt ) != std::string::npos ) &&
            !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludeSys ) != std::string::npos ) &&
            !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludeTmp ) != std::string::npos ) &&
            !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludeRom ) != std::string::npos ) &&
            !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludedev ) != std::string::npos ) &&
            !( mountsColumn2.find( Watcher::Resources::SystemActivity::excludeOverlay ) != std::string::npos ) ) {
            fillFileSystemInfo( mountsColumn2, fileSystemInfo );
            m_sysInfo.fileSystemInfo.push_back( fileSystemInfo );
        }
    }
}

void SystemActivity::collectSystemInfo() {
    clearSystemInfo();
    collectMemory();
    collectLoad();
    collectTime();
    collectFileSystem();
    collectNetwork();
}

void SystemActivity::prepareSystemReportData( std::string& systemReportData ) {
    JsonObject systemJson;
    JsonObject tmpNetworkJson;
    JsonObject tmpFilesysJson;

    systemJson[ "Memory" ] = JSON( Watcher::Resources::SystemActivity::memTotal
                                   << m_sysInfo.memInfo.totalBytes << Watcher::Resources::SystemActivity::memFree
                                   << m_sysInfo.memInfo.freeBytes << Watcher::Resources::SystemActivity::available
                                   << m_sysInfo.memInfo.availableBytes << Watcher::Resources::SystemActivity::cached
                                   << m_sysInfo.memInfo.cachedBytes );

    systemJson[ "Time" ] =
        JSON( Watcher::Resources::SystemActivity::timezone
              << m_sysInfo.timeInfo.timezone << Watcher::Resources::SystemActivity::timezoneIdentifier
              << m_sysInfo.timeInfo.timezoneIdentifier << Watcher::Resources::SystemActivity::uptime
              << m_sysInfo.timeInfo.uptime << Watcher::Resources::SystemActivity::utctime << m_sysInfo.timeInfo.utctime
              << Watcher::Resources::SystemActivity::timezoneOffset << m_sysInfo.timeInfo.timezoneOffset
              << Watcher::Resources::SystemActivity::ntpLastSync << m_sysInfo.timeInfo.ntpLastSync );

    for( const auto& disk : m_sysInfo.fileSystemInfo ) {
        // WARN: ubus does json convertion to it's own implementation that does not support 64bit integers
        tmpFilesysJson[ disk.mountPoint ] =
            JSON( Watcher::Resources::SystemActivity::fileSysTotalSpace
                  << std::to_string( disk.totalBytes ) << Watcher::Resources::SystemActivity::fileSysUsedSpace
                  << std::to_string( disk.usedBytes ) << Watcher::Resources::SystemActivity::fileSysAvailabledSpace
                  << std::to_string( disk.freeBytes ) );
    }
    systemJson[ "FileSystem" ] = tmpFilesysJson;

    systemJson[ "LoadAverage" ] =
        JSON( Watcher::Resources::SystemActivity::load1
              << m_sysInfo.sysLoad.load1 << Watcher::Resources::SystemActivity::load5 << m_sysInfo.sysLoad.load5
              << Watcher::Resources::SystemActivity::load15 << m_sysInfo.sysLoad.load15 );

    systemJson[ "oom_kill" ] = getKillCount();
    systemJson[ "reboot_count" ] = m_hubRebootCount;

    for( const auto& net : m_networkInfo ) {
        tmpNetworkJson[ net.interfaceName ] =
            JSON( Watcher::Resources::SystemActivity::networkRXCount
                  << net.rxCount << Watcher::Resources::SystemActivity::networkTXCount << net.txCount
                  << Watcher::Resources::SystemActivity::networkRXError << net.rxError
                  << Watcher::Resources::SystemActivity::networkTXError << net.txError
                  << Watcher::Resources::SystemActivity::networkRXBytes << net.rxBytes
                  << Watcher::Resources::SystemActivity::networkTXBytes << net.txBytes
                  << Watcher::Resources::SystemActivity::deviceName << net.deviceName
                  << Watcher::Resources::SystemActivity::networkInterfaceStatus << net.status
                  << Watcher::Resources::SystemActivity::uptime << net.uptime );
    }

    systemReportData = JSON( "System" << systemJson << "Network" << tmpNetworkJson ).dump();
}

void SystemActivity::getSystemReportData( std::string& systemReportData ) {
    collectSystemInfo();
    prepareSystemReportData( systemReportData );
}

void SystemActivity::collectInterfaceNames( std::vector< NetworkInfo >& networkInfo ) {
    std::string tmpInterfaceName;
    std::string tmpDeviceName;
    std::string responseString;

    JsonObject responseJson;
    JsonObject interfaceArray;

    m_uBusObject->sendCommand( Watcher::Resources::SystemActivity::networkInterface,
                               Watcher::Resources::SystemActivity::dump,
                               "",
                               "",
                               responseString );
    parseJson( responseString, responseJson );

    if( !json_get_arr( responseJson, Watcher::Resources::SystemActivity::interface, interfaceArray ) ) {
        LOGGER_DEBUG( "Could not get interface information from uBus" );
        return;
    }

    for( const auto& interface : interfaceArray ) {
        bool interfaceStatus = false;
        struct NetworkInfo tmpInterfaceInfo;
        if( json_get( interface, Watcher::Resources::SystemActivity::device, tmpDeviceName ) ) {
            tmpInterfaceInfo.deviceName = tmpDeviceName;
        }
        if( json_get( interface, Watcher::Resources::SystemActivity::interface, tmpInterfaceName ) ) {
            tmpInterfaceInfo.interfaceName = tmpInterfaceName;
        }

        if( json_get( interface, Watcher::Resources::SystemActivity::networkUp, interfaceStatus ) ) {
            if( interfaceStatus ) {
                tmpInterfaceInfo.status = true;
            }
        }
        int64_t interfaceUptime = json_get_int64( interface, Watcher::Resources::SystemActivity::networkUptime, -1 );
        if( interfaceUptime > -1 ) {
            tmpInterfaceInfo.uptime = interfaceUptime;
        }
        networkInfo.emplace_back( tmpInterfaceInfo );
    }
}

void SystemActivity::collectInterfaceStatus( NetworkInfo& netInf ) {
    JsonObject responseJson;
    JsonObject statisticsJson;
    std::string responseString;

    m_uBusObject->sendCommand( Watcher::Resources::SystemActivity::networkDevice,
                               Watcher::Resources::SystemActivity::status,
                               Watcher::Resources::SystemActivity::name,
                               netInf.deviceName,
                               responseString );
    parseJson( responseString, responseJson );

    if( !json_get( responseJson, Watcher::Resources::SystemActivity::statistics, statisticsJson ) ) {
        LOGGER_DEBUG( "Could not get network interface statistics from uBus: " + netInf.deviceName );
        return;
    }

    if( !json_get( statisticsJson, Watcher::Resources::SystemActivity::networkRXPackets, netInf.rxCount ) ) {
        LOGGER_DEBUG( "Could not get rx_packets from json" );
    }
    if( !json_get( statisticsJson, Watcher::Resources::SystemActivity::networkRXBytes, netInf.rxBytes ) ) {
        LOGGER_DEBUG( "Could not get rx_bytes from json" );
    }
    if( !json_get( statisticsJson, Watcher::Resources::SystemActivity::networkRXError, netInf.rxError ) ) {
        LOGGER_DEBUG( "Could not get rx_errors from json" );
    }

    if( !json_get( statisticsJson, Watcher::Resources::SystemActivity::networkTXPackets, netInf.txCount ) ) {
        LOGGER_DEBUG( "Could not get tx_packets from json" );
    }
    if( !json_get( statisticsJson, Watcher::Resources::SystemActivity::networkTXBytes, netInf.txBytes ) ) {
        LOGGER_DEBUG( "Could not get tx_bytes from json" );
    }
    if( !json_get( statisticsJson, Watcher::Resources::SystemActivity::networkTXError, netInf.txError ) ) {
        LOGGER_DEBUG( "Could not get tx_errors from json" );
    }
}

uint SystemActivity::getKillCount() {
    std::string tmpKill;
    uint killCount = 0;
    size_t startOffset = 0;
    std::ifstream vmstatFile( vmstatPath );

    if( !vmstatFile.is_open() ) {
        LOGGER_DEBUG( std::string( "Could not open " ).append( vmstatPath ) );
        return 0;
    }
    while( getline( vmstatFile, tmpKill ) ) {
        if( tmpKill.find( "oom_kill" ) != std::string::npos ) {
            break;
        }
    }
    vmstatFile.close();

    if( tmpKill.empty() ) {
        return 0;
    }

    startOffset = tmpKill.find( ' ' );
    killCount = std::atoi( tmpKill.substr( startOffset + 1, tmpKill.length() - startOffset ).c_str() );

    return killCount;
}

void SystemActivity::collectNetwork() {
    m_networkInfo.clear();
    collectInterfaceNames( m_networkInfo );

    for( auto& val : m_networkInfo ) {
        // TODO: Add status change check for interface status to prevent continuous logging
        /**
         * While calling **ubus call network.device status '{"name":"val.devicename"}'** command with down interface
         * ubus returns empty statics
         **/
        if( val.status == true && val.deviceName.compare( "na" ) != 0 ) {
            collectInterfaceStatus( val );
        }
    }
}

void SystemActivity::setRebootCount() {
    std::string rebootFilePath = Environment::getDataFolder() + m_rebootFile;
    std::string rebootFileString = HubPlatform::System::readFile( rebootFilePath );
    std::stringstream rebootFileStream;

    rebootFileStream.str( rebootFileString );

    if( !rebootFileStream.str().empty() ) {
        rebootFileStream >> m_hubRebootCount;
        m_hubRebootCount++;
    }

    std::ofstream rebootFile( m_rebootFile );
    rebootFile << m_hubRebootCount;
    rebootFile.close();
}

void SystemActivity::handleRebootStatus() {
    std::string ubusResponse;
    JsonObject ubusResponseJson;
    JsonObject instanceJson;
    int respawnCount = 0;

    m_uBusObject->sendCommand(
        Watcher::Resources::ServiceActivity::service, Watcher::Resources::ServiceActivity::list, "", "", ubusResponse );
    parseJson( ubusResponse, ubusResponseJson );

    if( json_get( ubusResponseJson,
                  { m_serviceName,
                    Watcher::Resources::ServiceActivity::instances,
                    Watcher::Resources::ServiceActivity::instance1,
                    Watcher::Resources::ServiceActivity::respawncount },
                  respawnCount ) ) {
        if( respawnCount == 0 ) {
            setRebootCount();
        }
    }
}

}  // namespace HubPlatform
