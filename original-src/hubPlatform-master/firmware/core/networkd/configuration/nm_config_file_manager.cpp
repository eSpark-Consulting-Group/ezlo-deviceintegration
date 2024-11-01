#include "networkd/configuration/nm_config_file_manager.h"
#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"
#include "common_utils/exception_utils.h"

#include <iomanip>
#include <utility>

namespace HubPlatform {
namespace Network {

namespace {
const char* configFileName = "network";

const char* mobileSectionName = "mobile";

const int maxAmountOfMobileNetworks = 256;
const int maxAmountOfAPNSettings = 16;

namespace MobileFields {
const char* operatorsWhiteList = "operatorsWhiteList";
const char* autoRegistration = "autoRegistration";
const char* pingPeriodNotActive = "pingPeriodNotActive";
const char* pingPeriodActive = "pingPeriodActive";
const char* modemWatchdogTimer = "modemWatchdogTimer";
const char* finallyFailedToRegisterTimeout = "finallyFailedToRegisterTimeout";
const char* connectionRevivalAttemptTimeout = "connectionRevivalAttemptTimeout";
const char* modemMonitoringThreadPollingPeriod = "modemMonitoringThreadPollingPeriod";
const char* modemInfoPollingPeriod = "modemInfoPollingPeriod";

const char* apnSettings = "apnSettings";
const char* defaultApnSettings = "defaultApnSettings";
}  // namespace MobileFields

std::string joinPaths( const std::string& s1, const std::string& s2 ) {
    return s1 + "/" + s2;
}

int getInt( ConfigReader& reader, const std::string& path, int defaultValue ) {
    try {
        auto configPath = ConfigReader::Path( reader, path.c_str() );
        return reader.getInt( configPath );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) + cc::error( ", error: " ) +
                      cc::note( CommonUtils::whatError( ex ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) );
    }
    return defaultValue;
}

bool getBool( ConfigReader& reader, const std::string& path, bool defaultValue ) {
    try {
        auto configPath = ConfigReader::Path( reader, path.c_str() );
        return reader.getBool( configPath );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) + cc::error( ", error: " ) +
                      cc::note( CommonUtils::whatError( ex ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) );
    }
    return defaultValue;
}

std::vector< std::string > getStringArray( ConfigReader& reader,
                                           const std::string& path,
                                           const std::vector< std::string >& defaultValue ) {
    try {
        auto configPath = ConfigReader::Path( reader, path.c_str() );
        return reader.getStringArray( configPath );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) + cc::error( ", error: " ) +
                      cc::note( CommonUtils::whatError( ex ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) );
    }
    return defaultValue;
}

std::vector< std::string > getObjectKeys( ConfigReader& reader, const std::string& path ) {
    try {
        auto configPath = ConfigReader::Path( reader, path.c_str() );
        return reader.getObjectKeys( configPath );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) + cc::error( ", error: " ) +
                      cc::note( CommonUtils::whatError( ex ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Path not found: " ) + cc::note( path.c_str() ) );
    }
    return {};
}

std::chrono::seconds getSeconds( ConfigReader& reader, const std::string& path, int defaultValue ) {
    return std::chrono::seconds( getInt( reader, path, defaultValue ) );
}

std::string dumpStringArray( const std::vector< std::string >& array ) {
    if( array.empty() )
        return {};

    std::stringstream ss;
    auto it = array.cbegin();
    ss << *it++;

    for( ; it != array.cend(); ++it )
        ss << ", " << *it;
    return ss.str();
}

void dumpMobileConfiguration( NetworkConfigFileManager::MobileConfiguration& config ) {
    std::stringstream ss;

    auto formatDump = [&ss]( const std::string& key ) { ss << "....." << cc::note( key ) << '\n'; };
    auto formatKeyValueDump = [&ss]( const std::string& key, const std::string& value ) {
        ss << std::left << std::setfill( '.' ) << std::setw( 45 ) << cc::info( key );
        ss << "....." << cc::note( value ) << '\n';
    };

    ss << "Mobile Network Configuration\n";

    formatKeyValueDump( MobileFields::operatorsWhiteList, dumpStringArray( config.m_whiteOperatorsList ) );
    formatKeyValueDump( MobileFields::autoRegistration, cc::flag( config.m_autoRegistration ) );
    formatKeyValueDump( MobileFields::pingPeriodActive, cc::num10( config.m_pingPeriodActive.count() ) );
    formatKeyValueDump( MobileFields::pingPeriodNotActive, cc::num10( config.m_pingPeriodNotActive.count() ) );
    formatKeyValueDump( MobileFields::modemWatchdogTimer, cc::num10( config.m_modemWatchdogTimer.count() ) );
    formatKeyValueDump( MobileFields::finallyFailedToRegisterTimeout,
                        cc::num10( config.m_finallyFailedToRegisterTimeout.count() ) );
    formatKeyValueDump( MobileFields::connectionRevivalAttemptTimeout,
                        cc::num10( config.m_connectionRevivalAttemptTimeout.count() ) );
    formatKeyValueDump( MobileFields::modemMonitoringThreadPollingPeriod,
                        cc::num10( config.m_modemMonitoringThreadPollingPeriod.count() ) );
    formatKeyValueDump( MobileFields::modemInfoPollingPeriod, cc::num10( config.m_modemInfoPollingPeriod.count() ) );

    formatKeyValueDump( "APN Settings", cc::num10( config.m_APNSettings.size() ) );
    for( auto& apn : config.m_APNSettings ) {
        formatKeyValueDump( "....." + apn.first, dumpStringArray( apn.second ) );
    }

    formatKeyValueDump( "Default APN Settings", cc::num10( config.m_defaultAPNSettings.size() ) );
    for( auto& apn : config.m_defaultAPNSettings ) {
        formatDump( apn );
    }

    auto configInfo = ss.str();
    if( !configInfo.empty() && configInfo.back() == '\n' )
        configInfo.pop_back();
    Log::info( configInfo );
}

}  // namespace

NetworkConfigFileManager::NetworkConfigFileManager()
: m_configReader( m_defaultReader ) {
    init();
}

NetworkConfigFileManager::NetworkConfigFileManager( ConfigReader& readerInstance )
: m_configReader( readerInstance ) {
    init();
}

void NetworkConfigFileManager::init() {
    try {
        constructMobileConfiguration();
    }
    catch( const std::exception& ex ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to parse network configuration file: " ) +
                         cc::warn( CommonUtils::whatError( ex ) ) );
    }
    catch( ... ) {
        LOGGER_WARN_RAW( cc::warn( "Failed to parse network configuration file: unknown error" ) );
    }
}

void NetworkConfigFileManager::constructMobileConfiguration() {
    const std::string mobileSectionPath = joinPaths( configFileName, mobileSectionName );
    m_mobileConfiguration.m_whiteOperatorsList =
        getStringArray( m_configReader, joinPaths( mobileSectionPath, MobileFields::operatorsWhiteList ), {} );
    m_mobileConfiguration.m_autoRegistration =
        getBool( m_configReader, joinPaths( mobileSectionPath, MobileFields::autoRegistration ), true );
    m_mobileConfiguration.m_pingPeriodActive =
        getSeconds( m_configReader, joinPaths( mobileSectionPath, MobileFields::pingPeriodActive ), 30 );
    m_mobileConfiguration.m_pingPeriodNotActive =
        getSeconds( m_configReader, joinPaths( mobileSectionPath, MobileFields::pingPeriodNotActive ), 900 );
    m_mobileConfiguration.m_modemWatchdogTimer =
        getSeconds( m_configReader, joinPaths( mobileSectionPath, MobileFields::modemWatchdogTimer ), 300 );
    m_mobileConfiguration.m_finallyFailedToRegisterTimeout =
        getSeconds( m_configReader, joinPaths( mobileSectionPath, MobileFields::finallyFailedToRegisterTimeout ), 300 );
    m_mobileConfiguration.m_connectionRevivalAttemptTimeout = getSeconds(
        m_configReader, joinPaths( mobileSectionPath, MobileFields::connectionRevivalAttemptTimeout ), 600 );
    m_mobileConfiguration.m_modemMonitoringThreadPollingPeriod = getSeconds(
        m_configReader, joinPaths( mobileSectionPath, MobileFields::modemMonitoringThreadPollingPeriod ), 300 );
    m_mobileConfiguration.m_modemInfoPollingPeriod =
        getSeconds( m_configReader, joinPaths( mobileSectionPath, MobileFields::modemInfoPollingPeriod ), 180 );

    const auto apnSettingsPath = joinPaths( mobileSectionPath, MobileFields::apnSettings );
    const auto apnSettingsKeys = getObjectKeys( m_configReader, apnSettingsPath );
    if( apnSettingsKeys.size() > maxAmountOfMobileNetworks ) {
        LOGGER_WARN_RAW( cc::warn( "apn settings contain too much operators" ) );
    }

    for( const auto& key : apnSettingsKeys ) {
        auto path = joinPaths( apnSettingsPath, key );
        m_mobileConfiguration.m_APNSettings.emplace(
            std::make_pair( key, getStringArray( m_configReader, path, {} ) ) );
    }

    m_mobileConfiguration.m_defaultAPNSettings = getStringArray(
        m_configReader, joinPaths( mobileSectionPath, MobileFields::defaultApnSettings ), { "internet" } );

    dumpMobileConfiguration( m_mobileConfiguration );
}

const NetworkConfigFileManager::MobileConfiguration& NetworkConfigFileManager::getMobileConfiguration() const {
    return m_mobileConfiguration;
}

}  // namespace Network
}  // namespace HubPlatform
