#include "data_sender.h"
#include "udp_socket.h"
#include "reporting_schema_version.h"

#include "json_utils/json_utils.h"
#include "resources/metrics_resources.h"
#include "common_utils/firmware_info_helper.h"
#include "common_utils/hub_info_helper.h"
#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "service_metrics_data/ssh_tunnel_data/ssh_tunnel_data.h"
#include "service_metrics_data/hub_errors_table/hub_errors_table.h"
#include "service_metrics_data/ping_statistics_data/ping_statistics_data.h"
#include "defaults.h"

#include <map>
#include <cstdint>

#include <unistd.h>

namespace HubPlatform {
namespace Metrics {

static void fillPingStatistics(::NetworkInterface& interface, const ServiceMetricsData::PingStatisticsData& data ) {
    auto* statsReport = interface.mutable_ping_statistics();
    auto* externalStatsReport = statsReport->mutable_external_stats();
    auto* localStatsReport = statsReport->mutable_local_stats();

    auto updateGenericStats = [](::PingStatistics_GenericStats* report,
                                 const ServiceMetricsData::PingStatisticsData::GenericStats& data ) {
        report->set_hits( data.hits );
        report->set_fails( data.fails );
        report->set_sequential_fails( data.sequentialFails );
        report->set_packets_sent( data.packetsSent );
        report->set_packets_received( data.packetsReceived );
        report->set_packets_lost( data.packetsLost );
    };

    updateGenericStats( externalStatsReport, data.externalStats );
    updateGenericStats( localStatsReport, data.localStats );

    for( auto& host : data.hosts ) {
        auto* report = statsReport->add_hosts();
        report->set_host_url( host.hostUrl );

        using HostLocation = ServiceMetricsData::PingStatisticsData::HostStats::HostLocation;
        if( host.hostLocation == HostLocation::EXTERNAL ) {
            report->set_host_location(::PingStatistics_HostStats_HostLocation_HOST_LOCATION_EXTERNAL );
        }
        else {
            report->set_host_location(::PingStatistics_HostStats_HostLocation_HOST_LOCATION_LOCAL );
        }
        report->set_host_description( host.hostDescription );

        report->set_hits( host.hits );
        report->set_fails( host.fails );
        report->set_packets_sent( host.packetsSent );
        report->set_packets_received( host.packetsReceived );
        report->set_packets_lost( host.packetsLost );
        report->set_round_trip_min( host.roundTripMin );
        report->set_round_trip_avg( host.roundTripAvg );
        report->set_round_trip_max( host.roundTripMax );
        report->set_timestamp_min( host.timestampMin );
        report->set_timestamp_max( host.timestampMax );
    }
}

DataSender::DataSender( InternalMetricsPtr metricsPtr )
: m_protobufVersion( PROACTIVE_REPORTING_PROTOBUF_SCHEMA_VERSION )
, m_sendInterval( DefaultSendIntervalNetworkDefault )
, m_internalMetrics( std::move( metricsPtr ) ) {
    m_serverPortIndex = 0;
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_digests();
}

DataSender::~DataSender() {
    if( m_clientPrivateKey ) {
        EVP_PKEY_free( m_clientPrivateKey );
    }

    if( m_serverPublicKey ) {
        EVP_PKEY_free( m_serverPublicKey );
    }

    EVP_cleanup();

    google::protobuf::ShutdownProtobufLibrary();
    m_internalMetrics.reset();
}

void DataSender::setServerUrlAndPort( const std::string& newServerUrl, std::vector< size_t > newServerPorts ) {
    m_serverUrl = newServerUrl;
    m_serverPorts = std::move( newServerPorts );
}

void DataSender::setClientCertificate( const std::string& clientCertificate, CERTIFICATE_SOURCE sourceType ) {
    if( !m_isClientCrtBytesRead ) {
        readClientCrtBytes( clientCertificate, sourceType );
    }
}

void DataSender::setClientPrivateKey( const std::string& clientPrivateKey, CERTIFICATE_SOURCE sourceType ) {
    if( !m_clientPrivateKeyRead ) {
        m_clientPrivateKey = readPrivateKey( clientPrivateKey, sourceType );
    }
}

void DataSender::setServerPublicKey( const std::string& serverPublicKey, CERTIFICATE_SOURCE sourceType ) {
    if( !m_serverPublicKeyRead ) {
        m_serverPublicKey = readPublicKey( serverPublicKey, sourceType );
    }
}

void DataSender::setCertifications( const std::string& serverCrt,
                                    const std::string& clientCrt,
                                    const std::string& clientKey ) {
    m_serverCrtFile = serverCrt;
    m_clientCrtFile = clientCrt;
    m_clientKeyFile = clientKey;

    setClientCertificate( m_clientCrtFile, FILE_SOURCE );
    setClientPrivateKey( m_clientKeyFile, FILE_SOURCE );
    setServerPublicKey( m_serverCrtFile, FILE_SOURCE );
}

void DataSender::setSendInterval( const uint64_t& interval ) {
    m_sendInterval = interval;
}

EVP_PKEY* DataSender::readPrivateKey( const std::string& keySource, CERTIFICATE_SOURCE sourceType ) {
    EVP_PKEY* privateKey = nullptr;
    BioPtr bo( nullptr, &BIO_free_all );

    switch( sourceType ) {
    case FILE_SOURCE:
        bo = BioPtr( BIO_new( BIO_s_file() ), &BIO_free_all );
        if( bo == nullptr ) {
            Log::error( cc::error( "Error while creating bo for sile source" ) );
            return nullptr;
        }
        BIO_read_filename( bo.get(), keySource.c_str() );
        break;
    case STRING_SOURCE:
        bo = BioPtr( BIO_new( BIO_s_mem() ), &BIO_free_all );
        if( bo == nullptr ) {
            Log::error( cc::error( "Error while creating bo for string source" ) );
            return nullptr;
        }
        BIO_write( bo.get(), keySource.c_str(), keySource.length() );
        break;
    default:
        Log::error( cc::error( "Invalid private key source!" ) );
        break;
    }

    PEM_read_bio_PrivateKey( bo.get(), &privateKey, nullptr, nullptr );

    m_clientPrivateKeyRead = true;
    return privateKey;
}

EVP_PKEY* DataSender::readPublicKey( const std::string& keySource, CERTIFICATE_SOURCE sourceType ) {
    BioPtr bo( nullptr, &BIO_free_all );
    X509Ptr crt( nullptr, X509_free );
    EVP_PKEY* publicKey = nullptr;

    switch( sourceType ) {
    case FILE_SOURCE:
        bo = BioPtr( BIO_new( BIO_s_file() ), &BIO_free_all );
        if( bo == nullptr ) {
            Log::error( cc::error( "Error while creating bo for file source" ) );
            return nullptr;
        }
        BIO_read_filename( bo.get(), keySource.c_str() );
        break;
    case STRING_SOURCE:
        bo = BioPtr( BIO_new( BIO_s_mem() ), &BIO_free_all );
        if( bo == nullptr ) {
            Log::error( cc::error( "Error while creating bo for string source" ) );
            return nullptr;
        }
        BIO_write( bo.get(), keySource.c_str(), keySource.length() );
        break;
    default:
        Log::error( cc::error( "Invalid public key source!" ) );
        break;
    }

    crt = X509Ptr( PEM_read_bio_X509( bo.get(), nullptr, nullptr, nullptr ), &X509_free );
    publicKey = X509_get_pubkey( crt.get() );

    m_serverPublicKeyRead = true;
    return publicKey;
}

void DataSender::readClientCrtBytes( const std::string& keySource, CERTIFICATE_SOURCE sourceType ) {
    BioPtr bo = BioPtr( nullptr, &BIO_free_all );
    // X509* x509 = nullptr;
    X509Ptr x509( nullptr, &X509_free );
    unsigned char* readData = nullptr;

    switch( sourceType ) {
    case FILE_SOURCE:
        bo = BioPtr( BIO_new( BIO_s_file() ), &BIO_free_all );
        BIO_read_filename( bo.get(), keySource.c_str() );
        break;
    case STRING_SOURCE:
        bo = BioPtr( BIO_new( BIO_s_mem() ), &BIO_free_all );
        BIO_write( bo.get(), keySource.c_str(), keySource.length() );
        break;
    default:
        Log::error( cc::error( "Invalid client certificate source!" ) );
        break;
    }

    x509 = X509Ptr( PEM_read_bio_X509_AUX( bo.get(), nullptr, nullptr, nullptr ), &X509_free );
    m_clientCrtBytesLen = i2d_X509( x509.get(), &readData );

    if( readData != nullptr ) {
        m_clientCrtBytes = ucharToStdString( readData, m_clientCrtBytesLen );
        delete readData;
        m_isClientCrtBytesRead = true;
    }
    else {
        Log::error( cc::error( "Client Certification files was not open" ) );
    }
}

bool DataSender::encryptAndSignMessage( const std::string& serializedMessage, MetricsWire& wireOut ) {
    // signature
    size_t msgLen = serializedMessage.size();
    EvpMdCtxPtr ctxRSASign( EVP_MD_CTX_create(), EVP_MD_CTX_destroy );
    if( EVP_DigestSignInit( ctxRSASign.get(), nullptr, EVP_sha256(), nullptr, m_clientPrivateKey ) <= 0 ) {
        Log::error( cc::error( "EVP_DigestSignInit error" ) );
        return false;
    }
    if( EVP_DigestSignUpdate( ctxRSASign.get(), serializedMessage.data(), msgLen ) <= 0 ) {
        Log::error( cc::error( "EVP_DigestSignUpdate error" ) );
        return false;
    }
    size_t signatureLen;
    if( EVP_DigestSignFinal( ctxRSASign.get(), nullptr, &signatureLen ) <= 0 ) {
        Log::error( cc::error( "EVP_DigestSignFinal error" ) );
        return false;
    }
    std::string signature( signatureLen, 0 );
    if( EVP_DigestSignFinal( ctxRSASign.get(), (unsigned char*) signature.data(), &signatureLen ) <= 0 ) {
        Log::error( cc::error( "EVP_DigestSignFinal error" ) );
        return false;
    }

    // encrypt key
    EvpPkeyCtxPtr ctx( EVP_PKEY_CTX_new( m_serverPublicKey, nullptr ), EVP_PKEY_CTX_free );
    if( ctx == nullptr ) {
        Log::error( cc::error( "EVP_PKEY_CTX_new error" ) );
        return false;
    }
    if( EVP_PKEY_encrypt_init( ctx.get() ) <= 0 ) {
        Log::error( cc::error( "EVP_PKEY_encrypt_init error" ) );
        return false;
    }

    int keyLen = m_AES256keySize;
    std::string key = generateRandString( keyLen );
    int size = RSA_size( m_serverPublicKey->pkey.rsa );
    std::string encryptedKey( size, 0 );
    size_t encryptedKeyLen = encryptedKey.size();

    if( EVP_PKEY_CTX_set_rsa_padding( ctx.get(), RSA_PKCS1_OAEP_PADDING ) <= 0 ) {
        Log::error( cc::error( "EVP_PKEY_CTX_set_rsa_padding error" ) );
        return false;
    }
    if( EVP_PKEY_CTX_set_rsa_oaep_md( ctx.get(), EVP_sha256() ) <= 0 ) {
        Log::error( cc::error( "EVP_PKEY_CTX_set_rsa_oaep_md error" ) );
        return false;
    }
    if( EVP_PKEY_CTX_set_rsa_mgf1_md( ctx.get(), EVP_sha256() ) <= 0 ) {
        Log::error( cc::error( "EVP_PKEY_CTX_set_rsa_mgf1_md error" ) );
        return false;
    }
    if( EVP_PKEY_encrypt(
            ctx.get(), (unsigned char*) encryptedKey.data(), &encryptedKeyLen, toBytePointer( key ), keyLen ) <= 0 ) {
        Log::error( cc::error( "EVP_PKEY_encrypt error" ) );
        return false;
    }
    encryptedKey.resize( encryptedKeyLen );

    // encrypt message
    std::string iv = generateRandString( m_ivSize );
    int maxSize = msgLen + m_AES256keySize;
    std::string encryptedBytes( maxSize, 0 );
    int encryptedBytesLen = encrypt( toBytePointer( serializedMessage ),
                                     serializedMessage.size(),
                                     toBytePointer( key ),
                                     toBytePointer( iv ),
                                     (unsigned char*) encryptedBytes.data() );
    encryptedBytes.resize( encryptedBytesLen );
    // -------------------------------------------------------------------------

    // set output struct
    wireOut.signature = std::move( signature );
    wireOut.encryptedKey = std::move( encryptedKey );
    wireOut.encryptedBytes = std::move( encryptedBytes );
    wireOut.iv = std::move( iv );

    return true;
}

void DataSender::jsonToReportSystem( const Json& json, Report& report ) {
    float valFloat;
    uint64_t valUint64;
    int64_t valInt64;
    std::string valStr;
    JsonObject tmpJson;

    try {
        // Report -> System
        if( json_get( json, { Resources::DataSender::system, Resources::DataSender::TimeFields::time }, tmpJson ) ) {
            valUint64 = 0;
            if( json_get( tmpJson, Resources::DataSender::ServiceFields::uptime, valUint64 ) ) {
                report.mutable_system()->set_uptime( valUint64 );
            }
        }

        if( json_get( json, { Resources::DataSender::system, Resources::DataSender::loadAverage }, tmpJson ) ) {
            if( json_get( tmpJson, Resources::DataSender::load1, valFloat ) )
                report.mutable_system()->mutable_load_average()->set_load_1( valFloat );
            if( json_get( tmpJson, Resources::DataSender::load5, valFloat ) )
                report.mutable_system()->mutable_load_average()->set_load_5( valFloat );
            if( json_get( tmpJson, Resources::DataSender::load15, valFloat ) )
                report.mutable_system()->mutable_load_average()->set_load_15( valFloat );
        }

        if( json_get( json, { Resources::DataSender::system, Resources::DataSender::memory }, tmpJson ) ) {
            if( json_get( tmpJson, "total", valInt64 ) )
                report.mutable_system()->mutable_memory()->set_total( valInt64 );

            if( json_get( tmpJson, "available", valInt64 ) )
                report.mutable_system()->mutable_memory()->set_available( valInt64 );

            if( json_get( tmpJson, Resources::DataSender::free, valInt64 ) )
                report.mutable_system()->mutable_memory()->set_free( valInt64 );

            if( json_get( tmpJson, Resources::DataSender::cached, valInt64 ) ) {
                report.mutable_system()->mutable_memory()->set_cached( valInt64 );
            }
        }

        if( json_get( json, { Resources::DataSender::system, Resources::DataSender::fileSystem }, tmpJson ) ) {
            for( auto it = tmpJson.begin(); it != tmpJson.end(); ++it ) {
                std::string mountPoint = it.key();

                auto* filesystem = report.mutable_system()->add_filesystems();
                filesystem->set_id( mountPoint );

                // WARN: ubus does json convertion to its own implementation that does not support 64bit integers

                auto total = json_get_str( it.value(), Resources::DataSender::totalSpace );
                filesystem->set_total_space( ( !total.empty() ) ? std::stoull( total ) : 0 );

                auto available = json_get_str( it.value(), Resources::DataSender::availableSpace );
                filesystem->set_available_space( ( !available.empty() ) ? std::stoull( available ) : 0 );
            }
        }

        if( json_get( json, Resources::DataSender::system, tmpJson ) ) {
            if( json_get( tmpJson, Resources::DataSender::oomKill, valUint64 ) )
                report.mutable_system()->set_oom_kill_count( valUint64 );
            if( json_get( tmpJson, Resources::DataSender::rebootCount, valUint64 ) )
                report.mutable_system()->set_reboot_count( valUint64 );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create system report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to create system report. Unknown exception" ) );
    }
}

void DataSender::jsonToReportNetwork( const Json& json, Report& report ) {
    Json networkObject;
    JsonObject modemInfo;
    JsonObject sshTunnels;
    JsonObject signalJson;

    JsonObject valJson;
    std::string valStr;
    uint64_t valUint64;
    int64_t valInt64;
    bool valBool;

    std::map< std::string, ServiceMetricsData::PingStatisticsData > pingStatistics;

    try {
        if( json_get( json, Resources::DataSender::network, networkObject ) ) {
            if( json_get( networkObject, Resources::DataSender::ping_statistics, valJson ) ) {
                for( auto& stats : valJson ) {
                    auto interfaceName = json_get_str( stats, "interface" );
                    pingStatistics.emplace( std::move( interfaceName ),
                                            ServiceMetricsData::PingStatisticsData::fromJson( stats ) );
                }
            }

            for( auto it = networkObject.begin(); it != networkObject.end(); ++it ) {
                auto interfaceReport = report.mutable_network()->add_interfaces();

                // string name = 1;
                interfaceReport->set_name( it.key() );

                Json interfaceJson = it.value();
                const std::string deviceName = json_get_str( interfaceJson, Resources::DataSender::deviceName );

                if( !deviceName.empty() ) {
                    if( deviceName.compare( 0, 3, "eth" ) == 0 ) {
                        interfaceReport->set_type( NetworkInterface_Type_TYPE_ETHERNET );
                        if( json_get( networkObject, Resources::DataSender::EthernetInterfaceUptime, valInt64 ) ) {
                            interfaceReport->set_uptime( valInt64 );
                        }
                    }
                    else if( deviceName.compare( 0, 4, "wlan" ) == 0 ) {
                        interfaceReport->set_type( NetworkInterface_Type_TYPE_WIFI );
                        if( json_get( networkObject, Resources::DataSender::WifiStaInterfacesUptime, valInt64 ) ) {
                            interfaceReport->set_uptime( valInt64 );
                        }
                    }
                    else if( deviceName.compare( 0, 6, "mobile" ) == 0 ) {
                        interfaceReport->set_type( NetworkInterface_Type_TYPE_MOBILE );
                        if( json_get( networkObject, Resources::DataSender::MobileInterfacesUptime, valInt64 ) ) {
                            interfaceReport->set_uptime( valInt64 );
                        }
                    }
                    else {
                        interfaceReport->set_type( NetworkInterface_Type_TYPE_UNKNOWN );
                    }
                }

                // Status status = 2;
                if( json_get( interfaceJson, Resources::DataSender::interfaceStatus, valBool ) ) {
                    interfaceReport->set_status( valBool ? NetworkInterface_Status_STATUS_CONNECTED :
                                                           NetworkInterface_Status_STATUS_DISCONNECTED );
                }
                if( json_get( interfaceJson, Resources::DataSender::rxBytes, valUint64 ) ) {
                    interfaceReport->mutable_rx_packets()->set_bytes( valUint64 );
                }
                if( json_get( interfaceJson, Resources::DataSender::rxCount, valUint64 ) ) {
                    interfaceReport->mutable_rx_packets()->set_count( valUint64 );
                }
                if( json_get( interfaceJson, Resources::DataSender::rxError, valUint64 ) ) {
                    interfaceReport->mutable_rx_packets()->set_errors( valUint64 );
                }

                if( json_get( interfaceJson, Resources::DataSender::txBytes, valUint64 ) ) {
                    interfaceReport->mutable_tx_packets()->set_bytes( valUint64 );
                }
                if( json_get( interfaceJson, Resources::DataSender::txCount, valUint64 ) ) {
                    interfaceReport->mutable_tx_packets()->set_count( valUint64 );
                }
                if( json_get( interfaceJson, Resources::DataSender::txError, valUint64 ) ) {
                    interfaceReport->mutable_tx_packets()->set_errors( valUint64 );
                }

                auto pingStatsIt = pingStatistics.find( deviceName );
                if( pingStatsIt != pingStatistics.cend() ) {
                    fillPingStatistics( *interfaceReport, pingStatsIt->second );
                }
                else {
                    fillPingStatistics( *interfaceReport, ServiceMetricsData::PingStatisticsData{} );
                }
            }
        }

        if( json_get( json,
                      { Resources::DataSender::StatusReportFields::service, Resources::DataSender::serviceNetwork },
                      networkObject ) ) {
            if( json_get( networkObject, Resources::DataSender::modems, modemInfo ) ) {
                for( const auto& it : modemInfo ) {
                    report.mutable_network()->add_modems();
                    int size = report.mutable_network()->modems_size();
                    if( json_get( it, Resources::DataSender::interfaceName, valStr ) ) {
                        report.mutable_network()->mutable_modems( size - 1 )->set_interface( valStr );
                    }
                    if( json_get( it, Resources::DataSender::frequencyBand, valStr ) ) {
                        report.mutable_network()->mutable_modems( size - 1 )->set_band_frequency( valStr );
                    }
                    if( json_get( it, Resources::DataSender::signalQuality, signalJson ) ) {
                        if( json_get( signalJson, Resources::DataSender::rssi, valInt64 ) )
                            report.mutable_network()
                                ->mutable_modems( size - 1 )
                                ->set_signal_quality( std::to_string( valInt64 ) );
                    }
                }
            }

            if( json_get( networkObject, Resources::DataSender::ssh_tunnels, sshTunnels ) ) {
                fillSshTunnelsInfo( sshTunnels, report );
            }

            if( json_get( networkObject, Resources::DataSender::internetAvailability, valBool ) ) {
                report.mutable_hub_info()->set_internet_availability( valBool );
            }
            if( json_get( networkObject, Resources::DataSender::uiNetworkConfiguration, valJson ) ) {
                report.mutable_hub_info()->set_ui_network_configuration( valJson.dump() );
            }
            if( json_get( networkObject, Resources::DataSender::activeInterface, valStr ) ) {
                report.mutable_hub_info()->set_active_interface( valStr );
            }
            if( json_get( networkObject, Resources::DataSender::wifiAvailability, valBool ) ) {
                report.mutable_hub_info()->set_wifi_availability( valBool );
            }
            if( json_get( networkObject, Resources::DataSender::nonMobileInternetAvailability, valBool ) ) {
                report.mutable_hub_info()->set_non_mobile_internet_availability( valBool );
            }
            if( json_get( networkObject, Resources::DataSender::activeWifiDevice, valStr ) ) {
                report.mutable_hub_info()->set_active_wifi_device( valStr );
            }
        }
        int processesSize = report.mutable_processes()->processes_size();
        LOGGER_TRACE( "Processes size: " + std::to_string( processesSize ) );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create network report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to create network report. Unknown exception" ) );
    }
}

void DataSender::jsonToReportTime( const Json& json, Report& report ) {
    uint64_t valUint64;
    uint32_t valUint32;
    std::string valStr;
    JsonObject tmpJson;
    std::string timezoneOffset;

    try {
        if( json_get( json, { Resources::DataSender::system, Resources::DataSender::TimeFields::time }, tmpJson ) ) {
            if( json_get( tmpJson, Resources::DataSender::TimeFields::timezone, valUint32 ) )
                report.mutable_time()->set_timezone( valUint32 );
            valStr = json_get_str( tmpJson, Resources::DataSender::TimeFields::timezoneIdentifier );
            if( valStr.empty() )
                valStr = "na";
            report.mutable_time()->set_timezone_identifier( valStr );
            if( json_get( tmpJson, Resources::DataSender::TimeFields::utctime, valUint64 ) )
                report.mutable_time()->set_utc_time( valUint64 );
            if( json_get( tmpJson, Resources::DataSender::TimeFields::ntpLastSync, valUint64 ) )
                report.mutable_time()->set_ntp_last_sync( valUint64 );
            timezoneOffset = json_get_str( tmpJson, Resources::DataSender::TimeFields::timezoneOffset );
            if( timezoneOffset.empty() )
                timezoneOffset = "na";
            report.mutable_time()->set_timezone_offset( timezoneOffset );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create time report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to create time report. Unknown exception" ) );
    }
}

void DataSender::jsonToReportUIService( const Json& data, Report& report ) {
    try {
        fillHttpServerReport( data, report );
        fillNmaReport( data, report );
    }

    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create ui service report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to create ui service report. Unknown exception" ) );
    }
}

void DataSender::fillHttpServerReport( const JsonObject& data, Report& report ) {
    uint32_t valUint32;
    uint64_t valUint64;
    JsonObject tmpJson;

    if( json_get( data, Resources::DataSender::HttpServer, tmpJson ) ) {
        using namespace Resources::DataSender::HttpServerFields;
        UiService_HttpServer* pHttpServer = report.mutable_ui_service()->mutable_http_server();
        if( json_get( tmpJson, Received, valUint32 ) )
            pHttpServer->set_received( valUint32 );
        if( json_get( tmpJson, Success, valUint32 ) )
            pHttpServer->set_success( valUint32 );
        if( json_get( tmpJson, Errors, valUint32 ) )
            pHttpServer->set_errors( valUint32 );
        if( json_get( tmpJson, WrongAuth, valUint32 ) )
            pHttpServer->set_wrong_auth( valUint32 );
        if( json_get( tmpJson, TimedOut, valUint64 ) )
            pHttpServer->set_timed_out( valUint64 );
        if( json_get( tmpJson, LimitReached, valUint32 ) )
            pHttpServer->set_limit_reached( valUint32 );
        if( json_get( tmpJson, RxBytes, valUint64 ) )
            pHttpServer->set_rx_bytes( valUint64 );
        if( json_get( tmpJson, TxBytes, valUint64 ) )
            pHttpServer->set_tx_bytes( valUint64 );
        if( json_get( tmpJson, Unhandled, valUint32 ) )
            pHttpServer->set_unhandled( valUint32 );
        if( json_get( tmpJson, ParallelRequests, valUint32 ) )
            pHttpServer->set_parallel_requests( valUint32 );
    }
}

void DataSender::fillNmaReport( const JsonObject& data, Report& report ) {
    uint32_t valUint32;
    uint64_t valUint64;
    JsonObject tmpJson;

    if( json_get( data, Resources::DataSender::Nma, tmpJson ) ) {
        LOGGER_DEBUG( "NMA report: " + cc::j( tmpJson ) );

        if( json_get( tmpJson, Resources::DataSender::NmaConnectionUptime, valUint64 ) ) {
            report.mutable_ui_service()->mutable_nma()->set_nma_connection_uptime( valUint64 );
        }
        else {
            Log::error( "Couldn't fetch NMA uptime" );
        }
        if( json_get( tmpJson, Resources::DataSender::NmaReconnects, valUint32 ) ) {
            report.mutable_ui_service()->mutable_nma()->set_nma_reconnects( valUint32 );
        }
        if( json_get( tmpJson, Resources::DataSender::NmaConnectionCumulativeUptime, valUint64 ) ) {
            report.mutable_ui_service()->mutable_nma()->set_nma_connection_cumulative_uptime( valUint64 );
        }
        else {
            Log::error( "Couldn't fetch NMA cumulative uptime" );
        }
        if( json_get( tmpJson, Resources::DataSender::NmaReconnectsCumulative, valUint32 ) ) {
            report.mutable_ui_service()->mutable_nma()->set_nma_reconnects_cumulative( valUint32 );
        }
    }
}

void DataSender::jsonToReportProcesses( const Json& json, Report& report ) {
    Json servicesObject;
    Json memJson;
    std::string valStr;
    uint valInt = 0;
    uint64_t uptime = 0;
    uint64_t reportDate = 0;
    uint64_t openFileDescriptors;

    try {
        if( json_get( json, Resources::DataSender::StatusReportFields::service, servicesObject ) ) {
            for( auto it = servicesObject.begin(); it != servicesObject.end(); ++it ) {
                auto* process = report.mutable_processes()->add_processes();

                // string name = 1;
                process->set_name( it.key() );

                Json json_process = it.value();

                // Status status = 2;
                if( json_get( json_process, Resources::DataSender::serviceStatus, valStr ) ) {
                    if( valStr == Resources::DataSender::healthy ) {
                        process->set_status( Processes_Process_Status_STATUS_RUNNING );
                    }
                    else if( valStr == Resources::DataSender::stopped ) {
                        process->set_status( Processes_Process_Status_STATUS_STOPPED );
                    }
                    else if( valStr == Resources::DataSender::corrupt ) {
                        process->set_status( Processes_Process_Status_STATUS_CRASHED );
                    }
                    else {
                        process->set_status( Processes_Process_Status_STATUS_UNKNOWN );
                    }
                }
                if( json_get( json_process, Resources::DataSender::ServiceFields::cpuTime, valInt ) ) {
                    process->set_cpu_time( valInt );
                }
                if( json_get( json_process, Resources::DataSender::ServiceFields::respawnCount, valInt ) ) {
                    process->set_restarts( valInt );
                }
                if( json_get( json_process, Resources::DataSender::ServiceFields::uptime, uptime ) ) {
                    process->set_uptime( uptime );
                }
                if( json_get( json_process, Resources::DataSender::ServiceFields::reportDate, reportDate ) ) {
                    process->set_report_date( reportDate );
                }
                if( json_get( json_process,
                              Resources::DataSender::ServiceFields::openFileDescriptors,
                              openFileDescriptors ) ) {
                    process->set_open_file_descriptors( openFileDescriptors );
                }
                if( json_get( json_process, Resources::DataSender::ServiceFields::memUsage, memJson ) ) {
                    auto* processMemory = process->mutable_memory();
                    if( json_get( memJson, "VmSize", valInt ) ) {
                        processMemory->set_vm_size( valInt );
                    }
                    if( json_get( memJson, "VmRSS", valInt ) ) {
                        processMemory->set_vm_rss( valInt );
                    }
                    if( json_get( memJson, "RssAnon", valInt ) ) {
                        processMemory->set_rss_anon( valInt );
                    }
                    if( json_get( memJson, "VmPeak", valInt ) ) {
                        processMemory->set_vm_peak( valInt );
                    }
                    if( json_get( memJson, "RssFile", valInt ) ) {
                        processMemory->set_rss_file( valInt );
                    }
                    if( json_get( memJson, "RssShmem", valInt ) ) {
                        processMemory->set_rss_shmem( valInt );
                    }
                    if( json_get( memJson, "VmData", valInt ) ) {
                        processMemory->set_vm_data( valInt );
                    }
                    if( json_get( memJson, "VmStk", valInt ) ) {
                        processMemory->set_vm_stk( valInt );
                    }
                    if( json_get( memJson, "VmExe", valInt ) ) {
                        processMemory->set_vm_exe( valInt );
                    }
                    if( json_get( memJson, "VmHWM", valInt ) ) {
                        processMemory->set_vm_hwm( valInt );
                    }
                    if( json_get( memJson, "VmLck", valInt ) ) {
                        processMemory->set_vm_lck( valInt );
                    }
                    if( json_get( memJson, "VmLib", valInt ) ) {
                        processMemory->set_vm_lib( valInt );
                    }
                    if( json_get( memJson, "VmPTE", valInt ) ) {
                        processMemory->set_vm_pte( valInt );
                    }
                    if( json_get( memJson, "VmPin", valInt ) ) {
                        processMemory->set_vm_pin( valInt );
                    }
                    if( json_get( memJson, "VmSwap", valInt ) ) {
                        processMemory->set_vm_swap( valInt );
                    }
                }
            }
        }
        int processesSize = report.mutable_processes()->processes_size();
        LOGGER_TRACE( "Processes size: " + std::to_string( processesSize ) );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create processes report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to create processes report. Unknown exception" ) );
    }
}

void DataSender::buildInternalMetricsReport( Report& report ) const {
    try {
        auto metrics = report.mutable_metrics();

        if( m_internalMetrics ) {
            auto transferStat = metrics->mutable_transfer_stat();
            transferStat->set_bus_requests( m_internalMetrics->bus().requests() );
            transferStat->set_bus_errors( m_internalMetrics->bus().errors() );
            transferStat->set_packets( m_internalMetrics->packetsSent() );
            transferStat->set_sent_size( m_internalMetrics->sentSize() );

            const Json serializedMetrics = m_internalMetrics->toJson();
            transferStat->set_sent_at( json_get_int64( serializedMetrics, "sentAt", 0 ) );
            transferStat->set_errors( m_internalMetrics->sentErrors() );
            transferStat->set_error_at( json_get_int64( serializedMetrics, "errorAt", 0 ) );

            metrics->set_sequence( m_internalMetrics->sequenceNumber() );
            metrics->set_report_type( Metrics_ReportType_TYPE_FULL );
        }
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Exception while building metrics report: " + ex.what() );
    }
}

void DataSender::jsonToReportZwaveService( const Json& data, Report& report ) {
    try {
        fillZwaveCompletedCommandsReport( data, report );
        fillZwaveFailedCommandsReport( data, report );
    }

    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create zwave service report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to create zwave service report. Unknown exception" ) );
    }
}

void DataSender::fillZwaveCompletedCommandsReport( const JsonObject& data, Report& report ) {
    using namespace Resources::DataSender;

    uint32_t valUint32;

    std::vector< Json > keyPath{ StatusReportFields::service,
                                 Zwave,
                                 ZwaveCommandsStatisticsFields::ZwaveCommandsStatistics,
                                 ZwaveCommandsStatisticsFields::CompletedCommands };

    json_get( data, keyPath, [&valUint32, &report]( const Json& commandInfos ) {
        using namespace Resources::DataSender::ZwaveCommandsStatisticsFields;

        if( commandInfos.is_array() ) {
            for( const auto& commandInfo : commandInfos ) {
                auto* pCommandReport = report.mutable_zwave()->add_node_command_delays();
                if( json_get( commandInfo, NodeId, valUint32 ) )
                    pCommandReport->set_node_id( valUint32 );
                if( json_get( commandInfo, When, valUint32 ) )
                    pCommandReport->set_send_time( valUint32 );
                if( json_get( commandInfo, Delay, valUint32 ) )
                    pCommandReport->set_delay( valUint32 );
            }
        }
    } );
}

void DataSender::fillZwaveFailedCommandsReport( const JsonObject& data, Report& report ) {
    using namespace Resources::DataSender;

    uint32_t valUint32;

    std::vector< Json > keyPath{ StatusReportFields::service,
                                 Zwave,
                                 ZwaveCommandsStatisticsFields::ZwaveCommandsStatistics,
                                 ZwaveCommandsStatisticsFields::FailedCommands };

    json_get( data, keyPath, [&valUint32, &report]( const Json& commandInfos ) {
        using namespace Resources::DataSender::ZwaveCommandsStatisticsFields;

        if( commandInfos.is_array() ) {
            for( const auto& commandInfo : commandInfos ) {
                auto* pCommandReport = report.mutable_zwave()->add_node_command_fails();
                if( json_get( commandInfo, NodeId, valUint32 ) )
                    pCommandReport->set_node_id( valUint32 );
                if( json_get( commandInfo, When, valUint32 ) )
                    pCommandReport->set_send_time( valUint32 );
            }
        }
    } );
}

void DataSender::jsonToReportMsg( Report& report, const Json& watcherReport, bool reduceReport ) const {
    try {
        const Json& json = watcherReport;
        std::string valStr;

        // Report -> version
        report.set_version( m_protobufVersion );

        Json errorJSON;
        if( json_get( json, "error", errorJSON ) ) {
            if( json_get( errorJSON, "message", valStr ) ) {
                if( valStr == "Failed to get data from supervisor" )
                    return;
            }
        }

        jsonToReportSystem( json, report );
        jsonToReportTime( json, report );
        fillReportHubInfo( json, reduceReport, report );
        if( !reduceReport ) {
            jsonToReportProcesses( json, report );
            jsonToReportNetwork( json, report );
            jsonToReportUIService( json, report );
            jsonToReportZwaveService( json, report );
        }
        buildInternalMetricsReport( report );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to build report. Exception: " ) + cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to build report. Unknown exception" ) );
    }
}

std::string DataSender::ucharToStdString( unsigned char* data, int dataLen ) {
    return { reinterpret_cast< char* >( data ), size_t( dataLen ) };
}

void DataSender::fillReportHubInfo( const Json& json, bool reducedReport, Report& report ) {
    bool tmpBool = false;
    std::string tmpString;
    JsonObject tmpJson;
    JsonArray tmpJsonArray;
    uint valInt;

    try {
        if( json_get( json,
                      { Resources::DataSender::StatusReportFields::service, Resources::DataSender::uiService },
                      tmpJson ) ) {
            if( !reducedReport ) {
                if( json_get( tmpJson, Resources::DataSender::zWaveCalibration, tmpString ) ) {
                    report.mutable_hub_info()->set_zwave_calibration( tmpString );
                }
                if( json_get( tmpJson, Resources::DataSender::cloudCoordinatorURL, tmpString ) ) {
                    report.mutable_hub_info()->set_cloud_coordinator_url( tmpString );
                }
                if( json_get( tmpJson, Resources::DataSender::explicitCloudCoordinatorURL, tmpString ) ) {
                    report.mutable_hub_info()->set_explicit_cloud_coordinator_url( tmpString );
                }
                if( json_get( tmpJson, Resources::DataSender::nmaServerUrl, tmpString ) ) {
                    report.mutable_hub_info()->set_nma_server_url( tmpString );
                }
                if( json_get( tmpJson, Resources::DataSender::hubErrors, tmpJsonArray ) ) {
                    fillHubErrors( tmpJsonArray, report );
                }
            }
            if( json_get( tmpJson, Resources::DataSender::nmaConnection, tmpBool ) ) {
                report.mutable_hub_info()->set_nma_status( tmpBool ? HubInfo_NMAStatus_NMA_STATUS_CONNECTED :
                                                                     HubInfo_NMAStatus_NMA_STATUS_DISCONNECTED );
                if( json_get( tmpJson, Resources::DataSender::busConnectionStatus, tmpBool ) ) {
                    report.mutable_hub_info()->set_bus_connection_status( tmpBool );
                }
                if( json_get( tmpJson, Resources::DataSender::hubRegistrationStatus, tmpBool ) ) {
                    report.mutable_hub_info()->set_hub_registration_status( tmpBool );
                }
                if( json_get( tmpJson, Resources::DataSender::hubConnectingStatus, tmpBool ) ) {
                    report.mutable_hub_info()->set_hub_connecting_status( tmpBool );
                }
            }
            report.mutable_hub_info()->set_serial( std::stoull( HubInfoHelper::getSerial() ) );
            report.mutable_hub_info()->set_model( HubInfoHelper::getHubType() );
            report.mutable_hub_info()->set_controller_uuid( HubInfoHelper::getControllerUUID() );
            report.mutable_hub_info()->set_hardware_revision( HubInfoHelper::getHardwareRevision() );
            report.mutable_hub_info()->set_firmware_version( FirmwareInfoHelper::getFirmwareVersionString() );
        }
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to build hub info report. Exception: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to build hub info report. Unknown exception" ) );
    }
}

void DataSender::fillHubErrors( const JsonArray& hubErrors, Report& report ) {
    auto errors = ServiceMetricsData::HubErrorsTable::fromJsonArray( hubErrors );
    for( auto& error : errors ) {
        report.mutable_hub_info()->add_hub_errors();
        int size = report.mutable_hub_info()->hub_errors_size();
        auto* errorProto = report.mutable_hub_info()->mutable_hub_errors( size - 1 );
        errorProto->set_error_id( error.errorId );
        errorProto->set_count( error.count );
        errorProto->set_last_updated( error.lastUpdated );
    }
}

void DataSender::fillSshTunnelsInfo( const JsonArray& sshTunnels, Report& report ) {
    for( const auto& tunnelDataJson : sshTunnels ) {
        auto tunnelData = ServiceMetricsData::SshTunnelData::fromJson( tunnelDataJson );
        report.mutable_network()->add_ssh_tunnels();
        int size = report.mutable_network()->ssh_tunnels_size();
        auto* tunnelProto = report.mutable_network()->mutable_ssh_tunnels( size - 1 );
        tunnelProto->set_pid( tunnelData.pid );
        tunnelProto->set_id( tunnelData.id );

        if( tunnelData.sshTunnelType == ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_UNKNOWN ) {
            tunnelProto->set_ssh_tunnel_type( SshTunnel_SshTunnelType_TYPE_UNKNOWN );
        }
        else if( tunnelData.sshTunnelType == ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_HTTP_SERVER ) {
            tunnelProto->set_ssh_tunnel_type( SshTunnel_SshTunnelType_TYPE_HTTP_SERVER );
        }
        else if( tunnelData.sshTunnelType == ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_CAMERA ) {
            tunnelProto->set_ssh_tunnel_type( SshTunnel_SshTunnelType_TYPE_CAMERA );
        }
        else if( tunnelData.sshTunnelType == ServiceMetricsData::SshTunnelData::SshTunnelType::TYPE_SUPPORT ) {
            tunnelProto->set_ssh_tunnel_type( SshTunnel_SshTunnelType_TYPE_SUPPORT );
        }

        if( tunnelData.sshTunnelStatus == ServiceMetricsData::SshTunnelData::SshTunnelStatus::STATUS_UNKNOWN ) {
            tunnelProto->set_ssh_tunnel_status( SshTunnel_SshTunnelStatus_STATUS_UNKNOWN );
        }
        else if( tunnelData.sshTunnelStatus == ServiceMetricsData::SshTunnelData::SshTunnelStatus::STATUS_OPEN ) {
            tunnelProto->set_ssh_tunnel_status( SshTunnel_SshTunnelStatus_STATUS_OPEN );
        }
        else if( tunnelData.sshTunnelStatus == ServiceMetricsData::SshTunnelData::SshTunnelStatus::STATUS_BROKEN ) {
            tunnelProto->set_ssh_tunnel_status( SshTunnel_SshTunnelStatus_STATUS_BROKEN );
        }
        else if( tunnelData.sshTunnelStatus == ServiceMetricsData::SshTunnelData::SshTunnelStatus::STATUS_CLOSED ) {
            tunnelProto->set_ssh_tunnel_status( SshTunnel_SshTunnelStatus_STATUS_CLOSED );
        }

        tunnelProto->set_tcp_state( tunnelData.tcpState );
        tunnelProto->set_server_url( tunnelData.serverUrl );
        tunnelProto->set_server_port( tunnelData.serverPort );
        tunnelProto->set_remote_port( tunnelData.remotePort );
        tunnelProto->set_local_port( tunnelData.localPort );
        tunnelProto->set_start_time( tunnelData.startTime );
        tunnelProto->set_uptime( tunnelData.uptime );

        for( auto& element : tunnelData.connectionStatistics ) {
            SshTunnel::SshTunnelConnectionStatus status;
            if( element.first == ServiceMetricsData::SshTunnelData::SshTunnelConnectionStatus::STATUS_CONNECTED ) {
                status = SshTunnel_SshTunnelConnectionStatus_CONNECTION_STATUS_CONNECTED;
            }
            else if( element.first ==
                     ServiceMetricsData::SshTunnelData::SshTunnelConnectionStatus::STATUS_CONNECTION_REFUSED ) {
                status = SshTunnel_SshTunnelConnectionStatus_CONNECTION_STATUS_REFUSED;
            }
            else {
                continue;
            }

            tunnelProto->add_connection_statistics();
            auto size = tunnelProto->connection_statistics_size();
            auto* statistics = tunnelProto->mutable_connection_statistics( size - 1 );
            statistics->set_status( status );
            statistics->set_count( element.second );
        }
    }
}

bool DataSender::sendData( const std::string& data ) {
    Json report;
    if( parseJson( data, report ) ) {
        return sendData( report );
    }
    else {
        Log::error( "Failed to parse watcher report" );
    }
    return false;
}

bool DataSender::sendData( const Json& watcherReport ) {
    try {
        // create Report protobuf
        Report report;

        Log::info( "Prepare report" );
        const bool reducedReport = m_internalMetrics && m_internalMetrics->packetsSent() % 20 == 0;
        /** Creates protobuf report from json data */
        // json to protobuf
        jsonToReportMsg( report, watcherReport, reducedReport );

        /** Checks if report is good and if it has anomalies */
        if( !checkIfDataSendable( report ) ) {
            LOGGER_INFO_RAW( "Data received but anomaly was not detected, data won't be sent to the cloud." );
            return false;
        }

        std::string serializedMsg;
        if( reducedReport ) {
            if( !report.SerializeToString( &serializedMsg ) ) {
                Log::error( cc::error( "Protobuf serialize error" ) );
                return false;
            }
            LOGGER_DEBUG( cc::debug( "Original report size: " ) + cc::num10( serializedMsg.size() ) );

            serializedMsg.clear();
            if( report.has_metrics() ) {
                LOGGER_WARN( "Set Metrics REDUCED report type, sequence number: " +
                             cc::num10( m_internalMetrics->sequenceNumber() ) );
                auto metricsMessage = report.mutable_metrics();
                metricsMessage->set_report_type( Metrics_ReportType_TYPE_REDUCED );
            }
            // Reduce report
            LOGGER_INFO( "Sending reduced report: Sequence Number: " +
                         cc::num10( m_internalMetrics->sequenceNumber() ) );
            if( report.has_processes() ) {
                report.clear_processes();
            }
            if( report.has_network() ) {
                report.clear_network();
            }
            if( report.has_ui_service() ) {
                report.clear_ui_service();
            }
            if( !report.SerializeToString( &serializedMsg ) ) {
                Log::error( cc::error( "Protobuf serialize error" ) );
                return false;
            }
            LOGGER_INFO( cc::info( "Reduced report size: " ) + cc::num10( serializedMsg.size() ) );
        }
        else {
            // serialize full message
            if( !report.SerializeToString( &serializedMsg ) ) {
                Log::error( cc::error( "Protobuf serialize error" ) );
                return false;
            }
        }

        // Encrypt and sign message
        MetricsWire metricsWireStruct;
        bool isEncOk = encryptAndSignMessage( serializedMsg, metricsWireStruct );

        if( !isEncOk ) {
            Log::error( cc::error( "Data encrypt failed" ) );
            return false;
        }

        Log::info( "Encrypt the message" );
        // create Wire protobuf
        Wire encryptedMessage;
        encryptedMessage.set_iv( metricsWireStruct.iv );
        encryptedMessage.set_signature( metricsWireStruct.signature );
        encryptedMessage.set_encryptedkey( metricsWireStruct.encryptedKey );
        encryptedMessage.set_encryptedbytes( metricsWireStruct.encryptedBytes );

        // add client crt file to wire
        if( !m_clientCrtBytes.empty() )
            encryptedMessage.set_certificate( m_clientCrtBytes );

        // serialize encrypted message
        std::string serializedEncryptedMsg;
        if( !encryptedMessage.SerializeToString( &serializedEncryptedMsg ) ) {
            Log::error( cc::error( "Protobuf serialize encrypted message error" ) );
            return false;
        }

        Log::info( "Send report message via UDP" );
        const auto& dataSize = serializedEncryptedMsg.size();
        bool status = udpSendData( serializedEncryptedMsg.data(), dataSize );
        if( status ) {
            m_lastSendTime = report.system().uptime();

            LOGGER_INFO( "Report was sent successfully, sent data size: " + std::to_string( dataSize ) +
                         ", result: " + cc::flag( status ) );
            if( m_internalMetrics ) {
                m_internalMetrics->registerSend( dataSize );
            }
        }
        else {
            if( m_internalMetrics ) {
                m_internalMetrics->registerError();
            }

            LOGGER_ERROR( "Sending report failed, size: " + std::to_string( dataSize ) );
        }

        return status;
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to send report. Exception: " ) + cc::note( CommonUtils::whatError( e ) ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Unable to send report. Unknown exception" ) );
    }
    return false;
}

size_t DataSender::getServerPort() {
    m_serverPortIndex = m_serverPortIndex < m_serverPorts.size() ? m_serverPortIndex : 0;

    return m_serverPorts[ m_serverPortIndex++ ];
}

bool DataSender::udpSendData( const char* data, size_t dataLen ) {
    auto serverAddr = UdpSocket::getServerAddr( m_serverUrl, getServerPort() );

    bool result = sendUdpReport( data, dataLen, serverAddr );
    if( !result ) {
        LOGGER_ERROR( cc::error( "Failed to send UDP report, size: " + cc::num10( dataLen ) ) );
        return false;
    }

    LOGGER_INFO_RAW( cc::info( "Binary report has been sent successfully, length: " ) + cc::num10( dataLen ) );
    return true;
}

int DataSender::encrypt( const unsigned char* plainText,
                         int plainTextLen,
                         const unsigned char* key,
                         const unsigned char* iv,
                         unsigned char* cipherText ) {
    EvpCipherCtxPtr ctx( EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free );

    int len;

    int cipherTextLen;
    if( 1 != EVP_EncryptInit_ex( ctx.get(), EVP_aes_256_cbc(), nullptr, key, iv ) )
        Log::error( cc::error( "EVP_EncryptInit_ex error" ) );

    if( 1 != EVP_EncryptUpdate( ctx.get(), cipherText, &len, plainText, plainTextLen ) )
        Log::error( cc::error( "EVP_EncryptUpdate error" ) );
    cipherTextLen = len;

    if( 1 != EVP_EncryptFinal_ex( ctx.get(), cipherText + len, &len ) )
        Log::error( cc::error( "EVP_EncryptFinal_ex error" ) );
    cipherTextLen += len;

    return cipherTextLen;
}

std::string DataSender::generateRandString( int size ) {
    std::string str( size, 0 );
    RAND_bytes( (unsigned char*) str.data(), size );
    return str;
}

const unsigned char* DataSender::toBytePointer( const std::string& str ) {
    return reinterpret_cast< const unsigned char* >( str.data() );
}

namespace {
bool hasAnomalyLoadAverage( const Report& protobufData, int64_t loadAverageThreshold ) {
    if( !protobufData.has_system() ) {
        return false;
    }
    if( !protobufData.system().has_load_average() ) {
        return false;
    }

    bool loadAvg1 = protobufData.system().load_average().load_1();
    bool loadAvg5 = protobufData.system().load_average().load_5();
    bool loadAverageCheck = loadAvg1 > loadAverageThreshold || loadAvg5 > loadAverageThreshold;

    return loadAverageCheck;
}
bool hasAnomalyMemory( const Report& protobufData, int64_t availMemoryThreshold ) {
    int64_t memAvailable = protobufData.system().memory().available();

    return ( memAvailable < availMemoryThreshold );
}
}  // namespace

// Return which anomaly is found
bool DataSender::checkAnomaly( const Report& protobufData ) {
    uint64_t uptime;

    if( !protobufData.has_system() ) {
        return false;
    }
    if( !protobufData.system().has_load_average() ) {
        return false;
    }
    if( !protobufData.system().has_memory() ) {
        return false;
    }

    uptime = protobufData.system().uptime();

    bool uptimeCheck = uptime <= m_uptime5MinThreshold;
    bool loadAverageCheck = hasAnomalyLoadAverage( protobufData, m_loadAverageThreshold );
    bool memoryCheck = hasAnomalyMemory( protobufData, m_availMemoryThreshold );
    std::string anomaly;
    if( uptimeCheck )
        anomaly += "uptime ";
    if( loadAverageCheck )
        anomaly += "loadAverage ";
    if( memoryCheck )
        anomaly += "memory ";

    if( uptimeCheck || loadAverageCheck || memoryCheck ) {
        LOGGER_WARN( cc::warn( "Anomaly found: " ) + anomaly );
        return true;
    }

    return false;
}

bool DataSender::checkIfDataSendable( const Report& protobufData ) const {
    if( checkAnomaly( protobufData ) ) {
        return true;
    }

    if( !protobufData.has_system() ) {
        return false;
    }

    uint64_t uptime = protobufData.system().uptime();
    if( uptime - m_lastSendTime >= m_sendInterval ) {
        return true;
    }

    return false;
}

}  // namespace Metrics
}  // namespace HubPlatform
