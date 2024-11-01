#include "uid/ui_service.h"

#include "uid/stat_handlers.h"
#include "uid/broadcast.h"
#include "uid/answers.h"
#include "uid/mdns_config/mdns_config.h"
#include "uid/handlers/hub_offline_commands_handler.h"
#include "uid/rpc_message_filters.h"
#include "cloud_storage/cloud_storage.h"
#include "uid/camera_addon_handler.h"
#include "http_bridge/ws_api_bridge.h"
#include "http_bridge/taskloop_handlers.h"
#include "http_bridge/cameras_addon_endpoint.h"

#include "uid/environment.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"

#include "service_interfaces/groups_names.h"

#include "version/ha_version.h"
#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

#include "base_utils/ezlodb.h"
#include "config_reader/config_keys.h"

#include "service_interfaces/bus_wrapper.h"
#include "networkd/remote_interface/network_remote_interface.h"
#include "provision_data_utils/provision_data_manager.h"

#include "uid/ui_proto.h"
#include "uid/nma_proto.h"

#include "system_utils/environment.h"
#include "system_utils/filesystem.h"

#include "uid/nma_client.h"
#include "uid/server/server_peer_messager.h"
#include "log_utils/logsettings.h"
#include "uid/ezlo.h"

#include "ssl_utils/x509.h"
#include "ui_reporter.h"
#include "common_utils/firmware_info_helper.h"

#include <tiny/tiny_url.h>
#include <tiny/tiny_network.h>
#include <tiny/tiny_utils.h>
#include <tiny/tiny_multithreading.h>
#include <tiny/tiny_dispatch.h>
#include <tiny/tiny_command_line_parser.h>
#include "base_utils/timeutils.h"
#include "database_client/db_flags_names.h"
#include <functional>

extern "C" {
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <openssl/conf.h>
#include <openssl/ssl.h>
#if !defined( OPENSSL_NO_ENGINE )
#include <openssl/engine.h>
#endif  // !defined(OPENSSL_NO_ENGINE)
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>
}

namespace HubPlatform {
namespace Uid {

namespace AccessKeysSync {
const auto RequestCertificatesInterval = std::chrono::hours( 48 );
const auto CheckCertificatesInterval = std::chrono::minutes( 10 );
const int SecondsBeforeCertificatesForceChange =
    2 * std::chrono::duration_cast< std::chrono::seconds >( CheckCertificatesInterval ).count();
const auto AccessKeysRepeatAfterError = std::chrono::minutes( 20 );
}  // namespace AccessKeysSync

namespace {
constexpr std::chrono::hours ProvisioningReaskInterval( 24 );

const std::vector< std::string > BackupTables = {
    RegisteredControllersDBName,
    SysFlagsDBName,
    DbVersionsDBName /*Actually migration_tool is responsible for this table, but lets backup it here*/,
    HaFavoriteDBName,
    UiExtensionsHttpLinksDBName,
    HubErrorsDBName

};

}  // namespace

Service* Service::m_instance = nullptr;

Json Service::CustomService::toJson() const {
    return JSON( "name" << m_name << "requestDomain" << m_requestDomain << "spreadServiceName" << m_spreadServiceName
                        << "requestTimeout" << m_requestTimeout.count() );
}

static const std::string& getLibWebSocketLogPrefix() {
    static std::string g_str( cc::debug( "LIBWS" ) );
    return g_str;
}

Service& Service::getInstance() {
    if( m_instance == nullptr ) {
        throw std::runtime_error( "UI Service is not created!" );
    }

    return *m_instance;
}

Service::Service()
: Services::Service( GroupsNames::UI_GROUP )
, m_iccManager( *this )
, m_clusterManager( *this, ProvisionDataUtils::getManager().getProvisionData() )
, m_uiExtensionsManager( *this, HttpBridge::getApiBridge() )
, m_cloudStorage( std::make_unique< CloudStorage >( *this ) ) {
    setInstance( this );
    m_cloudStorage->setInternetAvailable( false );
}

Service::Service( std::unique_ptr< Services::BusWrapper > busWrapper, std::unique_ptr< IDbClient > dbClient )
: Services::Service( GroupsNames::UI_GROUP, std::move( busWrapper ), std::move( dbClient ) )
, m_iccManager( *this )
, m_clusterManager( *this, ProvisionDataUtils::getManager().getProvisionData() )
, m_uiExtensionsManager( *this, HttpBridge::getApiBridge() )
, m_cloudStorage( std::make_unique< CloudStorage >( *this ) ) {
    setInstance( this );
    m_cloudStorage->setInternetAvailable( false );
}

Service::~Service() {
    setInstance( nullptr );
}

void Service::setInstance( Service* instance ) {
    if( instance != nullptr && m_instance != nullptr ) {
        throw std::runtime_error( "UI Service has been already created!" );
    }

    m_instance = instance;
}

void Service::defineDaemonParameters( tiny::command_line::parser& parser ) {
    m_settings.defineCommandLineParameters( parser );
}

void Service::printFirmwareVersion() {
    bool cc_was_on = cc::_on_;
    cc::_on_ = Log::getLocalColors();

    std::string strCanonicalHostName = tiny::network::get_canonical_host_name();
    size_t cntCPUs = tiny::tools::cpu_count();
    std::string strCloudCoordinatorURL = m_environment->getCloudCoordinatorURL();
    time_t ttCloudCoordinatorQueryTimeoutInSeconds = m_environment->getCloudCoordinatorQueryTimeout();
    time_t ttCloudCoordinatorReconnectPeriodInSeconds = m_environment->getCloudCoordinatorReconnectPeriod();
    std::string strNmaServerURL = m_environment->getNmaServerURL( false );
    std::stringstream ss;
    ss << std::endl
       << cc::info( "Linux Firmware Version" ) << cc::debug( "..............................." )
       << FirmwareInfoHelper::getFirmwareVersionString() << std::endl
       << cc::info( "OpenWrt Version" ) << cc::debug( "......................................" )
       << FirmwareInfoHelper::getSystemVersionString() << std::endl;

    stat_info_msg( ss.str() );
    cc::_on_ = cc_was_on;
}

void Service::printRuntimeOptions() {
    bool cc_was_on = cc::_on_;
    cc::_on_ = Log::getLocalColors();

    std::string strCanonicalHostName = tiny::network::get_canonical_host_name();
    size_t cntCPUs = tiny::tools::cpu_count();
    std::string strCloudCoordinatorURL = m_environment->getCloudCoordinatorURL();
    time_t ttCloudCoordinatorQueryTimeoutInSeconds = m_environment->getCloudCoordinatorQueryTimeout();
    time_t ttCloudCoordinatorReconnectPeriodInSeconds = m_environment->getCloudCoordinatorReconnectPeriod();
    std::string strNmaServerURL = m_environment->getNmaServerURL( false );
    std::stringstream ss;
    ss << std::endl
       << cc::info( "CPU core(s) count" ) << cc::debug( "...................................." )
       << cc::size10( cntCPUs ) << std::endl
       << cc::info( "Canonical host name" ) << cc::debug( ".................................." )
       << cc::sunny( strCanonicalHostName ) << std::endl
       << cc::info( "Thread count" ) << cc::debug( "............................." )
       << cc::size10( m_settings.cntThreadsInDispatcher() ) << std::endl
       << cc::info( "Call queue limit" ) << cc::debug( "........................." )
       << cc::size10( m_settings.queueLimitInDispatcher() ) << std::endl
       << cc::info( "WS server backend" ) << cc::debug( "...................................." )
       << cc::attention( tiny::ws::server::basic_participant::stat_backend_name() ) << std::endl
       << cc::info( "WS client backend" ) << cc::debug( "...................................." )
       << cc::attention( tiny::ws::client::basic_participant::stat_backend_name() ) << std::endl
#if( defined __HAVE_TINY_WS_BACKEND_NLWS__ )
       << cc::info( "NLWS server mode" ) << cc::debug( "....................................." )
       << cc::attention( tiny::ws::nlws::srvmode2str( tiny::ws::nlws::g_default_srvmode ) ) << std::endl
       << cc::info( "NLWS explicit vhost (simple and external poll)" ) << cc::debug( "......." )
       << cc::c( tiny::ws::nlws::g_default_explicit_vhost_enable ) << std::endl
       << cc::info( "NLWS dynamic vhosts (external poll)" ) << cc::debug( ".................." )
       << cc::c( tiny::ws::nlws::g_default_dynamic_vhost_enable ) << std::endl
#endif  /// (defined __HAVE_TINY_WS_BACKEND_NLWS__)
       << cc::info( "WS open handshake timeout(seconds)" ) << cc::debug( "..................." )
       << cc::num10( tiny::ws::basic_network_settings::default_instance().timeout_handshake_open_ ) << std::endl
       << cc::info( "WS close handshake timeout(seconds)" ) << cc::debug( ".................." )
       << cc::num10( tiny::ws::basic_network_settings::default_instance().timeout_handshake_close_ ) << std::endl
       << cc::info( "WS client restart interval after close(seconds)" ) << cc::debug( "......" )
       << cc::num10( tiny::ws::basic_network_settings::default_instance().timeout_restart_on_close_ ) << std::endl
       << cc::info( "WS client restart interval after fail(seconds)" ) << cc::debug( "......." )
       << cc::num10( tiny::ws::basic_network_settings::default_instance().timeout_restart_on_fail_ ) << std::endl
       << cc::info( "WS max message size(bytes)" ) << cc::debug( "..........................." )
       << ( ( tiny::ws::basic_network_settings::default_instance().max_message_size_ > 0 ) ?
                cc::size10( tiny::ws::basic_network_settings::default_instance().max_message_size_ ) :
                cc::warn( "unlimited" ) )
       << std::endl
       << cc::info( "WS max message body size(bytes)" ) << cc::debug( "......................" )
       << ( ( tiny::ws::basic_network_settings::default_instance().max_body_size_ > 0 ) ?
                cc::size10( tiny::ws::basic_network_settings::default_instance().max_body_size_ ) :
                cc::warn( "unlimited" ) )
       << std::endl
       << cc::info( "WS logging level" ) << cc::debug( "....................................." )
       << cc::note( tiny::ws::wsll2str( tiny::ws::g_eWSLL ) ) << std::endl
       << cc::info( "WS traffic logging" ) << cc::debug( "..................................." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().log_ws_rx_tx_ )
       << std::endl
       //
       << cc::info( "NMA Certificate file" ) << cc::debug( "................................." )
       << cc::pe( m_environment->getCertificateFile() ) << std::endl
       << cc::info( "NMA Certification chain file" ) << cc::debug( "........................." )
       << cc::pe( m_environment->getCertificationChainFile() ) << std::endl
       << cc::info( "NMA Private key file" ) << cc::debug( "................................." )
       << cc::pe( m_environment->getPrivateKeyFile() ) << std::endl;
    //	//
    ss << cc::info( "NMA client threaded message handling" ) << cc::debug( "................." )
       << ( NmaClient::g_bSingleThreadedPeerMessages ? cc::fatal( "single-threaded" ) :
                                                       cc::success( "multi-threaded" ) )
       << std::endl
       << cc::info( "NMA client threaded message answering" ) << cc::debug( "................" )
       << ( NmaClient::g_bSingleThreadedPeerAnswers ? cc::fatal( "single-threaded" ) : cc::success( "multi-threaded" ) )
       << std::endl
#if( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
       << cc::info( "WS performs global SSL initialization" ) << cc::debug( "................" )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_perform_global_init_ ) << std::endl
       << cc::info( "WS server: Disable IP v6" ) << cc::debug( "............................." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().server_disable_ipv6_ ) << std::endl
       << cc::info( "WS server: Enable server side UTF-8 validation" ) << cc::debug( "......." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().server_validate_utf8_ ) << std::endl
       << cc::info( "WS server: Disable host canonical name check" ) << cc::debug( "........." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().server_skip_canonical_name_ ) << std::endl
       << cc::info( "WS server: Require valid client certificate" ) << cc::debug( ".........." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_server_require_valid_certificate_ )
       << std::endl
       << cc::info( "WS server: Enable non-ssl connections on ssl port" ) << cc::debug( "...." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_server_allow_non_ssl_on_ssl_port_ )
       << std::endl
       << cc::info( "WS server: Accept selfsigned certificates" ) << cc::debug( "............" )
       << cc::c(
              tiny::ws::basic_network_settings::default_instance().ssl_server_accept_connections_without_valid_cert_ )
       << std::endl
       << cc::info( "WS server: Enable http to https redirecation" ) << cc::debug( "........." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_server_redirect_http_2_https_ )
       << std::endl
       //
       << cc::info( "WS client: Enable selfsigned certificate" ) << cc::debug( "............." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_client_allow_self_signed_ ) << std::endl
       << cc::info( "WS client: Skip host name check" ) << cc::debug( "......................" )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_client_skip_host_name_check_ ) << std::endl
       << cc::info( "WS client: Enable expired certificates" ) << cc::debug( "..............." )
       << cc::c( tiny::ws::basic_network_settings::default_instance().ssl_client_allow_expired_ ) << std::endl
       << cc::info( "WS client: Max bytes to send as message portion" ) << cc::debug( "......" )
       << ( tiny::ws::basic_network_settings::default_instance().cntMaxPortionBytesToSend_ ?
                cc::size10( tiny::ws::basic_network_settings::default_instance().cntMaxPortionBytesToSend_ ) :
                cc::warn( "unlimited" ) )
       << std::endl
#endif  /// if( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
       << cc::info( "Use colors in output" ) << cc::debug( "................................." ) << cc::c( cc_was_on )
       << std::endl
       << cc::info( "Reset mode in colorized output" ) << cc::debug( "......................." )
       << cc::reset_mode_2_str( cc::g_eResetMode ) << std::endl
       << cc::info( "JSON indent" ) << cc::debug( ".........................................." )
       << cc::num10( cc::_default_json_indent_ ) << std::endl
       << cc::info( "JSON string value limit" ) << cc::debug( ".............................." )
       << cc::size10( cc::g_nJsonStringValueOutputSizeLimit ) << std::endl
       << cc::info( "Max size of log message" ) << cc::debug( ".............................." )
       << cc::size10( ezlo::log_settings::maxLogMessageSizeGet() ) << std::endl
       << cc::info( "Log directory" ) << cc::debug( "........................................" )
       << cc::p( ezlo::log_settings::getLogDirectory() ) << std::endl
       << cc::info( "eZLO hub serial" ) << cc::debug( "......................................" )
       << cc::notice( Ezlo::serial() ) << std::endl
       << cc::info( "eZLO hub type" ) << cc::debug( "........................................" )
       << cc::note( Ezlo::hubType() ) << std::endl
       << cc::info( "Cloud coordinator: query timeout(seconds)" ) << cc::debug( "............" )
       << cc::size10( ttCloudCoordinatorQueryTimeoutInSeconds ) << std::endl
       << cc::info( "Cloud coordinator: reconnect period(seconds)" ) << cc::debug( ".......,." )
       << cc::size10( ttCloudCoordinatorReconnectPeriodInSeconds ) << std::endl
       << cc::info( "Cloud coordinator: URL" ) << cc::debug( "..............................." )
       << ( ( !strCloudCoordinatorURL.empty() ) ? cc::u( strCloudCoordinatorURL ) : cc::empty_str() ) << std::endl
       << cc::info( "NMA server URL" ) << cc::debug( "......................................." )
       << ( ( !strNmaServerURL.empty() ) ? cc::u( strNmaServerURL ) : cc::not_computed_yet_str() )
       << std::endl
       //
       << cc::info( "Restore environment on startup" ) << cc::debug( "......................." ) << std::endl
       << cc::info( "Unlock DB on startup" ) << cc::debug( "................................." )
       << cc::c( !m_settings.isNoDbUnlock() ) << std::endl
       << cc::info( "DB collection size limit" ) << cc::debug( "............................." )
       << cc::num10( m_settings.dbMaxCollectionSize() ) << std::endl
       << cc::info( "Perform locked state check on startup" ) << cc::debug( "................" )
       << cc::c( !m_settings.isNoLockedStateCheck() ) << std::endl
       << cc::info( "Convert data items to FOs on startup" ) << cc::debug( "................." )
       << cc::c( !m_settings.isNoConvertDataitemsToFOs() ) << std::endl
       << cc::info( "Enable replication thread" ) << cc::debug( "............................." )
       << cc::c( !m_settings.isNoReplication() ) << std::endl;

    printRegisteredCustomServices( ss );
    ::tiny::command_line::parser::stat_network_interfaces_info( ss );

    std::string s = ss.str();
    size_t n = s.length();
    if( n > 0 && s[ n - 1 ] == '\n' )
        s.erase( n - 1, 1 );
    stat_info_msg( s );  // std::cout << s; std::cout.flush();
    cc::_on_ = cc_was_on;
}

/** Fetches current/currentCert field from UI credentials database. Returns -1 if record not found. */
int Service::getCurrentCertificateIndex() const {
    auto fieldsToReturn = JSON( "currentCert" << 1 );
    auto prevJson = getDBClient().findOne( CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), &fieldsToReturn );
    if( !prevJson.empty() ) {
        return json_get_int( prevJson, "currentCert", -1 );
    }
    return -1;
}

void Service::setCurrentCertificateIndex( int idx ) {
    getDBClient().update(
        CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), JSON( "$set" << JSON( "currentCert" << idx ) ) );

    m_clusterManager.onCertificatesUpdated();
}

/** Gets boundaries in which the current certificate is valid:
 * Fetched current certificate an d extracts notBefore, notAfter fields from it.
 * Returns a pair with zeroes if any errors occurs. */
std::pair< int, int > Service::getTimeUntilCurrentCertificateExpire() const {
    std::pair< int, int > result = { 0, 0 };

    auto current = getDBClient().findOne( CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), nullptr );
    if( current.empty() ) {
        return result;
    }

    auto certificates = current[ "certificates" ];
    if( certificates.empty() ) {
        return result;
    }

    int currentIndex = json_get_int( current, "currentCert", -1 );
    if( currentIndex == -1 ) {
        return result;
    }

    JsonObject validityPeriods;
    auto notAfter = json_get_str( certificates[ std::to_string( currentIndex ) ], "notAfter" );

    if( !SslUtils::asn1TimeDiff( result.first, result.second, "", notAfter ) ) {
        LOGGER_ERROR(
            "Certificate expiration: Couldn't calculate time difference between asn1 times: left=empty and right=" +
            notAfter );
        result = { 0, 0 };
    }

    if( result.first < 0 || result.second < 0 ) {
        LOGGER_WARN( "Certificate expiration: Time difference is negative, reset time to expire to zero, notAfter=" +
                     notAfter );
        result = { 0, 0 };
    }

    return result;
}

/** Fetches the current certificates record, extracts certificates collection from it.
 * Finds a certificates record that contains current time and has the expiration date that is closest to now. */
int Service::getNextCertificateIndex() const {
    auto current = getDBClient().findOne( CredentialsDBName, JSON( db::ID_FIELD_NAME << "current" ), nullptr );
    if( current.empty() ) {
        Log::debug( cc::debug( "Local mode server certificates not synced" ) );
        return -1;
    }

    auto certificates = current[ "certificates" ];
    if( certificates.empty() ) {
        Log::debug( cc::debug( "No local mode server certificates indexed" ) );
        return -1;
    }

    SslUtils::ValidityPeriods periods;
    periods.resize( certificates.size() );
    LOGGER_DEBUG_RAW( "Number of local mode server certificates: " + cc::num10( certificates.size() ) );

    for( auto it = certificates.begin(); it != certificates.end(); ++it ) {
        auto idx = it.key();
        auto& validityPeriods = it.value();
        std::string notBefore = json_get_str( validityPeriods, "notBefore" );
        std::string notAfter = json_get_str( validityPeriods, "notAfter" );
        periods[ std::atoi( idx.c_str() ) ] = std::make_pair( notBefore, notAfter );
        LOGGER_INFO_RAW( "Local mode server certificates, index: " + cc::str( idx ) + ", notBefore: " + notBefore +
                         ", notAfter: " + notAfter );
    }
    int idx = SslUtils::chooseValidPeriod( periods );
    if( idx == -1 ) {
        LOGGER_INFO_RAW( cc::info( "No valid local mode server certificates found" ) );
        return -1;
    }
    LOGGER_INFO_RAW( cc::info( "Valid local mode server certificates found, index: " ) + cc::num10( idx ) );

    return idx;
}

/** Starts or stops local mode server according to he state of local mode server certificates.
 *  Whether certificates are present and current date is not in range, local mode server is stopped. */
void Service::manageServerListener( bool forceRestart ) {
    LOGGER_INFO_RAW( cc::info( "Checking if there are new local mode certificates" ) );

    int currentIdx = getCurrentCertificateIndex();
    int nextIdx = getNextCertificateIndex();

    if( -1 == currentIdx ) {
        LOGGER_WARN_RAW( "Current local mode certificate index is not set: -1" );
    }
    LOGGER_INFO_RAW( "Next local mode certificate index: " + cc::num10( nextIdx ) );

    bool serverRunning = getServerListener().get();
    if( nextIdx == -1 ) {
        Log::warn(
            cc::warn( "No local mode credentials and certificates (no certificates that match the current date)" ) );

        if( serverRunning ) {
            setCurrentCertificateIndex( nextIdx );
            stopServerListener();
        }
    }
    else {
        bool shouldStart = false;
        LOGGER_INFO_RAW( cc::info( "Current certificate idx: " ) + cc::num10( currentIdx ) +
                         cc::info( ", server running: " ) + cc::flag( serverRunning ) );
        if( currentIdx == -1 ) {
            LOGGER_INFO_RAW( cc::info( "There was no valid certificate" ) );
            shouldStart = true;
        }
        else if( !serverRunning ) {
            LOGGER_INFO_RAW( cc::debug( "Local mode server was not running" ) );
            shouldStart = true;
        }
        else {
            if( currentIdx != nextIdx ) {
                LOGGER_INFO_RAW( cc::info( "Server connections: " ) +
                                 cc::num10( getServerListener()->getNumberOfConnections() ) );
                if( getServerListener()->getNumberOfConnections() == 0 ) {
                    LOGGER_INFO_RAW( cc::debug( "Server have no active connections" ) );
                    shouldStart = true;
                }
                else {
                    auto time = getTimeUntilCurrentCertificateExpire();
                    LOGGER_INFO_RAW( cc::trace( "Remaining days: " ) + cc::num10( time.first ) +
                                     cc::trace( ", seconds: " ) + cc::num10( time.second ) );
                    if( time.first == 0 && time.second < AccessKeysSync::SecondsBeforeCertificatesForceChange ) {
                        LOGGER_INFO_RAW( cc::debug( "Current certificate is near to expire" ) );
                        shouldStart = true;
                    }
                }
            }
        }

        if( shouldStart || forceRestart ) {
            if( currentIdx != nextIdx ) {
                LOGGER_INFO_RAW( "Set current certificate index with next index value: " + cc::num10( nextIdx ) );
                setCurrentCertificateIndex( nextIdx );
            }
            startServerListener();
        }
        else {
            LOGGER_INFO_RAW( cc::debug( "Nothing to do" ) );
        }
    }
}

void Service::watchForServerListener() {
    createPostponedTask( [this]( Services::PostponedTask& ) { manageServerListener(); } )
        ->start( std::chrono::seconds( 0 ), AccessKeysSync::CheckCertificatesInterval );
    auto requestOfflineCredentialsTask = createPostponedTask(
        [this]( Services::PostponedTask& ) { HubOfflineCommandsHandler( *this ).requestCredentials(); } );
    requestOfflineCredentialsTask->start( std::chrono::seconds( 0 ), AccessKeysSync::RequestCertificatesInterval );

    registerRequestOfflineCredentialsCall( std::move( requestOfflineCredentialsTask ) );
}

/** Resets timer for fetching access_keys_controller_sync. Stops the previous timer if present. */
void Service::registerRequestOfflineCredentialsCall( Services::PostponedTaskPtr fetchOfflineCredentials ) {
    if( m_fetchOfflineCredentials )
        m_fetchOfflineCredentials->stop();

    m_fetchOfflineCredentials = std::move( fetchOfflineCredentials );
}

void Service::onHubRegistered( bool isHubRegistered ) {
    getDBClient().update( HaTempCacheDBName,
                          JSON_EMPTY_OBJ,
                          JSON( db::ID_FIELD_NAME << "cache-cloud-status"
                                                  << "registered" << isHubRegistered ),
                          true,
                          false );
    sendBroadcast( isHubRegistered ? CLOUD_REGISTERED : CLOUD_UNREGISTERED, JSON_EMPTY_OBJ, GroupsNames::CLOUD_GROUP );
}

void Service::onHubRegisterRepeatStarted() {
    sendBroadcast( HUB_CLOUD_REGISTER_REPEAT_PROGRESS,
                   JSON( "status"
                         << "started" ) );
}

void Service::onHubRegisterRepeatFinished() {
    m_cloudStorage->sendUrlAndTokenRequest();                 // Request storage tokens
    HubOfflineCommandsHandler( *this ).requestCredentials();  // Request access tokens
    sendBroadcast( HUB_CLOUD_REGISTER_REPEAT_PROGRESS,
                   JSON( "status"
                         << "finished" ) );
}

bool Service::redirectCloudMessage( rpc::Message& rpcMessage ) {
    LOGGER_TRACE_RAW( cc::trace( "Checking if a cloud message should be redirected" ) );
    JsonObject& joMessage = rpcMessage.getMessage();
    if( rpcMessage.getId() == "ui_broadcast" ) {
        auto subclass = json_get_str( rpcMessage.getMessage(), "msg_subclass" );
        auto subclassDotPos = subclass.find( '.' );
        if( subclassDotPos != std::string::npos ) {
            const auto prefix = subclass.substr( 0, subclassDotPos );
            if( prefix == "cloud" ) {
                LOGGER_TRACE_RAW( cc::trace( "Redirect as broadcast" ) );
                sendBroadcast( CLOUD_MESSAGE, joMessage, GroupsNames::CLOUD_GROUP );
                return true;
            }
        }
    }
    else {
        auto uuid = rpcMessage.getId();
        auto method = rpcMessage.getMethod();
        auto submethodPos = method.find( '.' );
        if( submethodPos != std::string::npos ) {
            const auto prefix = method.substr( 0, submethodPos );
            if( prefix == "cloud" ) {
                if( joMessage.count( "result" ) != 0 ) {
                    LOGGER_TRACE_RAW( cc::trace( "Redirect as response" ) );
                    if( hasError( joMessage ) ) {
                        joMessage[ "result" ][ "error" ] = std::move( joMessage[ "error" ] );
                    }
                    std::string service = GroupsNames::CLOUD_GROUP;
                    auto delimiter = uuid.find( '|' );
                    if( delimiter != std::string::npos ) {
                        service = uuid.substr( 0, delimiter );
                        uuid = uuid.substr( delimiter + 1 );
                    }

                    auto error = sendAsyncResponse( service, joMessage[ "result" ], uuid );

                    if( !error.empty() ) {
                        auto& joAnswer = rpcMessage.getAnswer();
                        joAnswer[ "error" ] = error;
                        joAnswer[ "result" ] = nullptr;
                        m_nmaClient->postMessage( joAnswer );
                    }

                    return true;
                }
                if( joMessage.count( "params" ) != 0 ) {
                    LOGGER_TRACE_RAW( cc::trace( "Redirect as request" ) );
                    rpcMessage.answerPrepare();
                    rpcMessage.copySender();
                    auto& joAnswer = rpcMessage.getAnswer();
                    if( joMessage.count( "api" ) != 0 ) {
                        joAnswer[ "api" ] = std::move( joMessage[ "api" ] );
                    }

                    sendAsyncRequest(
                        GroupsNames::CLOUD_GROUP,
                        joMessage,
                        CLOUD_MESSAGE,
                        [joAnswer, this]( const JsonObject& response ) mutable {
                            setErrorIfHas( joAnswer, response );
                            joAnswer[ "result" ] = std::move( response );
                            json_remove( joAnswer[ "result" ], Errors::ERROR_FIELD );
                            m_nmaClient->postMessage( joAnswer );
                        },
                        [joAnswer, method, this]() mutable {
                            Log::error( cc::error( "Received command not handled: " ) + cc::note( method ) );
                            setError( joAnswer, Errors::UNKNOWN_METHOD );
                            m_nmaClient->postMessage( joAnswer );
                        } );
                    return true;
                }
            }
        }
    }
    return false;
}

int Service::getServerPort() const {
    return !m_server || m_server->getPort() == 0 ? 17000 : m_server->getPort();
}

int Service::getInsecureServerPort() const {
    return !m_server || m_server->getInsecurePort() == 0 ? 17001 : m_server->getInsecurePort();
}

std::unique_ptr< server_listener >& Service::getServerListener() {
    return m_server;
}

void Service::stopServerListener() {
    if( m_server ) {
        Log::info( cc::info( "Stopping local server" ) );
        m_server->service_interrupt();
        m_server.reset();
    }
}

std::unique_lock< std::mutex > Service::lockCertificateFiles() {
    return std::unique_lock< std::mutex >( m_certificateFilesMutex );
}

tiny::ws::security_args Service::getSecurityArgs() {
    tiny::ws::security_args securityArgs;

    int idx = getCurrentCertificateIndex();
    if( idx == -1 ) {
        return securityArgs;
    }

    std::string localCertificatesDirectory = HubPlatform::Environment::getCertificateFolder() + "/local";
    std::string basePath = localCertificatesDirectory + "/" + std::to_string( idx );
    std::string strCertificateFile = basePath + ".crt";
    std::string strCertificationChainFile = basePath + ".ca";
    std::string strPrivateKeyFile = basePath + ".key";

    if( !filesystem::isFileExist( strCertificateFile ) || !filesystem::isFileExist( strCertificationChainFile ) ||
        !filesystem::isFileExist( strPrivateKeyFile ) ) {
        return securityArgs;
    }
    securityArgs.strCertificateFile_ = std::move( strCertificateFile );
    securityArgs.strCertificationChainFile_ = std::move( strCertificationChainFile );
    securityArgs.strPrivateKeyFile_ = std::move( strPrivateKeyFile );
    return securityArgs;
}

bool Service::startServerListener() {
    stopServerListener();

    int idx = getCurrentCertificateIndex();
    if( idx == -1 ) {
        return false;
    }

    Log::info( cc::info( "Starting local server with certificate " ) + cc::note( std::to_string( idx ) ) );

    tiny::ws::security_args securityArgs = getSecurityArgs();
    if( !securityArgs.valid() ) {
        Log::error( cc::error( "Local mode server certificates are missing" ) );
        return false;
    }

    auto onMessage = [this]( server_peer_message& message ) {
        bool isHandled = false;
        if( message.getPeerObject()->uiType() == "HUB" ) {
            auto uuid = message.getPeerObject()->userId();
            isHandled = getIccManager().processDirectCmd( message, uuid );
        }
        if( !isHandled ) {
            m_nmaClient->getUiProto().processCmd( message );
        }
    };
    auto onBeforeLwsCreateContext =
        []( const lws_protocols*& protocols, unsigned& length, const lws_http_mount*& mount ) {
            LOGGER_TRACE_RAW( "onBeforeLwsCreateContext: adding ApiBridge" );
            using ApiBridge = HubPlatform::HttpBridge::ApiBridge;

            mount = &ApiBridge::getHttpMount();

            protocols = ApiBridge::getProtocols( length );
        };

    std::string serverUrlScheme = "wss";
    int serverPort = m_settings.isRandomLocalServerPort() ? 0 : getServerPort();
    {
        auto lock = lockCertificateFiles();
        server_vhost_settings defaultSettings(
            securityArgs, m_settings.isRandomLocalServerPort() ? 0 : getServerPort(), serverUrlScheme );
        server_vhost_settings insecureSettings(
            tiny::ws::security_args(), m_settings.isRandomLocalServerPort() ? 0 : getInsecureServerPort(), "ws" );
        m_server = std::make_unique< server_listener >(
            onMessage, defaultSettings, insecureSettings, onBeforeLwsCreateContext );
    }

    injectApiBridge();

    // Random port selection is supposed to be used only for tests
    // Just for tests send broadcast with actual local server port
    // NOTE: MDNS announce default 17000 port despite of what actual port is
    if( m_settings.isRandomLocalServerPort() ) {
        LOGGER_DEBUG( "Trying to start local mode server, using random port" );
        int actualPort = m_server->getPort();
        if( actualPort ) {
            sendBroadcast( LOCAL_SERVER_STARTED,
                           JSON( "port" << actualPort << "insecurePort" << m_server->getInsecurePort() ) );
            Log::warn( cc::warn( "Local WebSocket server: server is started on a random port " ) +
                       cc::note( cc::num10( actualPort ) ) + cc::warn( ", insecure port " ) +
                       cc::note( cc::num10( m_server->getInsecurePort() ) ) );
            onLocalServerStarted( actualPort, m_server->getInsecurePort() );
        }
    }
    else {
        LOGGER_DEBUG( "Trying to start local mode server" );
        onLocalServerStarted( m_server->getPort(), m_server->getInsecurePort() );
    }
    g_fn_on_shutdown_interrupt_nma_server_ = [this]() {
        g_fn_on_shutdown_interrupt_nma_server_ = nullptr;
        try {
            stopServerListener();
        }
        catch( const std::exception& ex ) {
            const char* strWhat = ex.what();
            if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
                strWhat = "unknown exception";
            Log::error( cc::error( "Local WebSocket server: caught exception:" ) + cc::warn( strWhat ) );
        }
        catch( ... ) {
            Log::error( cc::error( "Local WebSocket server: caught unknown exception." ) );
        }
    };
    return true;
}

void Service::setUp() {
    Services::Service::setUp();

    createPostponedTask( [this]( Services::PostponedTask& ) {
        auto status = checkRotateLogsStatus() == 1 ? true : false;
        if( status ) {
            resetRotateLogsStatus();
            sendBroadcast( HUB_LOGS_ROTATED, JSON_EMPTY_OBJ, GroupsNames::BROADCAST_GROUP );
        }
    } )
        ->start( std::chrono::minutes( 1 ), true );

    join( GroupsNames::DEVICE_GROUP );
    join( GroupsNames::BROADCAST_GROUP );
    join( GroupsNames::UPDATE_MANAGER_BROADCASTS );
    join( GroupsNames::BACKUPD_BROADCASTS );

    std::srand( std::time( nullptr ) );
    m_environment = Environment::create( m_settings, getDBClient() );

    try {
        tiny::multithreading::on_exception_handler() = []( const std::string& s ) -> bool {
            stat_error_msg( s );
            return true;  // do not cancel exception throwing
        };

        std::string strAppVersion = HubPlatform::Version::git_version();
        //
        size_t nThreadCount = m_settings.cntThreadsInDispatcher();
        size_t nCallQueueLimit = m_settings.queueLimitInDispatcher();
        tiny::dispatch::default_domain( nThreadCount, nCallQueueLimit );  // init default domain
        //
        if( cc::_on_ )
            std::cout << cc::reset();
        stat_startup_msg( cc::p( getBinaryName() ) + cc::info( " daemon is started. " ) + cc::warn( strAppVersion ) );
        stat_startup_msg( cc::debug( "Main startup." ) );
        stat_startup_msg( cc::debug( "Common signal handling startup." ) );
        ::atexit( stat_handle_at_exit );
        g_bIsStopEzloHubEmotionsOnShutdown = m_environment->isStopEzloHubEmotionsOnShutdown();
        tiny::signal::init_common_signal_handling( stat_handle_signal );

        stat_startup_msg( cc::debug( "Initializing " ) + cc::info( "SSL" ) + cc::debug( "..." ) );
        stat_startup_msg( cc::debug( "    " ) + cc::info( "Library" ) + cc::debug( "..." ) );
        ::SSL_library_init();
        stat_startup_msg( cc::debug( "    " ) + cc::info( "SSL error information strings" ) + cc::debug( "..." ) );
        ::SSL_load_error_strings();
        stat_startup_msg( cc::debug( "    " ) + cc::info( "BIO error information strings" ) + cc::debug( "..." ) );
        ::ERR_load_BIO_strings();
        stat_startup_msg( cc::debug( "    " ) + cc::info( "Algorithms" ) + cc::debug( "..." ) );
        ::OpenSSL_add_all_algorithms();
        stat_startup_msg( cc::debug( "    " ) + cc::info( "Digests" ) + cc::debug( "..." ) );
        ::OpenSSL_add_all_digests();
        stat_startup_msg( cc::success( "Done" ) + cc::debug( ", " ) + cc::info( "SSL" ) +
                          cc::debug( " initialization." ) );
        //
        stat_startup_msg( cc::debug( "Initializing " ) + cc::info( "CURL" ) + cc::debug( "..." ) );
        ::curl_global_init( CURL_GLOBAL_ALL );  // CURL init

        //
        if( !m_settings.isNoDbUnlock() ) {
            stat_startup_msg( cc::debug( "Checking " ) + cc::info( "DB" ) + cc::debug( " is unlocked..." ) );
            Db::unlock();
        }
        int maxCollectionSize = m_settings.dbMaxCollectionSize();
        if( maxCollectionSize > 0 ) {
            stat_startup_msg( cc::debug( "Setting " ) + cc::info( "DB" ) + cc::debug( " max collection size to " ) +
                              cc::num10( maxCollectionSize ) + cc::debug( "..." ) );
            Db::capped( "db.firmware.cored.devicelog", maxCollectionSize );
        }
        if( !m_settings.isNoLockedStateCheck() ) {
            stat_startup_msg( cc::debug( "Checking device " ) + cc::info( "locked state" ) + cc::debug( "..." ) );
            bool bIsDeviceLocked = Db::getFlag( "isDevicesLocked" );
            stat_startup_msg( cc::debug( "Device " ) + cc::info( "locked state" ) + cc::debug( " is " ) +
                              cc::c( bIsDeviceLocked ) );
            if( bIsDeviceLocked ) {
                // NOTE: currently there is no way to lock device,
                // will need to implement proper indication of this state when it will be implemented
            }
        }
        //

        //
        // l_sergiy: check "cache-owner-info" state
        std::string strOwnerEmail, strHash;
        auto joCacheOwner = Db::findOne( HaCacheDBName, JSON( "cache-owner-info" << 1 ) );
        if( !joCacheOwner.empty() ) {
            strOwnerEmail = getFieldSafe< std::string >( joCacheOwner, "cache-owner" );
            strHash = getFieldSafe< std::string >( joCacheOwner, "cache-hash" );
        }
        if( ( !strOwnerEmail.empty() ) && ( !strHash.empty() ) )
            Log::info( cc::success( "Success" ) + cc::debug( ", have " ) + cc::info( "cache-owner-info" ) +
                       cc::debug( ", cached " ) + cc::info( "owner" ) + cc::debug( " is " ) + cc::eml( strOwnerEmail ) +
                       cc::debug( ", cached " ) + cc::info( "hash" ) + cc::debug( " is " ) + cc::warn( strHash ) );
        else
            Log::info( cc::notice( "Notice" ) + cc::debug( ", " ) + cc::info( "cache-owner-info" ) +
                       cc::debug( " is " ) + cc::error( "not present" ) + cc::debug( " in DB yet" ) );

        //
        if( m_settings.isNoBus() ) {
            stat_startup_msg( cc::debug( "No " ) + cc::info( "bus" ) + cc::debug( " mode..." ) );
        }
        else {
            stat_startup_msg( cc::debug( "Initializing main " ) + cc::info( "bus" ) + cc::debug( " in daemon..." ) );
        }

        tiny::ws::security_args nmaSecurityArgs;

        nmaSecurityArgs.strCertificateFile_ = m_environment->getCertificateFile();
        nmaSecurityArgs.strPrivateKeyFile_ = m_environment->getPrivateKeyFile();
        nmaSecurityArgs.strCertificationChainFile_ = m_environment->getCertificationChainFile();
        registerCustomServices();

        printFirmwareVersion();
        printRuntimeOptions();

        m_nmaClient.reset( new NmaClient(
            *this,
            nmaSecurityArgs,
            [this]( NmaClient& ws, bool isRead, const std::string& msg, tiny::ws::client::opcv eOpCode ) {
                if( !ws.log_ws_rx_tx_ )
                    return;
                if( NmaClient::g_bEnableNmaClientStatsLogging ) {
                    std::stringstream ss;
                    ss << cc::debug( "Main NMA connection summary traffic stats:" ) << ws.getLifeTimeDescription( true )
                       << cc::debug( ", " ) << ws.getTrafficStatsDescription( true );
                    //
                    std::string strEventEPSs = ws.getEventStatsDescription( true );
                    if( !strEventEPSs.empty() )
                        ss << cc::debug( " summary event stats: " ) << strEventEPSs;
                    stat_classified_msg( getLibWebSocketLogPrefix(), ss.str() );
                }

                stat_classified_msg(
                    getLibWebSocketLogPrefix(),
                    cc::rpc_log( isRead ? cc::e_rpc_log_type_t::erpcltt_ws_rx : cc::e_rpc_log_type_t::erpcltt_ws_tx,
                                 "NMA",  // "NMA connection"
                                 m_environment->getNmaServerURL( false ),
                                 msg,
                                 eOpCode == tiny::ws::client::opcv::binary ) );
            },
            m_environment->getExplicitNmaServerURL() ) );

        addTask( [this]() {
            Network::RemoteInterface networkRemoteInterface( *this );
            bool isInternetAvailable = networkRemoteInterface.isInternetAvailable();
            LOGGER_INFO_RAW( cc::info( "Service startup: is internet available: " ) + cc::flag( isInternetAvailable ) );
            m_nmaClient->enableRestartTimer( isInternetAvailable );
            m_cloudStorage->setInternetAvailable( isInternetAvailable );
        } );

        auto loopTask = createPostponedTask( [this]( Services::PostponedTask& ) {
            LOGGER_DEBUG_RAW( cc::debug( "It's time to check controller information." ) );
            const auto updateStatus = m_environment->requestNewProvisionInformation();
            applyProvisioningRequirements( updateStatus );
            LOGGER_DEBUG_RAW( cc::debug( "Controller information check finished." ) );
        } );

        loopTask->setPeriodicMode( Services::PostponedTask::PeriodicMode::FixedIdleDuration );
        loopTask->start( ProvisioningReaskInterval, true );

        watchForServerListener();
        // TODO: server port unknown here, move it after local server creation
        addTask( [this]() { MdnsConfig::runMdns( getServerPort() ); } );

        auto provisionData = ProvisionDataUtils::getManager().getProvisionData();
        if( !provisionData.isValid() ) {
            HardwareManager::RemoteInterface hardwareManager( *this );
            hardwareManager.setCloudConnection(
                HardwareManager::HubState::CloudConnection::UpdatingControllerInformation );
        }
        Db::clrFlag( DbFlags::NmaClientConnected );

        addTask( [this]() { m_clusterManager.setUp(); } );
        m_cloudStorage->initStorage();
    }
    catch( const std::exception& ex ) {
        const char* strWhat = ex.what();
        if( strWhat == nullptr || strWhat[ 0 ] == '\0' )
            strWhat = "unknown exception";
        Log::error( cc::error( "Top level, caught exception:" ) + cc::warn( strWhat ) );
        stop();
    }
    catch( ... ) {
        Log::error( cc::error( "Top level, caught unknown exception." ) );
        stop();
    }

    m_connectionReporter = std::make_unique< ConnectionNmaReporter >( getDBClient() );

    Service::Service::setReporter(
        std::make_unique< UIReporter >( getName(), m_environment, m_nmaClient, m_settings, *m_connectionReporter ) );

    addTask( [this]() { m_uiExtensionsManager.setUp(); } );

    cc::RpcMessageFiltersSingleton::instance().pushFront( std::make_shared< WsCommandRpcMessageFilters >() );
}

void Service::tearDown() {
    m_cloudStorage.reset();
    m_clusterManager.tearDown();
    registerRequestOfflineCredentialsCall( nullptr );

    HardwareManager::RemoteInterface hardwareManager( Uid::Service::getInstance() );
    hardwareManager.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
    Db::clrFlag( DbFlags::NmaClientConnected );

    Ezlo::setBus( nullptr );

    m_nmaClient->closeConnection();

    stat_shutdown_msg( cc::debug( "Have " ) + cc::info( "stop flag" ) + cc::debug( " set to " ) +
                       cc::c( tiny::signal::g_bStop ) + cc::debug( " before exit" ) );
    stat_handle_at_exit();
    // stat_handle_at_exit / nmaClient uses busWrapper to send cloud_unregistered message
    Services::Service::tearDown();
    stat_shutdown_msg( cc::info( "Application exit point reached" ) );
}

ProvisionDataUtils::UpdateStatus Service::updateAndApplyProvisioningWithDelay(
    const ProvisionDataUtils::ProvisionData& provisioningData,
    const Interval& delay ) {
    const auto status = m_environment->updateProvisioningData( provisioningData );
    if( status != ProvisionDataUtils::UpdateStatus::Failed &&
        status != ProvisionDataUtils::UpdateStatus::NothingRequired ) {
        sendBroadcast( HUB_CLOUD_ENVIRONMENT_CHANGED, JSON_EMPTY_OBJ );
        if( delay.count() == 0 ) {
            applyProvisioningRequirements( status );
        }
        else {
            auto loopTask = createPostponedTask(
                [this, status]( Services::PostponedTask& ) { applyProvisioningRequirements( status ); } );

            loopTask->start( delay, false );
        }
    }

    return status;
}

void Service::applyProvisioningRequirements( ProvisionDataUtils::UpdateStatus status ) {
    switch( status ) {
    case ProvisionDataUtils::UpdateStatus::RestartRequired:
        m_environment->restart();
        break;

    case ProvisionDataUtils::UpdateStatus::ReconnectRequired:
        LOGGER_DEBUG_RAW( cc::debug( "Restart connection after applying new controller information." ) );
        m_environment->invalidateNmaServerURL();
        if( m_nmaClient->isConnected() ) {
            m_nmaClient->closeConnection();
            m_nmaClient->enableRestartTimer( true );
        }
        break;

    default:
        break;
    }
}

void Service::handleMessageFromBus( const Services::Message& message, const std::string& sender ) {
    switch( message.msgClass ) {
    case EZLO_EVENT:
        handleEzloEventFromBus( message );
        break;

    case HA_EXCEPTION:
        handleHaExceptionFromBus( message );
        break;

    case UI_EVENT:
        handleUiEventFromBus( message );
        break;

    case SYSTEM_INFO:
        handleSystemInfoFromBus( message );
        break;

    case LOG_CMD:
        handleLogCmdFromBus( message );
        break;

    case EVENT:
        handleAllUiEventsFromBus( message );
        handleEventFromBus( message );
        break;

    default:
        Services::Service::handleMessageFromBus( message, sender );
        break;
    }
}

void Service::handleMethodCall( const JsonObject& json, MessageSubClasses requestType, const std::string& sender ) {
    switch( requestType ) {
    case ICC_REGISTER_CONTROLLER_CONNECTION:
        m_clusterManager.handleRegisterControllerConnection( json, sender );
        break;
    case ICC_UNREGISTER_CONTROLLER_CONNECTION:
        m_clusterManager.handleUnregisterControllerConnection( json, sender );
        break;
    }

    m_uiExtensionsManager.handleMethodCall( json, requestType, sender );
}

void Service::handleEzloEventFromBus( const Services::Message& message ) {
    if( !Answers::remove( message.data ) ) {
        auto wepMessage = message.data;
        wepMessage[ "result" ][ "serial" ] = Ezlo::serial();

        if( m_nmaClient )
            m_nmaClient->postMessage( wepMessage.dump() );
    }
}

void Service::handleHaExceptionFromBus( const Services::Message& message ) {
    if( m_nmaClient )
        m_nmaClient->postMessage( message.data.dump() );
}

void Service::handleUiEventFromBus( const Services::Message& message ) {
    if( m_nmaClient )
        m_nmaClient->postMessage( message.data.dump() );
}

void Service::handleAllUiEventsFromBus( const Services::Message& message ) {
    m_nmaClient->processBroadcast( message );
    m_uiExtensionsManager.handleEventFromBus( message.data, static_cast< MessageSubClasses >( message.msgSubClass ) );
    switch( message.msgSubClass ) {
    case PLUGIN_RUN_PROGRESS:
    case PLUGIN_UI_BROADCAST:
    case GET_HUB_STATUS:
    case HUB_GATEWAY_ADDED:
    case HUB_GATEWAY_UPDATED:
    case HUB_GATEWAY_REMOVED:
    case HUB_GATEWAY_SETTING_ADDED:
    case HUB_GATEWAY_SETTING_UPDATED:
    case HUB_GATEWAY_SETTING_REMOVED:
    case HUB_DEVICE_ADDED:
    case HUB_DEVICE_REMOVED:
    case HUB_DEVICE_UPDATED:
    case HUB_ITEM_ADDED:
    case HUB_ITEM_UPDATED:
    case HUB_ITEM_REMOVED:
    case HUB_DEVICE_SETTING_ADDED:
    case HUB_DEVICE_SETTING_UPDATED:
    case HUB_DEVICE_SETTING_REMOVED:
    case SCENE_ADDED:
    case SCENE_CHANGED:
    case SCENE_DELETED:
    case SCENE_PROGRESS:
    case GLOBAL_SCENE_ADDED:
    case GLOBAL_SCENE_CHANGED:
    case GLOBAL_SCENE_DELETED:
    case GLOBAL_SCENE_PROGRESS:
    case SCENE_EXPRESSION_ADDED:
    case SCENE_EXPRESSION_CHANGED:
    case SCENE_EXPRESSION_DELETED:
    case HUB_DEVICE_FIRMWARE_UPDATE_PROGRESS:
    case HUB_DEVICE_FIRMWARE_DOWNLOAD_PROGRESS:
    case HUB_DEVICE_CHECKED_PROGRESS:
    case PLANNING_SOFTWARE_UPDATE:
    case UPDATING_FIRMWARE:
    case UPDATING_SOFTWARE:
    case HOUSE_MODE_ALARMED:
    case HOUSE_MODE_CANCEL_ENTRY_DELAY:
    case HOUSE_MODE_SKIP_ENTRY_DELAY:
    case HOUSE_MODE_SET_ENTRY_DELAY:
    case HOUSE_MODE_ENTRY_DELAY_RESET:
    case HOUSE_MODE_ENTRY_DELAY_CHANGED:
    case HOUSE_MODE_SWITCHED:
    case HOUSE_MODE_NOTIFICATIONS_ADDED:
    case HOUSE_MODE_NOTIFICATIONS_REMOVED:
    case HOUSE_MODE_NOTIFICATIONS_NOTIFYALL:
    case HOUSE_MODE_CHANGED:
    case HOUSE_MODE_DISARMED_DEVICES_ADDED:
    case HOUSE_MODE_DISARMED_DEVICES_REMOVED:
    case HOUSE_MODE_ALARMS_OFF_ADDED:
    case HOUSE_MODE_ALARMS_OFF_REMOVED:
    case HOUSE_MODE_CAMERAS_OFF_ADDED:
    case HOUSE_MODE_CAMERAS_OFF_REMOVED:
    case HOUSE_MODE_BYPASS_DEVICES_ADDED:
    case HOUSE_MODE_BYPASS_DEVICES_REMOVED:
    case HOUSE_MODE_PROTECT_BUTTONS_ADDED:
    case HOUSE_MODE_PROTECT_BUTTONS_UPDATED:
    case HOUSE_MODE_PROTECT_BUTTONS_REMOVED:
    case HOUSE_MODES_PROTECT_DEVICES_ADDED:
    case HOUSE_MODES_PROTECT_DEVICES_REMOVED:
    case HOUSE_MODES_LOCAL_ALARM_OFF:
    case HOUSE_MODES_LOCAL_ALARMS_TURNED_OFF:
    case HUB_ROOM_CREATED:
    case HUB_ROOM_EDITED:
    case HUB_ROOM_DELETED:
    case HUB_ROOMS_REORDERED:
    case NETWORK_WIFI_SCAN_PROGRESS:
    case HUB_NETWORK_CHANGED:
    case HUB_USER_NOTIFICATION:
    case HUB_ITEM_DICTIONARY_UPDATED:
    case HUB_DEVICE_SETTING_DICTIONARY_UPDATED:
    case HUB_INFO_CHANGED:
    case ZWAVE_NODES_PROVISIONING_ADDED:
    case ZWAVE_NODES_PROVISIONING_UPDATED:
    case ZWAVE_NODES_PROVISIONING_REMOVED:
    case HUB_SETTING_UPDATED:
    case NETWORK_MODEM_PIN_CODE_SET:
    case NETWORK_MODEM_PIN_CODE_RESET:
    case NETWORK_MODEM_TRY_CONNECT_FAILED:
    case NETWORK_MODEM_PIN_CODE_SET_FAILED:
    case NETWORK_MODEM_PIN_CODE_RESET_FAILED:
    case NETWORK_MODEM_PIN_CODE_ENTER_FAILED:
    case NETWORK_MODEM_SIM_LOCK_SET_FAILED:
    case NETWORK_STATISTICS_GET:
    case BACKUP_PROGRESS:
    case RESTORE_PROGRESS:
    case DEVICE_GROUP_CREATED:
    case DEVICE_GROUP_DELETED:
    case DEVICE_GROUP_UPDATED:
    case ITEM_GROUP_CREATED:
    case ITEM_GROUP_DELETED:
    case ITEM_GROUP_UPDATED:
    case POLICY_CREATED:
    case POLICY_DELETED:
    case POLICY_UPDATED:
    case HUB_CLOUD_REGISTER_REPEAT_PROGRESS:
    case HUB_STORAGE_CONTROLLER_TOKEN_UPDATED:
    case HUB_STORAGE_CONTROLLER_TOKEN_EXPIRED:
    case MODES_SWINGER_SHUTDOWN_RESET_DONE:
    case MODES_SWINGER_SHUTDOWN_DISABLE_ADDED:
    case MODES_SWINGER_SHUTDOWN_DISABLE_REMOVED:
    case MODES_SWINGER_SHUTDOWN_LIMITS_CHANGED:
    case HUB_FEATURE_ADDED:
    case HUB_FEATURE_UPDATED:
    case HUB_FEATURE_REMOVED:
    case NETWORK_COMMUNICATION_TEST_PROGRESS:
    case HUB_OFFLINE_CLOUD_DATA_UPDATE:
    case HUB_OFFLINE_CLOUD_DATA_UPDATE_PROGRESS:
    case WALK_TEST_PROGRESS:
        Broadcast::sendToUis( static_cast< MessageSubClasses >( message.msgSubClass ), message.data );
        break;
    default:
        break;
    }
}

void Service::handleEventFromBus( const Services::Message& message ) {
    m_nmaClient->processBroadcast( message );
    m_uiExtensionsManager.handleEventFromBus( message.data, static_cast< MessageSubClasses >( message.msgSubClass ) );

    // Why should we filter it at all ????
    // what about just make the handler for sending ui_broadcasts
    switch( message.msgSubClass ) {
    case HUB_NETWORK_CONNECTED:
        LOGGER_DEBUG_RAW( cc::debug( "Internet connection established from " ) + cc::info( "Network manager" ) );
        addTask( [this]() {
            if( m_nmaClient ) {
                m_nmaClient->restoreConnection();
            }
        } );
        m_cloudStorage->setInternetAvailable( true );
        break;

    case HUB_NETWORK_DISCONNECTED:
        LOGGER_DEBUG_RAW( cc::debug( "Internet connection lost from " ) + cc::info( "Network manager" ) );
        addTask( [this]() {
            if( m_nmaClient ) {
                m_nmaClient->closeConnection();
            }
        } );
        m_cloudStorage->setInternetAvailable( false );
        break;

    case HUB_SYSTEM_DATETIME_CHANGED:
        LOGGER_INFO_RAW( cc::info( "System time changed" ) );
        addTask( [this]() {
            if( m_cloudStorage ) {
                LOGGER_DEBUG_RAW( cc::debug( "Reschedule CloudStorage tasks" ) );
                m_cloudStorage->stopTasks();
                m_cloudStorage->initStorage();
            }
        } );
        Broadcast::sendToUis( static_cast< MessageSubClasses >( message.msgSubClass ), message.data );
        break;

    case SERVICE_STARTED: {
        const std::string serviceId = json_get_str( message.data, "id" );
        if( serviceId == GroupsNames::HARDWARE_MANAGER ) {
            // Hardware manager service has just started, send actual led state.
            HardwareManager::RemoteInterface hwRemoteInterface( Uid::Service::getInstance() );
            if( m_nmaClient ) {
                if( m_nmaClient->isConnected() ) {
                    hwRemoteInterface.setCloudConnection( HardwareManager::HubState::CloudConnection::Connected );
                    Db::setFlag( DbFlags::NmaClientConnected );
                }
                else if( m_nmaClient->isConnecting() ) {
                    hwRemoteInterface.setCloudConnection( HardwareManager::HubState::CloudConnection::Connecting );
                }
                else {
                    hwRemoteInterface.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
                }
                break;
            }
            hwRemoteInterface.setCloudConnection( HardwareManager::HubState::CloudConnection::NotConnected );
        }
        break;
    }

    case NMA_CLIENT_CONNECTED: {
        m_connectionReporter->setNmaConnectionStatus( message.data );
        break;
    }
    case NMA_CLIENT_DISCONNECTED: {
        m_connectionReporter->setNmaDisconnectionStatus( message.data );
        break;
    }

    default:
        break;
    }
}

void Service::handleSystemInfoFromBus( const Services::Message& message ) {
    JsonObject result;
    std::string msg, type;

    if( json_get_obj( message.data, "result", result ) && json_get( result, "type", type ) &&
        json_get( result, "message", msg ) ) {
        if( type == "info" ) {
            if( isNeedToSendInfoEvent( msg, type ) ) {
                Broadcast::sendToAll( SYSTEM, result );
            }
        }
        else if( type == "sendDailyLog" ) {
            const JsonObject& joParams = msg.data();
            auto response = sendRequest( "backupd", joParams, SEND_DAILY_LOG );
            sendResponseToUis( SEND_DAILY_LOG, response );
        }
        else if( type == "sendTraceLog" ) {
            const JsonObject& joParams = msg.data();
            auto response = sendRequest( "backupd", joParams, SEND_TRACE_LOG );
            sendResponseToUis( SEND_TRACE_LOG, response );
        }
    }
}

void Service::handleLogCmdFromBus( const Services::Message& message ) {
    Services::Service::handleLogCommand( message, "" );
}

bool Service::handleAsyncRequestFromBus( const JsonObject& request,
                                         MessageSubClasses requestType,
                                         const std::string& uuid,
                                         const std::string& sender ) {
    switch( requestType ) {
    case ICC_REQUEST:
        m_iccManager.handleRequest( request, uuid, sender );
        return true;
    case ICC_REGISTER_CONTROLLER_CONNECTION:
        m_clusterManager.handleRegisterControllerConnection( request, sender );
        return true;
    case ICC_UNREGISTER_CONTROLLER_CONNECTION:
        m_clusterManager.handleUnregisterControllerConnection( request, sender );
        return true;
    case CLOUD_MESSAGE: {
        auto requestWithId = request;
        // NOTE: if request already contains an "id" filed, it will be ignored(overwritten),
        // so do not add any logic that depends on it.
        requestWithId[ "id" ] = sender + "|" + uuid;
        m_nmaClient->postMessage( requestWithId );
        return true;
    }
    default:
        return false;
    }
}

JsonObject Service::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    case BACKUP_CREATE: {
        CommonUtils::BackupHelper helper( getDBClient(), request );
        for( auto& tableName : BackupTables ) {
            if( !helper.backupDatabase( tableName ) ) {
                return helper.getResponse();
            }
        }
        return helper.getResponse();
    }
    default:
        auto answer = m_uiExtensionsManager.handleRequestFromBus( request, requestType );
        if( answer == unhandledRequestAnswer() ) {
            answer = HubPlatform::Services::Service::handleRequestFromBus( request, requestType );
        }
        return answer;
    }
}

int Service::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& tableName : BackupTables ) {
        if( !helper.restoreDatabase( tableName ) ) {
            return helper.getReturnCode();
        }
    }
    return helper.getReturnCode();
}

bool Service::isNeedToSendInfoEvent( const std::string& message, const std::string& type ) {
    bool isNeedToSend = true;

    if( type == "info" && ( message == "onBattery" || message == "onPowerLine" ) ) {
        // A workaround for ezlo onBattery can occur many times with the same state

        std::lock_guard< std::mutex > lock( m_mutex );

        if( m_previousSystemInfoMessage == message ) {
            isNeedToSend = false;
        }
        else {
            m_previousSystemInfoMessage = message;
        }
    }

    return isNeedToSend;
}

bool Service::sendResponseToUis( const MessageSubClasses eventType, const JsonObject& response ) {
    JsonObject result = JSON_EMPTY_OBJ;
    if( hasError( response ) ) {
        result[ "error" ] = std::move( response[ "error" ] );
        return false;
    }
    else {
        Broadcast::sendToUis( eventType, response );
    }
    return true;
}

NmaClientPtr Service::getNmaClient() {
    return m_nmaClient;
}

InterControllerCommunication::IccManager& Service::getIccManager() {
    return m_iccManager;
}

ClusterManager& Service::getClusterManager() {
    return m_clusterManager;
}

EnvironmentPtr Service::getEnvironment() {
    return m_environment;
}

void Service::injectApiBridge() {
    using namespace HubPlatform::HttpBridge;
    auto& apiBridge = getApiBridge();

    using UidApiBridgeHandlers = HttpBridge::UidApiBridgeHandlers< Service, HubOfflineCommandsHandler >;
    auto handlersPtr = std::make_unique< UidApiBridgeHandlers >( *this );
    apiBridge.resetHandlers( ApiBridgeHandlersPtr{ handlersPtr.release() } );

    auto camerasAddonEndpoint =
        static_cast< HttpBridge::CamerasAddonEndpoint* >( apiBridge.camerasAddonEndpoint().get() );
    camerasAddonEndpoint->setEndpointHandler( std::make_unique< CamerasEndpointHandler >( *this ) );
}

void Service::onLocalServerStarted( uint16_t securePort, uint16_t insecurePort ) {
    getDBClient().update( HaTempCacheDBName,
                          JSON_EMPTY_OBJ,
                          JSON( db::ID_FIELD_NAME << "local_server_started"
                                                  << "securePort" << securePort << "insecurePort" << insecurePort ),
                          true,
                          false );
    m_uiExtensionsManager.setUp();
}

void Service::registerCustomServices() {
    try {
        // TODO: that would be more convenient to have custom service configurations in separate config fileS
        ConfigReader::Path customServicesPath( m_configReader, ConfigKeys::CustomServices );
        const auto customServicesCount = ConfigReader::getArraySize( customServicesPath );

        for( auto i = 0; i < customServicesCount; ++i ) {
            std::string name;
            try {
                auto path = customServicesPath / i;

                name = ConfigReader::getString( path / "name" );
                auto requestDomain = ConfigReader::getString( path / "uid" / "requestDomain" );
                auto serviceName = ConfigReader::getString( path / "uid" / "spreadServiceName" );
                auto requestTimeout = ConfigReader::getInt( path / "uid" / "requestTimeout" );

                auto res = m_customServices.emplace( std::make_pair(
                    name, CustomService{ name, requestDomain, serviceName, std::chrono::seconds( requestTimeout ) } ) );
                if( !res.second ) {
                    Log::warn( cc::warn( "Custom service name duplication, only a first " ) + cc::note( name ) +
                               cc::warn( " custom service will be registered" ) );
                }
            }
            catch( const std::exception& ex ) {
                Log::error( cc::error( "Failed to register a " ) + cc::note( name ) +
                            cc::error( " custom service, reason: " ) + cc::note( ex.what() ) );
            }
            catch( ... ) {
                Log::error( cc::error( "Failed to register a " ) + cc::note( name ) + cc::error( " custom service" ) );
            }
        }
    }
    catch( const std::exception& ex ) {
        Log::error( cc::error( "Failed to register custom services, reason: " ) + cc::note( ex.what() ) );
    }
    catch( ... ) {
        Log::error( cc::error( "Failed to register custom services" ) );
    }
}

void Service::printRegisteredCustomServices( std::ostream& os ) const {
    os << cc::info( "Registered custom services" ) << cc::debug( "............................" ) << std::endl;
    for( const auto& pair : m_customServices ) {
        os << cc::debug( "...." ) << cc::debug( pair.second.m_name ) << std::endl;
    }
}

const Service::CustomService* Service::matchCustomService( const RequestDomain& requestDomain ) const {
    for( const auto& pair : m_customServices ) {
        const auto& service = pair.second;
        if( service.m_requestDomain == requestDomain ) {
            return &service;
        }
    }

    return nullptr;
}

int Service::checkRotateLogsStatus() {
    std::ifstream configFile( HubPlatform::Environment::getTempDataFolder() + "/logs_rotate_status" );
    if( !configFile.is_open() ) {
        LOGGER_TRACE_RAW( cc::trace( "Can't open the logs_rotate_status file" ) );
        return 0;
    }
    int value;
    if( !( configFile >> value ) ) {
        return 0;
    }
    configFile.close();
    return value;
}

int Service::resetRotateLogsStatus() {
    std::ofstream configFile( HubPlatform::Environment::getTempDataFolder() + "/logs_rotate_status" );
    if( !configFile.is_open() ) {
        LOGGER_TRACE_RAW( cc::trace( "Can't open the logs_rotate_status file" ) );
        return 0;
    }
    configFile << 0 << std::endl;
    configFile.close();
    return 0;
}

JsonObject Service::getServiceInfo() {
    Json info = JSON_EMPTY_OBJ;
    info[ "nma" ] = m_connectionReporter->updateAndGetCurrentNmaConnectionReport();
    info[ "httpServer" ] = HttpBridge::getApiBridge().getMetrics();

    if( auto& serverListenerUPtr = this->getServerListener() ) {
        auto status = "stopped";
        if( serverListenerUPtr->getServerOpenComplete() )
            status = "running";

        info[ "localMode" ] = { { "status", status } };
    }
    else {
        info[ "localMode" ] = { { "status", "not_started" } };
    }
    return info;
}

}  // namespace Uid
}  // namespace HubPlatform
