#include "uid/cluster_manager/controllers_discovery_service.h"
#include "uid/cluster_manager/avahi_utils.h"

#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

#include "system_utils/system.h"
#include "base_utils/utils.h"

namespace HubPlatform {
namespace Uid {

namespace {
constexpr auto ScanPeriod = std::chrono::seconds( 60 );
constexpr auto ScanCheckInterval = std::chrono::seconds( 10 );
constexpr unsigned MaxScanDuration = 15;

using ScanControllersSystemException = std::runtime_error;

}  // namespace

ControllersDiscoveryService::ControllersDiscoveryService( Services::TaskLoop& taskLoop,
                                                          IDbClient& dbClient,
                                                          InfoCallback onUpdated )
: m_taskLoop( taskLoop )
, m_dbClient( dbClient )
, m_isMonitoringTreadUp( false )
, m_onUpdated( std::move( onUpdated ) ) {}

void ControllersDiscoveryService::setUp() {
    m_isMonitoringTreadUp = true;
    m_monitoringThread = std::thread( &ControllersDiscoveryService::monitoringThread, this );
}

void ControllersDiscoveryService::tearDown() {
    m_isMonitoringTreadUp = false;
    if( m_monitoringThread.joinable() ) {
        m_monitoringThread.join();
    }
}

ControllersDiscoveryService::~ControllersDiscoveryService() {
    tearDown();
}

void ControllersDiscoveryService::monitoringThread() {
    while( true ) {
        try {
            if( waitForCondition(
                    [this]() { return !m_isMonitoringTreadUp.load(); }, ScanPeriod, ScanCheckInterval ) ) {
                break;
            }

            scanForControllers();
        }
        catch( const ScanControllersSystemException& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to scan LAN for ezlo controllers using mDNS, "
                                         "system error: " ) +
                              cc::note( ex.what() ) );
        }
        catch( const std::exception& ex ) {
            LOGGER_ERROR_RAW( cc::error( "Failed to scan LAN for ezlo controllers using mDNS, reason: " ) +
                              cc::note( ex.what() ) );
        }
    }
}

void ControllersDiscoveryService::scanForControllers() {
    Log::trace( cc::trace( "Scanning for neighbour controllers" ) );
    restartAvahiServiceIfItIsNotRunning();
    std::string output;
    int returnCode = 0;
    std::string bashCommand = "avahi-browse -lpktr _ezlo._tcp";

    bool useTimeout = true;
    if( useTimeout ) {
        // run command wrapped with `timeout DURATION cmd`, like `timeout 10s avahi-browse`
        bashCommand = System::formatTimeoutCommand( bashCommand, MaxScanDuration );
        Log::trace( cc::trace( "scanForControllers: using timeout, command: " ) + bashCommand );
    }
    std::tie( output, returnCode ) = System::getCommandOutput( bashCommand.c_str(), false, true );
    if( returnCode != 0 ) {
        throw ScanControllersSystemException( "avahi-browse command failed" );
    }

    auto newServices = AvahiUtils::parseAvahiBrowseOutput( output );

    std::map< std::string, Json > newInfoBySerial;
    for( auto& newService : newServices ) {
        auto serial = json_get_str( newService[ "records" ], "Serial" );
        if( serial.empty() )
            continue;

        auto& newInfo = newInfoBySerial[ serial ];
        if( newInfo.empty() ) {
            newInfo[ "_id" ] = serial;
            newInfo[ "services" ] = JSON_EMPTY_ARR;
        }
        newInfo[ "services" ].push_back( std::move( newService ) );
    }

    std::map< std::string, int > currentInfoIdBySerial;
    auto currentInfos = listControllersMdnsInfo();
    for( int i = 0; i < currentInfos.size(); ++i ) {
        auto& infoJson = currentInfos[ i ];
        auto serial = json_get_str( infoJson, "_id" );
        if( !serial.empty() ) {
            currentInfoIdBySerial.insert( std::make_pair( std::move( serial ), i ) );
        }
    }

    for( auto& newInfoPair : newInfoBySerial ) {
        auto& serial = newInfoPair.first;
        auto& newInfo = newInfoPair.second;
        if( currentInfoIdBySerial.count( serial ) == 0 ) {
            m_dbClient.insert( ControllersMdnsInfoDBName, newInfo );
            m_taskLoop.addTask( [this, newInfo]() { m_onUpdated( newInfo ); } );
        }
        else {
            if( newInfo != currentInfos[ currentInfoIdBySerial[ serial ] ] ) {
                m_dbClient.update( ControllersMdnsInfoDBName, JSON_EMPTY_OBJ, newInfo, true );
                m_taskLoop.addTask( [this, newInfo]() { m_onUpdated( newInfo ); } );
            }
            currentInfoIdBySerial.erase( serial );
        }
    }

    for( auto& currentInfoPair : currentInfoIdBySerial ) {
        auto newInfo = JSON( "_id" << currentInfoPair.first );
        m_taskLoop.addTask( [this, newInfo]() { m_onUpdated( newInfo ); } );
        m_dbClient.remove( ControllersMdnsInfoDBName, JSON( "_id" << currentInfoPair.first ) );
    }
}

void ControllersDiscoveryService::restartAvahiServiceIfItIsNotRunning() {
    // NOTE: sometimes avahi-daemon stuck registering service, as workaround, restart it in this case.
    if( System::getCommandOutput( "pgrep \"avahi-daemon\"", true ).second != 0 ) {
        Log::error( cc::error( "Service avahi-daemon not running, restarting..." ) );
#if !defined( __HUB_TYPE_LOCAL__ ) && !defined( __HUB_TYPE_VHUB__ )
        System::getCommandOutput( "/etc/init.d/avahi-daemon restart", true );
#endif
    }
}

Json ControllersDiscoveryService::getControllerMdnsInfo( const std::string& controllerId ) const {
    auto query = JSON( db::ID_FIELD_NAME << controllerId );
    auto info = m_dbClient.findOne( ControllersMdnsInfoDBName, query, nullptr );
    info.erase( db::ID_FIELD_NAME );
    return info;
}

std::vector< Json > ControllersDiscoveryService::listControllersMdnsInfo() const {
    return m_dbClient.findAll( ControllersMdnsInfoDBName, JSON_EMPTY_OBJ, nullptr );
}

}  // namespace Uid
}  // namespace HubPlatform
