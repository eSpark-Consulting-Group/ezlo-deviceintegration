
#include "uid/service_settings.h"

#include "uid/ui_service.h"
#include "uid/stat_handlers.h"
#include "uid/nma_client.h"

#include "system_utils/filesystem.h"

#include "uid/environment.h"
#include "common_utils/hub_info_helper.h"

#include "config_reader/config_reader.h"

#include "log_utils/logsettings.h"

#include "base_utils/ezlodb.h"

#include <tiny/tiny_command_line_parser.h>

namespace HubPlatform {
namespace Uid {

ServiceSettings::ServiceSettings()
: m_nMainLoopLimit( 0 )  // main loop limit
, m_cntThreadsInDispatcher( 3 )
, m_queueLimitInDispatcher( 512 )
, m_dbMaxCollectionSize( 5000 )
, m_bIsNoBus( false )
, m_isNoDbUnlock( false )
, m_isNoLockedStateCheck( false )
, m_isNoConvertDataitemsToFOs( false )
, m_isNoReplication( false )
, m_bRandomLocalServerPort( false )
, m_explicitCloudCoordinatorQueryTimeoutInSeconds{ 10 }
, m_explicitCloudCoordinatorReconnectPeriodInSeconds{ 60 * 3 } {}

void ServiceSettings::defineCommandLineParameters( tiny::command_line::parser& parser ) {
    auto setValueIfFileExist = []( const std::string& value, std::string& destination ) {
        if( HubPlatform::filesystem::isFileExist( value ) ) {
            destination = value;
        }
        else {
            std::cout << "File is not exist and will be ignored: " << cc::p( value ) << std::endl;
            std::cout.flush();
        }
    };

    parser
        .on( "UI daemon\nezlo-serial",
             "Specifies serial number of virtual ezlo hub",
             [&]( const std::string& s ) {
                 HubInfoHelper::setExplicitSerial( s );
                 std::cout << "\"ezlo-serial\" was set to " << s << std::endl;
                 std::cout.flush();
             } )
        .on( "UI daemon\nhub-type",
             "Force use hub type instead of \"/etc/hubtype\" file (\"a20.1\" is big ezlo, \"8266.1\" is tiny atom).",
             [&]( const std::string& s ) {
                 HubInfoHelper::setExplicitHubType( s );
                 std::cout << "Explicit hub type is set to " << s << std::endl;
                 std::cout.flush();
             } )
        .on( "UI daemon\nno-bus", "Disable bus with other hub components", [&]() { m_bIsNoBus = true; } )
        .on( "Cloud\ncoordinator",
             "Specifies URL of cloud coordinator",
             [this]( const std::string& path ) {
                 m_explicitCloudCoordinatorURL.set( path );
                 std::cout << "Cloud coordinator URL was set to: " << cc::u( path ) << std::endl;
                 std::cout.flush();
             } )

        .on( "Cloud\ncoordinator-timeout",
             "Specifies timeout in seconds for cloud coordinator queries",
             [this]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 time_t tt = ( time_t )::strtoull( s.c_str(), &end_ptr, 10 );

                 m_explicitCloudCoordinatorQueryTimeoutInSeconds = tt;
                 std::cout << "cloud coordinator timeout was set to " << m_explicitCloudCoordinatorQueryTimeoutInSeconds
                           << std::endl;
                 std::cout.flush();
             } )
        .on( "Cloud\ncoordinator-reconnect",
             "Specifies reconnect period in seconds for cloud coordinator",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 time_t tt = ( time_t )::strtoull( s.c_str(), &end_ptr, 10 );
                 m_explicitCloudCoordinatorReconnectPeriodInSeconds = tt;
                 std::cout << "cloud coordinator reconnect period was set to "
                           << m_explicitCloudCoordinatorReconnectPeriodInSeconds << std::endl;
                 std::cout.flush();
             } )
        .on( "NMA connection\nnma-server",
             "Specifies URL of NMA server to connect to",
             [&]( const std::string& url ) {
                 m_explicitNmaServerURL.set( url );
                 std::cout << "\"nma-server\" URL was set to: " << cc::u( url ) << std::endl;
                 std::cout.flush();
             } )
        .on( "Random local server port\nrandom-port",
             "Choose some unused port for local server",
             [&]() { m_bRandomLocalServerPort = true; } )
        .on( "NMA connection\nws-timeout-open",
             "Specifies open handshake timeout (in seconds)",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 uint64_t n = std::strtoull( s.c_str(), &end_ptr, 10 );
                 tiny::ws::basic_network_settings::default_instance().timeout_handshake_open_ = n;
             } )
        .on( "NMA connection\nws-timeout-close",
             "Specifies close handshake timeout (in seconds)",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 uint64_t n = std::strtoull( s.c_str(), &end_ptr, 10 );
                 tiny::ws::basic_network_settings::default_instance().timeout_handshake_close_ = n;
             } )
        .on( "NMA connection\nws-restart-on-close",
             "Specifies client web socket restart time interval after close (in seconds)",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 uint64_t n = std::strtoull( s.c_str(), &end_ptr, 10 );
                 tiny::ws::basic_network_settings::default_instance().timeout_restart_on_close_ = n;
             } )
        .on( "NMA connection\nws-restart-on-fail",
             "Specifies client web socket restart time interval after fail (in seconds)",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 uint64_t n = std::strtoull( s.c_str(), &end_ptr, 10 );
                 tiny::ws::basic_network_settings::default_instance().timeout_restart_on_fail_ = n;
             } )
        .on( "NMA network query handling\nsth-msgs",
             "Single-threaded message handling in NMA client (use for debug only)",
             [&]() { NmaClient::g_bSingleThreadedPeerMessages = true; } )
        .on( "NMA network query handling\nmth-msgs",
             "Multi-threaded message handling in NMA client (default, very recommended)",
             [&]() { NmaClient::g_bSingleThreadedPeerMessages = false; } )
        .on( "NMA network query handling\nsth-answers",
             "Single-threaded answer sending (use for debug only)",
             [&]() { NmaClient::g_bSingleThreadedPeerAnswers = true; } )
        .on( "NMA network query handling\nmth-answers",
             "Multi-threaded answer sending (default, very recommended)",
             [&]() { NmaClient::g_bSingleThreadedPeerAnswers = false; } )
        .on( "Startup\ndb-unlock", "Unlock DB on startup", [&]() { m_isNoDbUnlock = false; } )
        .on( "Startup\nno-db-unlock", "Do not unlock DB on startup", [&]() { m_isNoDbUnlock = true; } )
        .on( "UI daemon\ndb-capping",
             "DB collection size limit (default is 5000, set to 0 to disable capping)",
             [&]( const std::string& s ) {
                 m_dbMaxCollectionSize = ::atoi( s.c_str() );
                 std::cout << "\"db-capping\" was set to " << m_dbMaxCollectionSize << std::endl;
                 std::cout.flush();
             } )
        .on( "Startup\nlss", "Perform locked state check on startup", [&]() { m_isNoLockedStateCheck = false; } )
        .on( "Startup\nno-lss",
             "Do not perform locked state check on startup",
             [&]() { m_isNoLockedStateCheck = true; } )
        .on( "Startup\ndi2fo", "Convert data items to FOs on startup", [&]() { m_isNoConvertDataitemsToFOs = false; } )
        .on( "Startup\nno-di2fo",
             "Do not convert data items to FOs on startup",
             [&]() { m_isNoConvertDataitemsToFOs = true; } )
        .on( "Runtime\nenable-replication", "Enable replication thread", [&]() { m_isNoReplication = false; } )
        .on( "Runtime\ndisable-replication", "Disable replication thread", [&]() { m_isNoReplication = true; } )
        .on( "Logging\nmax-log-message-size",
             "Limit size of log message",
             [&]( const std::string& s ) {
                 size_t nMaxLogMessageSize = size_t(::atoi( s.c_str() ) );
                 ezlo::log_settings::maxLogMessageSizeSet( nMaxLogMessageSize );
                 size_t nEffectiveMaxLogMessageSize = ezlo::log_settings::maxLogMessageSizeGet();
                 if( nEffectiveMaxLogMessageSize != nMaxLogMessageSize ) {
                     std::cout << "\"max-log-message-size\" was set to " << nEffectiveMaxLogMessageSize << std::endl;
                     std::cout.flush();
                 }
             } )
        .on( "Logging\nlog-directory",
             "Specifies log directory",
             [&]( const std::string& s ) {
                 std::string strLogDirectoryOld = ezlo::log_settings::getLogDirectory();
                 ezlo::log_settings::setLogDirectory( s );
                 if( strLogDirectoryOld != s ) {
                     std::cout << "\"log-directory\" was set to \"" << s << "\"" << std::endl;
                     std::cout.flush();
                 }
             } )
        .on(
            "UI daemon\nfriendly",
            "Turns on friendly debugging mode",
            [&]() {
                cc::g_nJsonStringValueOutputSizeLimit = 512;
                cc::_default_json_indent_ = 4;
                if( !cc::_on_ ) {
                    cc::_on_ = true;
                    std::cout << cc::reset();
                    std::cout.flush();
                }
                ezlo::log_settings::maxLogMessageSizeSet( 1024 * 1024 * 100 );
                // Log::g_bFriendlyMode = true;
                Log::printAllLogsToConsole( true );
        // NmaClient::g_bEnableNmaClientStatsLogging = true;
        //
#if( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
                tiny::ws::basic_network_settings::default_instance().ssl_server_require_valid_certificate_ = false;
                tiny::ws::basic_network_settings::default_instance().ssl_server_allow_non_ssl_on_ssl_port_ = true;
                tiny::ws::basic_network_settings::default_instance().ssl_server_accept_connections_without_valid_cert_ =
                    true;
                tiny::ws::basic_network_settings::default_instance().ssl_client_allow_self_signed_ = true;
                tiny::ws::basic_network_settings::default_instance().ssl_client_skip_host_name_check_ = true;
                tiny::ws::basic_network_settings::default_instance().ssl_client_allow_expired_ = true;
#endif  // ( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
            } )
        .on( "Logging\nconn-stats",
             "Use connection statistics logging in main NMA server connection.",
             [&]() { NmaClient::g_bEnableNmaClientStatsLogging = true; } )
        .on( "Logging\nno-conn-stats",
             "Do not use connection statistics logging in main NMA server connection.",
             [&]() { NmaClient::g_bEnableNmaClientStatsLogging = false; } )
        .on( "Runtime\nmll",
             "Main loop limit counter (for debugging)",
             [&]( const std::string& s ) { m_nMainLoopLimit = size_t(::atoi( s.c_str() ) ); } )
        .on( "Runtime\nthreads",
             "Thread count (if 0, use processor cores count)",
             [&]( const std::string& s ) {
                 size_t n = size_t(::atoi( s.c_str() ) ), cntCPUs = tiny::tools::cpu_count();
                 if( n == 0 )
                     n = size_t( cntCPUs );
                 if( n < 1 )
                     n = 1;
                 if( n > 50 )
                     n = 50;
                 m_cntThreadsInDispatcher = n;
                 std::cout << "Thread count was set to " << m_cntThreadsInDispatcher << "(CPU core(s) count is"
                           << cntCPUs << ")" << std::endl;
                 std::cout.flush();
             } )
        .on( "Runtime\nqueue",
             "Call queue limit (if 0, then no limit)",
             [&]( const std::string& s ) {
                 size_t n = size_t(::atoi( s.c_str() ) );
                 m_queueLimitInDispatcher = n;
                 std::cout << "Call queue limit was set to " << m_queueLimitInDispatcher << std::endl;
                 std::cout.flush();
             } )
        //
        .on( "NMA security\ncrt",
             "Specifies NMA certificate file(default is \"/opt/firmware/data/certs/cloud.crt\")",
             [&]( const std::string& s ) { setValueIfFileExist( s, m_certificateFile ); } )
        .on( "NMA security\nca",
             "Specifies NMA certification chain file(default is \"/opt/firmware/data/certs/cloud.ca\")",
             [&]( const std::string& s ) { setValueIfFileExist( s, m_certificationChainFile ); } )
        .on( "NMA security\nkey",
             "Specifies NMA private key file(default is \"/opt/firmware/data/certs/cloud.key\")",
             [&]( const std::string& s ) { setValueIfFileExist( s, m_privateKeyFile ); } )
        //
        .on( "Web socket operational\nws-max-message",
             "Specifies maximal message size (in bytes, 0 is unlimited)",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 uint64_t n = std::strtoull( s.c_str(), &end_ptr, 10 );
                 tiny::ws::basic_network_settings::default_instance().max_message_size_ = n;
             } )
        .on( "Web socket operational\nws-max-body",
             "Specifies maximal message body size (in bytes, 0 is unlimited)",
             [&]( const std::string& s ) {
                 char* end_ptr = nullptr;
                 uint64_t n = std::strtoull( s.c_str(), &end_ptr, 10 );
                 tiny::ws::basic_network_settings::default_instance().max_body_size_ = n;
             } )
        .on( "Logging\nws-log",
             "Specifies web socket protocol logging level (none, basic, detailed)",
             [&]( const std::string& s ) {
                 tiny::ws::g_eWSLL = tiny::ws::str2wsll( s );
                 std::cout << "Web socket protocol logging level was set to " << tiny::ws::wsll2str( tiny::ws::g_eWSLL )
                           << std::endl;
                 std::cout.flush();
             } )
        .on( "Logging\nws-log-traffic",
             "Enable web socket traffic logging",
             [&]() { tiny::ws::basic_network_settings::default_instance().log_ws_rx_tx_ = true; } )
        .on( "Logging\nno-ws-log-traffic",
             "Disable web socket traffic logging",
             [&]() { tiny::ws::basic_network_settings::default_instance().log_ws_rx_tx_ = false; } )
#if( defined __HAVE_TINY_WS_BACKEND_NLWS__ )
        .on( "Web socket server\nnlws-server-mode",
             "Specifies web socket server mode (" + tiny::ws::nlws::list_srvmodes_as_str() + ")",
             [&]( const std::string& s ) { tiny::ws::nlws::g_default_srvmode = tiny::ws::nlws::str2srvmode( s ); } )
        .on( "Web socket server\nnlws-explicit-vhost",
             "Use NLWS explicit vhost (simple and external poll)",
             [&]( const std::string& s ) {
                 tiny::ws::nlws::g_default_explicit_vhost_enable = tiny::tools::to_bool( s );
             } )
        .on( "Web socket server\nnlws-dynamic-vhost",
             "NLWS dynamic vhosts (external poll)",
             [&]( const std::string& s ) {
                 tiny::ws::nlws::g_default_dynamic_vhost_enable = tiny::tools::to_bool( s );
             } )
#endif  /// (defined __HAVE_TINY_WS_BACKEND_NLWS__)
#if( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
        .on( "Web socket server\nno-6",
             "Disable IP v6",
             [&]() { tiny::ws::basic_network_settings::default_instance().server_disable_ipv6_ = true; } )
        .on( "Web socket server\n6",
             "Enable IP v6",
             [&]() { tiny::ws::basic_network_settings::default_instance().server_disable_ipv6_ = false; } )
        .on( "Web socket server\nvalidate-utf8",
             "Enable server side UTF-8 validation",
             [&]() { tiny::ws::basic_network_settings::default_instance().server_validate_utf8_ = true; } )
        .on( "Web socket server\nno-validate-utf8",
             "Disable server side UTF-8 validation",
             [&]() { tiny::ws::basic_network_settings::default_instance().server_validate_utf8_ = false; } )
        .on( "Web socket server\nno-canonocal-host-name-check",
             "Disable host canonical name check on server side",
             [&]() { tiny::ws::basic_network_settings::default_instance().server_skip_canonical_name_ = true; } )
        .on( "Web socket server\ncanonocal-host-name-check",
             "Enable host canonical name check on server side",
             [&]() { tiny::ws::basic_network_settings::default_instance().server_skip_canonical_name_ = false; } )
        .on( "Web socket operational\nssl-global-init",
             "Take care about SSL global initialization",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_perform_global_init_ = true; } )
        .on( "Web socket operational\nno-ssl-global-init",
             "Do not take care about SSL global initialization",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_perform_global_init_ = false; } )
        .on( "Web socket server\nrequire-valid-cert",
             "Do not allow the connection unless the client has a valid and recognizable certifcate",
             [&]() {
                 tiny::ws::basic_network_settings::default_instance().ssl_server_require_valid_certificate_ = true;
             } )
        .on( "Web socket server\nno-require-valid-cert",
             "Allow the connection for clients with badly validated certifcates",
             [&]() {
                 tiny::ws::basic_network_settings::default_instance().ssl_server_require_valid_certificate_ = false;
             } )
        .on( "Web socket server\nenable-non-ssl-on-ssl",
             "Enable non-ssl connections on ssl port",
             [&]() {
                 tiny::ws::basic_network_settings::default_instance().ssl_server_allow_non_ssl_on_ssl_port_ = true;
             } )
        .on( "Web socket server\ndisable-non-ssl-on-ssl",
             "Disable non-ssl connections on ssl port",
             [&]() {
                 tiny::ws::basic_network_settings::default_instance().ssl_server_allow_non_ssl_on_ssl_port_ = false;
             } )
        .on(
            "Web socket server\naccept-self-signed",
            "Accept connections with no valid certificate (i.e., selfsigned)",
            [&]() {
                tiny::ws::basic_network_settings::default_instance().ssl_server_accept_connections_without_valid_cert_ =
                    true;
            } )
        .on(
            "Web socket server\nreject-self-signed",
            "Reject connections with no valid certificate (i.e., selfsigned)",
            [&]() {
                tiny::ws::basic_network_settings::default_instance().ssl_server_accept_connections_without_valid_cert_ =
                    false;
            } )
        .on( "Web socket server\nhttp-to-https",
             "Enable http to https redirecation",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_server_redirect_http_2_https_ = true; } )
        .on( "Web socket server\nno-http-to-https",
             "Disable http to https redirecation",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_server_redirect_http_2_https_ = false; } )
        //
        .on( "Web socket client\nenable-self-signed",
             "Enable selfsigned certificate on client side",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_client_allow_self_signed_ = true; } )
        .on( "Web socket client\ndisable-self-signed",
             "Disable selfsigned certificate on client side",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_client_allow_self_signed_ = false; } )
        .on( "Web socket client\nskip-host-name-check",
             "Skip host name check",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_client_skip_host_name_check_ = true; } )
        .on( "Web socket client\nno-skip-host-name-check",
             "Do not skip host name check",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_client_skip_host_name_check_ = false; } )
        .on( "Web socket client\nenable-expired-ssl-client",
             "Enable expired certificates on client side",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_client_allow_expired_ = true; } )
        .on( "Web socket client\ndisable-expired-ssl-client",
             "Disable expired certificates on client side",
             [&]() { tiny::ws::basic_network_settings::default_instance().ssl_client_allow_expired_ = false; } )
        .on( "Web socket client\nws-max-send-portion",
             "Maximal count of bytes to send as message portion(experimental option, 0 is unlimited)",
             [&]( const std::string& s ) {
                 tiny::ws::basic_network_settings::default_instance().cntMaxPortionBytesToSend_ = ::atoi( s.c_str() );
             } )
#endif  /// if( defined __TINY_WS_OFFER_DETAILED_NLWS_CONFIGURATION_OPTIONS__ )
        .on( "Formatting\ncolors",
             "Enable colorized output",
             [&]() {
                 if( !cc::_on_ ) {
                     cc::_on_ = true;
                     std::cout << cc::reset();
                     std::cout.flush();
                 }
             } )
        .on( "Formatting\nno-colors",
             "Disable colorized output",
             [&]() {
                 if( cc::_on_ ) {
                     std::cout << cc::reset();
                     std::cout.flush();
                     cc::_on_ = false;
                 }
             } )
        .on( "Formatting\ncolor-reset-mode",
             "Reset mode during colorized output(full/wide/zero/colors/emulation)",
             [&]( const std::string& s ) { cc::g_eResetMode = cc::str_2_reset_mode( s ); } )
        .on( "Formatting\njson-indent",
             "Specifies JSON indentation in number of spaces",
             [&]( const std::string& s ) { cc::_default_json_indent_ = ::atoi( s.c_str() ); } )
        .on( "Formatting\njson-output-string-limit",
             "Limit output size of string values (0 is unlimited, default)",
             [&]( const std::string& s ) { cc::g_nJsonStringValueOutputSizeLimit = ::atoi( s.c_str() ); } );
}

bool ServiceSettings::isNoBus() const {
    return m_bIsNoBus;
}

size_t ServiceSettings::mainLoopLimit() const {
    return m_nMainLoopLimit;
}

size_t ServiceSettings::cntThreadsInDispatcher() const {
    return m_cntThreadsInDispatcher;
}

size_t ServiceSettings::queueLimitInDispatcher() const {
    return m_queueLimitInDispatcher;
}

bool ServiceSettings::isNoDbUnlock() const {
    return m_isNoDbUnlock;
}

int ServiceSettings::dbMaxCollectionSize() const {
    return m_dbMaxCollectionSize;
}

bool ServiceSettings::isNoLockedStateCheck() const {
    return m_isNoLockedStateCheck;
}

bool ServiceSettings::isNoConvertDataitemsToFOs() const {
    return m_isNoConvertDataitemsToFOs;
}

bool ServiceSettings::isNoReplication() const {
    return m_isNoReplication;
}

bool ServiceSettings::isRandomLocalServerPort() const {
    return m_bRandomLocalServerPort;
}

const std::string& ServiceSettings::getCertificateFile() const {
    return m_certificateFile;
}

const std::string& ServiceSettings::getPrivateKeyFile() const {
    return m_privateKeyFile;
}

const std::string& ServiceSettings::getCertificationChainFile() const {
    return m_certificationChainFile;
}

std::string ServiceSettings::getExplicitCloudCoordinatorURL() const {
    std::string coordinatorUrl;
    m_explicitCloudCoordinatorURL.get( coordinatorUrl );
    return coordinatorUrl;
}

std::string ServiceSettings::getExplicitNmaServerURL() const {
    std::string nmaUrl;
    m_explicitNmaServerURL.get( nmaUrl );
    return nmaUrl;
}

time_t ServiceSettings::getExplicitCloudCoordinatorQueryTimeoutInSeconds() const {
    return m_explicitCloudCoordinatorQueryTimeoutInSeconds;
}

time_t ServiceSettings::getExplicitCloudCoordinatorReconnectPeriodInSeconds() const {
    return m_explicitCloudCoordinatorReconnectPeriodInSeconds;
}

void ServiceSettings::setExplicitNmaServerURL( const std::string& url ) {
    m_explicitNmaServerURL.set( url );
}

void ServiceSettings::setExplicitCloudCoordinatorURL( const std::string& url ) {
    m_explicitCloudCoordinatorURL.set( url );
}

void ServiceSettings::setProvisioningDataState( ServiceSettings::ProvisioningState state ) {
    m_provisioningState = state;
}

ServiceSettings::ProvisioningState ServiceSettings::getProvisioningDataState() const {
    return m_provisioningState;
}

}  // namespace Uid
}  // namespace HubPlatform
