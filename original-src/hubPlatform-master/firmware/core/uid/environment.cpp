#include "database_client/db_tables_names.h"
#include "uid/environment.h"

#include "log_utils/ezlo_log.h"
#include "config_reader/config_keys.h"
#include "system_utils/filesystem.h"
#include "system_utils/environment.h"
#include "common_utils/curl_wrapper.h"
#include "provision_data_utils/provision_data_manager.h"
#include "vera_utils/provision_reader.h"
#include "database_client/idbclient.h"

#include "uid/ezlo.h"

#include "tiny/tiny_utils.h"
#include "tiny/tiny_consolecolors.h"

#include "uid/service_settings.h"
#include "log_utils/logsettings.h"

namespace HubPlatform {
namespace Uid {
namespace {
constexpr char CloudCacheInfo[] = "cache-cloud-info";
constexpr char CachedNmaUrl[] = "cache-nma-url";
constexpr char CachedProvisioningRequested[] = "cache-provisioning-synced";
}  // namespace

Environment::Environment( ServiceSettings& settings, IDbClient& dbClient, PrivateTag )
: m_settings( settings )
, m_dbClient( dbClient ) {
    m_provisionData.set( ProvisionDataUtils::getManager().getProvisionData() );

    try {
        if( m_configReader.hasValue( HubPlatform::ConfigKeys::LogPathKey ) ) {
            ezlo::log_settings::setLogDirectory( m_configReader.getString( HubPlatform::ConfigKeys::LogPathKey ) );
        }
    }
    catch( const ConfigReader::ReadingError& error ) {
        const std::string message = cc::error( "Failed to find the '" ) +
                                    cc::warn( HubPlatform::ConfigKeys::LogPathKey ) + cc::error( "' : " ) +
                                    cc::warn( error.what() );

        Log::fatal( message );
    }

    updateNmaUrlFromCache();
}

bool Environment::hasProvisionInformation() const {
    return m_provisionData.process( []( const ProvisionDataUtils::ProvisionData& data ) { return data.isValid(); } );
}

void Environment::restart( std::chrono::seconds waitBeforeRestart ) const {
    if( waitBeforeRestart.count() ) {
        Log::info( "Restarting in " + std::to_string( waitBeforeRestart.count() ) + " seconds..." );
        std::this_thread::sleep_for( waitBeforeRestart );
    }
    Log::info( "Restarting..." );
    while( system(
        ( HubPlatform::Environment::getBinariesFolder() + "/updatecmd --daemon --restart_firmware" ).c_str() ) ) {
        constexpr std::chrono::seconds attemptsTimeout( 5 );
        std::this_thread::sleep_for( attemptsTimeout );
    }
    constexpr std::chrono::seconds restartTimeout( 60 );
    std::this_thread::sleep_for( restartTimeout );
}

void Environment::reboot( std::chrono::seconds waitBeforeReboot ) const {
    if( waitBeforeReboot.count() ) {
        Log::info( "Rebooting in " + std::to_string( waitBeforeReboot.count() ) + " seconds..." );
        std::this_thread::sleep_for( waitBeforeReboot );
    }

#if defined( __HUB_TYPE_LOCAL__ ) || defined( __HUB_TYPE_VHUB__ )
    Log::warn( cc::warn( "reboot: local configuration, reboot skipped" ) );
#else
    Log::info( "Sending logs before reboot..." );

    std::string logCmd = HubPlatform::Environment::getBinariesFolder() + "/logcmd";
    logCmd += " --add-to-name=reboot-command";

    if( std::system( ( logCmd + " --idle" ).c_str() ) ) {
        Log::warn( "Unable to send logs" );
        Log::info( "Storing logs..." );
        std::system( ( logCmd + " --store-to-postponed" ).c_str() );
    }

    Log::info( "Rebooting..." );
    while( system( "reboot -f" ) ) {
        constexpr std::chrono::seconds attemptsTimeout( 5 );
        std::this_thread::sleep_for( attemptsTimeout );
    }
#endif
    constexpr std::chrono::minutes rebootTimeout( 5 );
    std::this_thread::sleep_for( rebootTimeout );
}

bool Environment::requestProvisionInformation() {
    if( hasProvisionInformation() )
        return true;

    using namespace HubPlatform;
    HubPlatform::VeraProvision::ProvisionReader reader;
    reader.readAll();
    auto serialNumber = reader.getSerial();
    auto hwKey = reader.getHwKey();
    auto macAddress = tiny::tools::to_lower( reader.getMacAddressString() );

#ifdef _DEBUG
    Log::debug( cc::debug( "Original serial number: " ) + cc::note( serialNumber ) );
    Log::debug( cc::debug( "Original controller key: " ) + cc::note( hwKey ) );
    Log::debug( cc::debug( "Original mac address: " ) + cc::note( macAddress ) );
#endif

    constexpr auto timeoutBeforeRestart = std::chrono::minutes( 5 );

    if( serialNumber.empty() || hwKey.empty() || macAddress.empty() ) {
        Log::error( cc::error( "Have not enough data to request new controller information" ) );
#if defined( __HUB_TYPE_G150__ )
        reboot( timeoutBeforeRestart );
#endif
        return false;
    }

    static const std::string reprovisionServer = "req-disp.mios.com";
    auto requestLink = "https://" + reprovisionServer + "/api/v1/controller/sync-legacy";
    CurlWrapper::Headers headers = { "accept: application/json", "Content-Type: application/json" };

    JsonObject body = JSON( "serial_no" << serialNumber << "mac_address" << macAddress << "hw_key" << hwKey );

    constexpr std::uint8_t maxReprovisionConnectionAttempts = 10;
    std::chrono::seconds timeout( 10 );
    long statusCode;
    std::string newProvisionString;
    auto startTime = std::chrono::steady_clock::now();
    for( int attempt = 1; attempt <= maxReprovisionConnectionAttempts; ++attempt ) {
        Log::info( cc::info( "[" ) + cc::note( std::to_string( attempt ) ) + cc::info( "/" ) +
                   cc::note( std::to_string( maxReprovisionConnectionAttempts ) ) +
                   cc::info( "] Getting new controller information" ) );

        auto connectingToReprovisionServerTimeout = startTime + ( timeout + timeout * attempt ) * attempt / 2;
        statusCode = CurlWrapper::curlCustomPost(
            requestLink, headers, "", "", "", body.dump(), newProvisionString, timeout.count() );

        if( statusCode == 200 ) {
            break;
        }

        auto errorString = newProvisionString.empty() ? std::to_string( statusCode ) : newProvisionString;
        Log::error( cc::warn( "Failed to get new controller information: " ) + cc::error( errorString ) );

        if( attempt == maxReprovisionConnectionAttempts ) {
            reboot( timeoutBeforeRestart );
            return false;
        }

        std::this_thread::sleep_until( connectingToReprovisionServerTimeout );
    }

    if( newProvisionString.empty() ) {
        Log::info( cc::warn( "No new controller information available" ) );
        reboot( timeoutBeforeRestart );
        return false;
    }

    JsonObject newProvisionJson;
    if( !parseJson( newProvisionString, newProvisionJson ) ) {
        Log::error( cc::error( "Received new controller information has incorrect format" ) );
#ifdef _DEBUG
        Log::debug( cc::debug( "New controller data: " ) + cc::note( newProvisionString ) );
#endif
        reboot( timeoutBeforeRestart );
        return false;
    }

#ifdef _DEBUG
    Log::debug( cc::debug( "New controller json: " ) + cc::json( newProvisionJson ) );
#endif

    Log::info( cc::info( "Got new controller information" ) );
    const auto isProvisioningSaved = m_provisionData.process(
        [this, &timeoutBeforeRestart, &newProvisionJson]( ProvisionDataUtils::ProvisionData& data ) {
            data.fillFromJson( newProvisionJson );

            if( !data.isValid() ) {
                Log::error( cc::error( "Received new controller information is invalid" ) );
                reboot( timeoutBeforeRestart );
                return false;
            }

            if( !ProvisionDataUtils::getManager().init( data ) ) {
                Log::error( cc::error( "Unable to apply new controller information" ) );
                reboot( timeoutBeforeRestart );
                return false;
            }

            return true;
        } );

    if( isProvisioningSaved ) {
        Log::info( cc::info( "New controller information has been saved" ) );
        restart();
        return true;
    }

    return false;
}

ProvisionDataUtils::UpdateStatus Environment::requestNewProvisionInformation() {
    auto provisionServerUrl = getProvisionServerURL();
    auto provisionToken = getProvisionToken();
    auto provisionVersion = getProvisionVersion();

#ifdef _DEBUG
    Log::debug( cc::debug( "Controller server: " ) + cc::note( provisionServerUrl ) );
    Log::debug( cc::debug( "Controller token: " ) + cc::note( provisionToken ) );
    Log::debug( cc::debug( "Controller data version: " ) + cc::num10( provisionVersion ) );
#endif

    if( provisionServerUrl.empty() || provisionToken.empty() || provisionVersion == 0 ) {
        Log::error( cc::error( "Have not enough data to request controller information" ) );
        return ProvisionDataUtils::UpdateStatus::Failed;
    }

    auto requestLink = provisionServerUrl + "/api/v1/controller/sync?version=" + std::to_string( provisionVersion );
    CurlWrapper::Headers headers = { "controller-key: " + provisionToken };

    constexpr std::uint8_t maxProvisionConnectionAttempts = 3;
    constexpr std::chrono::seconds timeout( 5 );

    long statusCode;
    std::string newProvisionString;
    auto startTime = std::chrono::steady_clock::now();
    for( int attempt = 1; attempt <= maxProvisionConnectionAttempts; ++attempt ) {
        Log::info( cc::info( "[" ) + cc::note( std::to_string( attempt ) ) + cc::info( "/" ) +
                   cc::note( std::to_string( maxProvisionConnectionAttempts ) ) +
                   cc::info( "] Getting controller information" ) );

        auto connectingToProvisionServerTimeout = startTime + timeout * attempt;
        statusCode = CurlWrapper::curlCustomPost( requestLink,
                                                  headers,
                                                  getCertificationChainFile(),
                                                  getCertificateFile(),
                                                  getPrivateKeyFile(),
                                                  "",
                                                  newProvisionString,
                                                  timeout.count() );

        if( statusCode == 200 || statusCode == 304 ) {
            break;
        }

        auto errorString = newProvisionString.empty() ? std::to_string( statusCode ) : newProvisionString;
        Log::error( cc::warn( "Failed to get controller information: " ) + cc::error( errorString ) );

        if( attempt == maxProvisionConnectionAttempts ) {
            return ProvisionDataUtils::UpdateStatus::Failed;
        }

        std::this_thread::sleep_until( connectingToProvisionServerTimeout );
    }

    if( statusCode == 304 || newProvisionString.empty() ) {
        Log::info( cc::warn( "No new controller information available" ) );
        return ProvisionDataUtils::UpdateStatus::NothingRequired;
    }

    JsonObject newProvisionJson;
    if( !parseJson( newProvisionString, newProvisionJson ) ) {
        Log::error( cc::error( "Received controller information has incorrect format" ) );
#ifdef _DEBUG
        Log::debug( cc::debug( "Controller data: " ) + cc::note( newProvisionString ) );
#endif
        return ProvisionDataUtils::UpdateStatus::Failed;
    }

#ifdef _DEBUG
    Log::debug( cc::debug( "Controller json: " ) + cc::json( newProvisionJson ) );
#endif

    Log::info( cc::info( "Got new controller information" ) );
    ProvisionDataUtils::ProvisionData provisionData;
    provisionData.fillFromJson( newProvisionJson );

    return updateProvisioningData( provisionData );
}

ProvisionDataUtils::UpdateStatus Environment::updateProvisioningData(
    const ProvisionDataUtils::ProvisionData& provisionData ) {
    if( !provisionData.isValid() ) {
        Log::error( cc::error( "Received controller information is invalid" ) );
        return ProvisionDataUtils::UpdateStatus::Failed;
    }

    auto updateStatus = ProvisionDataUtils::UpdateStatus::Failed;

    {
        const std::lock_guard< std::mutex > updateLock( m_provisioningRequestMutex );
        updateStatus = ProvisionDataUtils::getManager().update( provisionData );
        if( updateStatus != ProvisionDataUtils::UpdateStatus::Failed ) {
            m_provisionData.set( provisionData );
        }
    }

    switch( updateStatus ) {
    case ProvisionDataUtils::UpdateStatus::RestartRequired:
        Log::info( cc::info( "Controller will be" ) + cc::note( "rebooted" ) +
                   cc::info( " after applying new controller information." ) );
        break;

    case ProvisionDataUtils::UpdateStatus::ReconnectRequired:
        Log::info( cc::info( "Cloud connection will be " ) + cc::note( "reinitialized" ) +
                   cc::info( " after applying new controller information." ) );
        invalidateNmaServerURL();
        break;

    case ProvisionDataUtils::UpdateStatus::NothingRequired:
        Log::debug( cc::note( "Nothing" ) + cc::debug( " required after applying new controller information." ) );
        break;

    case ProvisionDataUtils::UpdateStatus::Failed:
        Log::error( cc::error( "Unable to apply new controller information" ) );
        break;
    };

    return updateStatus;
}

std::string Environment::requestCoordinatorForNewNMAUrl( const std::string& cloudCoordinatorURL ) {
    std::string nmaServerURL;

    if( cloudCoordinatorURL.empty() ) {
        LOGGER_DEBUG( cc::debug( "Request new provision information" ) );
        if( requestNewProvisionInformation() == ProvisionDataUtils::UpdateStatus::RestartRequired ) {
            restart();
        }
        return "";
    }

    std::string requestLink = ( cloudCoordinatorURL.back() == '/' ) ? cloudCoordinatorURL : cloudCoordinatorURL + "/";
    requestLink += "getserver?reinit=1&type=ezlolinux";

    std::uint8_t attempt = 0;
    constexpr std::uint8_t maxNmaConnectionAttempts = 20;
    constexpr std::uint8_t getRequestTimeout = 15;
    constexpr std::chrono::seconds connectingToCoordinatorTimeout( 5 );

    do {
        if( attempt >= maxNmaConnectionAttempts ) {
            LOGGER_DEBUG( cc::debug( "Request new provision information again" ) );
            if( requestNewProvisionInformation() == ProvisionDataUtils::UpdateStatus::RestartRequired ) {
                restart();
            }
            return "";
        }

        Log::info( cc::info( "[" ) + cc::note( std::to_string( attempt ) ) + cc::info( "/" ) +
                   cc::note( std::to_string( maxNmaConnectionAttempts ) ) +
                   cc::info( "] Connecting to cloud coordinator at " ) + cc::u( requestLink ) );

        if( CurlWrapper::curlGet( requestLink,
                                  getCertificationChainFile(),
                                  getCertificateFile(),
                                  getPrivateKeyFile(),
                                  nmaServerURL,
                                  getRequestTimeout ) ) {
            try {
                tiny::url newServerUrl( nmaServerURL );
                if( newServerUrl.scheme() != "ws" && newServerUrl.scheme() != "wss" ) {
                    Log::warn( cc::warn( "Unsupported NMA scheme: " ) + cc::u( nmaServerURL ) );
                    nmaServerURL.clear();
                    std::this_thread::sleep_for( connectingToCoordinatorTimeout );
                }
            }
            catch( ... ) {
                Log::warn( cc::warn( "Unsupported NMA URL format: " ) + cc::u( nmaServerURL ) );
                nmaServerURL.clear();
                std::this_thread::sleep_for( connectingToCoordinatorTimeout );
            }
        }
        else {
            Log::warn( cc::warn( "Failed to init curl connection to cloud coordinator: " ) +
                       cc::debug( nmaServerURL ) );
            nmaServerURL.clear();
            std::this_thread::sleep_for( connectingToCoordinatorTimeout );
        }

        ++attempt;

    } while( nmaServerURL.empty() );

    Log::debug( cc::debug( "Coordinator said: " ) + cc::u( nmaServerURL ) );

    return nmaServerURL;
}

EnvironmentPtr Environment::create( ServiceSettings& settings, IDbClient& dbClient ) {
    return std::make_shared< Environment >( settings, dbClient, PrivateTag{} );
}

void Environment::fillValueIfFileExist( const char* key, std::string& value ) const {
    try {
        if( m_configReader.hasValue( key ) ) {
            value = m_configReader.getString( key );
            if( !HubPlatform::filesystem::isFileExist( value ) ) {
                value.clear();
            }
        }
    }
    catch( const ConfigReader::ReadingError& error ) {
        const std::string message =
            cc::error( "Failed to find the '" ) + cc::warn( key ) + cc::error( "' : " ) + cc::warn( error.what() );

        Log::fatal( message );
    }
}

std::string Environment::getCertificateFile() const {
    std::string certificatePath = m_settings.getCertificateFile();
    if( certificatePath.empty() ) {
        certificatePath = ::HubPlatform::Environment::getCertificateFolder() + "/cloud.crt";
    }

    return certificatePath;
}
std::string Environment::getPrivateKeyFile() const {
    std::string certificatePath = m_settings.getPrivateKeyFile();
    if( certificatePath.empty() ) {
        certificatePath = ::HubPlatform::Environment::getCertificateFolder() + "/cloud.key";
    }

    return certificatePath;
}

std::string Environment::getCertificationChainFile() const {
    std::string certificatePath = m_settings.getCertificationChainFile();
    if( certificatePath.empty() ) {
        certificatePath = ::HubPlatform::Environment::getCertificateFolder() + "/cloud.ca";
    }

    return certificatePath;
}

std::string Environment::getProvisionServerURL() const {
    std::string url;
    m_provisionData.process( [&url]( const ProvisionDataUtils::ProvisionData& data ) { url = data.provisionServer; } );

    if( !url.empty() ) {
        if( url.back() == '/' ) {
            url.pop_back();
        }
        return url;
    }

    Log::fatal( cc::error( "Failed to find the " ) + cc::warn( "controller server url" ) );
    return "";
}

std::string Environment::getProvisionToken() const {
    std::string provisionToken;
    m_provisionData.process(
        [&provisionToken]( const ProvisionDataUtils::ProvisionData& data ) { provisionToken = data.provisionToken; } );

    if( !provisionToken.empty() ) {
        return provisionToken;
    }

    Log::fatal( cc::error( "Failed to find the " ) + cc::warn( "controller token" ) );
    return "";
}

int Environment::getProvisionVersion() const {
    auto configVersion = m_provisionData.process( []( const ProvisionDataUtils::ProvisionData& data ) {
        if( data.isValid() )
            return data.configVersion;

        return 0;
    } );

    if( configVersion <= 0 ) {
        Log::fatal( cc::error( "Failed to find the " ) + cc::warn( "controller data version" ) );
    }

    return configVersion;
}

std::string Environment::getCloudCoordinatorURL() const {
    std::string coordinatorUrl = m_settings.getExplicitCloudCoordinatorURL();

    if( coordinatorUrl.empty() ) {
        try {
            if( getConfigReader().hasValue( ConfigKeys::Coordinator ) ) {
                coordinatorUrl = getConfigReader().getString( ConfigKeys::Coordinator );
            }
            else {
                m_provisionData.process( [&coordinatorUrl]( const ProvisionDataUtils::ProvisionData& data ) {
                    if( !data.cloudServer.empty() ) {
                        coordinatorUrl = data.cloudServer;
                    }
                } );
            }
        }
        catch( const ConfigReader::ReadingError& ) {
            std::string message = cc::error( "Failed to find the " ) + cc::warn( "coordinator" ) +
                                  cc::error( " variable in the path  " ) + cc::warn( ConfigKeys::Coordinator );

            Log::fatal( message );
        }
    }

    return coordinatorUrl;
}

namespace {
time_t getTimeByPath( const HubPlatform::ConfigReader& configReader, const char* path, time_t defaultTime ) {
    try {
        if( configReader.hasValue( path ) ) {
            const auto time = static_cast< time_t >( std::stoll( configReader.getString( path ) ) );
            return time <= 0 ? defaultTime : time;
        }
    }
    catch( const ConfigReader::ReadingError& error ) {
        Log::warn( std::string( "Unexpected config reader error: " ) + error.what() );
    }

    return defaultTime;
}
}  // namespace

time_t Environment::getCloudCoordinatorQueryTimeout() const {
    return getTimeByPath(
        getConfigReader(), ConfigKeys::CloudTimeout, m_settings.getExplicitCloudCoordinatorQueryTimeoutInSeconds() );
}

time_t Environment::getCloudCoordinatorReconnectPeriod() const {
    return getTimeByPath( getConfigReader(),
                          ConfigKeys::CloudReconnectPeriod,
                          m_settings.getExplicitCloudCoordinatorReconnectPeriodInSeconds() );
}

bool Environment::isUrlCacheEnabled() const {
    return !m_configReader.hasValue( ConfigKeys::NmaUrlCacheOn ) || m_configReader.getBool( ConfigKeys::NmaUrlCacheOn );
}

bool Environment::wasProvisioningRequestedAfterStart() {
    auto state = m_settings.getProvisioningDataState();
    if( state == ServiceSettings::ProvisioningState::Unknown ) {
        bool isProvisioningRequested = false;
        auto joCacheCloud = m_dbClient.findOne( HaTempCacheDBName, JSON( CloudCacheInfo << 1 ), nullptr );
        if( !joCacheCloud.empty() ) {
            isProvisioningRequested = getFieldSafe< bool >( joCacheCloud, CachedProvisioningRequested, false );
        }

        state = isProvisioningRequested ? ServiceSettings::ProvisioningState::Requested :
                                          ServiceSettings::ProvisioningState::NotRequested;
        m_settings.setProvisioningDataState( state );
    }

    return state == ServiceSettings::ProvisioningState::Requested;
}

void Environment::markProvisioningAsRequested() {
    auto cacheCloud = m_dbClient.findOne( HaTempCacheDBName, JSON( CloudCacheInfo << 1 ), nullptr );
    if( cacheCloud.empty() ) {
        m_dbClient.insert( HaTempCacheDBName, JSON( CloudCacheInfo << 1 << CachedProvisioningRequested << true ) );
    }
    else {
        m_dbClient.update( HaTempCacheDBName,
                           JSON( CloudCacheInfo << 1 ),
                           JSON( "$set" << JSON( CachedProvisioningRequested << true ) ) );
    }

    m_settings.setProvisioningDataState( ServiceSettings::ProvisioningState::Requested );
}

std::string Environment::askCloudCoordinatorForNmaServerURL( std::string& cloudCoordinatorURL ) {
    std::string serverUrl;

    try {
        if( !cloudCoordinatorURL.empty() ) {
            serverUrl = requestCoordinatorForNewNMAUrl( cloudCoordinatorURL );
            if( !serverUrl.empty() && isUrlCacheEnabled() ) {
                Log::debug( cc::debug( "Update cloud cache: " ) + cc::u( serverUrl ) );
                auto cacheCloud = m_dbClient.findOne( HaTempCacheDBName, JSON( CloudCacheInfo << 1 ), nullptr );
                if( cacheCloud.empty() ) {
                    m_dbClient.insert( HaTempCacheDBName, JSON( CloudCacheInfo << 1 << CachedNmaUrl << serverUrl ) );
                }
                else {
                    m_dbClient.update( HaTempCacheDBName,
                                       JSON( CloudCacheInfo << 1 ),
                                       JSON( "$set" << JSON( CachedNmaUrl << serverUrl ) ) );
                }
            }
        }
    }
    catch( ... ) {
        Log::warn( cc::warn( "Unsupported NMA scheme: " ) + cc::u( serverUrl ) );
        serverUrl.clear();
    }

    return serverUrl;
}

std::string Environment::askCloudCoordinatorForNmaServerURL() {
    auto coordinator_url = getCloudCoordinatorURL();
    return askCloudCoordinatorForNmaServerURL( coordinator_url );
}

std::string Environment::getNmaServerURL( bool enableAskCloudCoordinator ) {
    std::string serverUrl = m_settings.getExplicitNmaServerURL();

    if( serverUrl.empty() ) {
        try {
            if( getConfigReader().hasValue( ConfigKeys::NmaUrl ) ) {
                serverUrl = getConfigReader().getString( ConfigKeys::NmaUrl );
            }
        }
        catch( const ConfigReader::ReadingError& ) {
            const std::string errorMessage = cc::error( "Failed to find the " ) + cc::warn( "url" ) +
                                             cc::error( " variable in the " ) + cc::warn( ConfigKeys::NmaUrl );

            Log::info( errorMessage );
        }

        if( serverUrl.empty() && enableAskCloudCoordinator ) {
            if( !wasProvisioningRequestedAfterStart() ) {
                LOGGER_DEBUG( cc::debug( "Request new provision information" ) );
                if( requestNewProvisionInformation() != ProvisionDataUtils::UpdateStatus::Failed ) {
                    markProvisioningAsRequested();
                }
            }

            serverUrl = askCloudCoordinatorForNmaServerURL();
            m_settings.setExplicitNmaServerURL( serverUrl );
        }
    }

    return serverUrl;
}

bool Environment::isStopEzloHubEmotionsOnShutdown() const {
    try {
        if( getConfigReader().hasValue( ConfigKeys::StopEmotionsOnShutdown ) ) {
            return getConfigReader().getBool( ConfigKeys::StopEmotionsOnShutdown );
        }
    }
    catch( const ConfigReader::ReadingError& ) {
    }

    return false;
}

const ConfigReader& Environment::getConfigReader() const {
    return m_configReader;
}

void Environment::setExplicitCloudCoordinatorURL( const std::string& explicitCloudCoordinatorURL ) {
    m_settings.setExplicitCloudCoordinatorURL( explicitCloudCoordinatorURL );
}

void Environment::setExplicitNmaServerURL( const std::string& explicitNmaServerURL ) {
    m_settings.setExplicitNmaServerURL( explicitNmaServerURL );
}

std::string Environment::getExplicitCloudCoordinatorURL() const {
    return m_settings.getExplicitCloudCoordinatorURL();
}

time_t Environment::getExplicitCloudCoordinatorQueryTimeoutInSeconds() const {
    return m_settings.getExplicitCloudCoordinatorQueryTimeoutInSeconds();
}

time_t Environment::getExplicitCloudCoordinatorReconnectPeriodInSeconds() const {
    return m_settings.getExplicitCloudCoordinatorReconnectPeriodInSeconds();
}

void Environment::invalidateNmaServerURL() {
    setExplicitNmaServerURL( "" );
    m_dbClient.update( HaTempCacheDBName, JSON( CloudCacheInfo << 1 ), JSON( "$set" << JSON( CachedNmaUrl << "" ) ) );
}

void Environment::updateNmaUrlFromCache() {
    if( getExplicitNmaServerURL().empty() ) {
        try {
            if( getConfigReader().hasValue( ConfigKeys::NmaUrl ) ) {
                setExplicitNmaServerURL( getConfigReader().getString( ConfigKeys::NmaUrl ) );
                return;
            }
        }
        catch( const ConfigReader::ReadingError& ) {
            const std::string errorMessage = cc::error( "Failed to find the " ) + cc::warn( "url" ) +
                                             cc::error( " variable in the " ) + cc::warn( ConfigKeys::NmaUrl );
            Log::error( errorMessage );
        }

        auto printInfoMessageAboutIgnoringCache = []() {
            Log::info( cc::notice( "Notice" ) + cc::info( ", " ) + cc::note( CloudCacheInfo ) +
                       cc::info( " from DB is " ) + cc::error( "ignored" ) );
        };

        if( isUrlCacheEnabled() ) {
            std::string cachedNmaURL;
            auto joCacheCloud = m_dbClient.findOne( HaTempCacheDBName, JSON( CloudCacheInfo << 1 ), nullptr );
            if( !joCacheCloud.empty() ) {
                cachedNmaURL = getFieldSafe< std::string >( joCacheCloud, CachedNmaUrl );
            }

            if( !cachedNmaURL.empty() ) {
                Log::info( cc::success( "Success" ) + cc::info( ", have " ) + cc::note( CloudCacheInfo ) +
                           cc::info( ", cached NMA URL" ) + cc::info( " is " ) + cc::u( cachedNmaURL ) );
                setExplicitNmaServerURL( cachedNmaURL );
            }
            else {
                printInfoMessageAboutIgnoringCache();
            }
        }
        else {
            printInfoMessageAboutIgnoringCache();
        }
    }
    else {
        Log::info( cc::notice( "Notice" ) + cc::info( ", " ) + cc::note( CloudCacheInfo ) + cc::info( " from DB is " ) +
                   cc::error( "ignored" ) );
    }
}

std::string Environment::getExplicitNmaServerURL() const {
    return m_settings.getExplicitNmaServerURL();
}

size_t Environment::getNmaClientRxBufferSize() const {
    // TODO: efwr-1927: replace this with actual value later
    // Magic number taken from tiny_ws.cpp client_api::init(), which rewrites buffer_size_rx
    //    size for clients.
    return 100 * 1024;
}

}  // namespace Uid
}  // namespace HubPlatform
