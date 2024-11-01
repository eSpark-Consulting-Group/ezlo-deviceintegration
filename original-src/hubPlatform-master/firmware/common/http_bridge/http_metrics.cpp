#include "http_metrics.h"

namespace HubPlatform {
namespace HttpBridge {

Json HubPlatform::HttpBridge::HttpServerMetrics::toJson() {
    LockType lock( m_mutex );

    Json report = {
        { "received", m_received },
        { "success", m_handled },
        // Cumulative errors number
        { "errors", m_errors + m_wrongAuth + m_timedOut + m_limitReached },
        { "wrongAuth", m_wrongAuth },
        { "timedOut", m_timedOut },
        { "limitReached", m_limitReached },
        { "rxBytes", m_rxBytes },
        { "txBytes", m_txBytes },
        { "unhandled", m_closedRequests },
        { "parallelRequests", m_parallelRequests },
    };
    return report;
}

void HttpServerMetrics::incrementReceivedRequest() {
    LockType lock( m_mutex );
    m_received++;
}

void HttpServerMetrics::incrementHandledRequest() {
    LockType lock( m_mutex );
    m_handled++;
}

void HttpServerMetrics::incrementErrorRequest() {
    LockType lock( m_mutex );
    m_errors++;
}

void HttpServerMetrics::incrementWrongAuthRequest() {
    LockType lock( m_mutex );
    m_wrongAuth++;
}

void HttpServerMetrics::appendRxBytes( size_t size ) {
    LockType lock( m_mutex );
    m_rxBytes += size;
}

void HttpServerMetrics::appendTxBytes( size_t size ) {
    LockType lock( m_mutex );
    m_txBytes += size;
}

void HttpServerMetrics::incrementRequestTimedOut() {
    LockType lock( m_mutex );
    m_timedOut++;
}

void HttpServerMetrics::incrementRequestsLimitReached() {
    LockType lock( m_mutex );
    m_limitReached++;
}

void HttpServerMetrics::incrementUnhandledRequest() {
    LockType lock( m_mutex );
    m_closedRequests++;
}

void HttpServerMetrics::fromJson( const Json& data ) {}

void HttpServerMetrics::parallelRequests( size_t parallelCount ) {
    LockType lock( m_mutex );
    m_parallelRequests = parallelCount;
}

}  // namespace HttpBridge
}  // namespace HubPlatform