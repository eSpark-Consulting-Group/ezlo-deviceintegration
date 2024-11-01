#include "defaults.h"
#include "data_receiver.h"
#include "data_sender.h"
#include "common_utils/cpp14.h"
#include "jsonrpc/error_codes.h"
#include "resources/metrics_resources.h"
#include "ubusmsg.h"
#include <cstdint>
#include <utility>

namespace HubPlatform {
namespace Metrics {

DataReceiver::DataReceiver( InternalMetricsPtr internalMetricsPtr )
: m_internalMetricsPtr( std::move( internalMetricsPtr ) ) {
    m_sender = std::make_unique< DataSender >( m_internalMetricsPtr );
}

DataReceiver::~DataReceiver() {
    stopGetDataLoop();
}

void DataReceiver::startGetDataLoop( const int& requestIntervalInSec ) {
    m_isRunning = true;
    std::unique_ptr< Ubus::UBusInterface > uBusObject = std::make_unique< Ubus::UBusImpl >();

#if !defined( __HUB_TYPE_G150__ ) && !defined( __HUB_TYPE_VHUB__ ) && !defined( __HUB_TYPE_LOCAL__ )
    uBusObject->connect( nullptr );
#endif

    if( uBusObject == nullptr || !uBusObject->isConnected() ) {
        return;
    }

    int requestsCount = 0;
    while( m_isRunning ) {
        requestsCount++;

        std::string result;

        Log::info( "Check connection to watcher" );
        if( uBusObject->checkConnection( "watcher" ) != UBUS_STATUS_OK ) {
            emitConnectionCheckFailed();
            LOGGER_DEBUG_RAW( cc::debug( "Watcher could not be found in ubus, re-connecting.." ) );
            if( !uBusObject->connect( nullptr ) ) {
                LOGGER_ERROR( "Couldn't reconnect to UBus" );
                emitUbusReconnectFailed();
            }
            else {
                LOGGER_INFO( "Reconnected to ubus successfully" );
                emitUbusReconnectSuccess();
            }
        }

        if( m_internalMetricsPtr ) {
            m_internalMetricsPtr->bus().registerAttempt();
        }

        Log::info( cc::str( "Send ubus request to supervisor: " ) + Resources::DataReceiver::uBusRequestMethod );
        bool isOk = uBusObject->sendCommand(
            Resources::DataReceiver::uBusTarget, Resources::DataReceiver::uBusRequestMethod, "", "", result );

        auto resultSize = result.size();
        // Parse as Json and minify
        Json watcherReport;
        if( parseJson( result, watcherReport ) ) {
            result.clear();
        }

        if( !isOk ) {
            Log::error( cc::error( "Failed to get data from supervisor" ) );

            m_sender->sendData(
                JSON( "error" << JSON( "code" << static_cast< int >( Errors::Codes::APPLICATION_ERROR ) << "message"
                                              << "Failed to get data from supervisor"
                                              << "data"
                                              << "GET" ) ) );
            if( m_internalMetricsPtr ) {
                m_internalMetricsPtr->bus().registerError();
            }
        }
        else {
            Log::info( cc::info( "Getting data from the supervisor is successful. Data size: " ) +
                       std::to_string( resultSize ) + ", iterations: " + cc::num10( requestsCount ) );

            if( m_internalMetricsPtr ) {
                m_internalMetricsPtr->bus().registerRequest();
            }
            Log::info( "Sending data..." );
            bool sendResult = m_sender->sendData( watcherReport );
            Log::info( "Sending data: complete, result: " + cc::flag( sendResult ) );

            if( m_internalMetricsPtr ) {
                m_internalMetricsPtr->saveLatestReport( std::move( watcherReport ) );
                watcherReport.clear();  // don't use result after this
            }
        }

        if( ( requestsCount < 100 && requestsCount % 4 == 0 ) ||
            ( requestsCount >= 100 && requestsCount % 120 == 0 ) ) {
            printMetricsStat();
        }
        std::unique_lock< std::mutex > lock( m_intervalMutex );
        m_intervalCondition.wait_for(
            lock, std::chrono::seconds( requestIntervalInSec ), [this] { return !m_isRunning; } );
    }
}

void DataReceiver::stopGetDataLoop() {
    std::unique_lock< std::mutex > lock( m_intervalMutex );
    m_isRunning = false;
    lock.unlock();
    m_intervalCondition.notify_one();
}

DataSender* DataReceiver::getSender() const {
    return m_sender.get();
}

const InternalMetrics* DataReceiver::getInternalMetrics() const {
    return m_internalMetricsPtr.get();
}

void DataReceiver::printMetricsStat() {
    if( !m_internalMetricsPtr )
        return;
    auto& metrics = *m_internalMetricsPtr;
    Log::info( "Internal metrics: " + cc::j( metrics.toJson() ) );
    Log::info( "Latest watcher report: " + cc::j( metrics.getLatestReport() ) );
}

bool DataReceiver::isRunning() const {
    return m_isRunning;
}

void DataReceiver::shutdown() {
    LOGGER_TRACE( "Shutdown ubus object" );
}

void DataReceiver::emitConnectionCheckFailed() {
    if( m_internalMetricsPtr ) {
        m_internalMetricsPtr->bus().connectionCheckFailed();
    }
}

void DataReceiver::emitUbusReconnectFailed() {
    if( m_internalMetricsPtr ) {
        m_internalMetricsPtr->bus().reconnectFailed();
    }
}

void DataReceiver::emitUbusReconnectSuccess() {
    if( m_internalMetricsPtr ) {
        m_internalMetricsPtr->bus().reconnectSuccess();
    }
}

}  // namespace Metrics
}  // namespace HubPlatform
