#include "internal_metrics.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Metrics {

InternalMetrics::InternalMetrics()
: m_sentAt( TimePoint::min() )
, m_errorAt( TimePoint::min() ) {}

Json InternalMetrics::toJson() const {
    LockType lock( m_mutex );

    Json result = JSON_EMPTY_OBJ;

    json_add_uint( result, "packets", m_packets );
    json_add_int64( result, "sentSize", m_sentSize );
    time_t timestamp = std::chrono::system_clock::to_time_t( m_sentAt );
    json_add_long( result, "sentAt", timestamp < 0 ? 0 : timestamp );

    json_add_uint( result, "errors", m_packetSentErrors );
    timestamp = std::chrono::system_clock::to_time_t( m_errorAt );
    json_add_long( result, "errorAt", timestamp < 0 ? 0 : timestamp );

    result[ "bus" ] = m_busMetrics.toJson();

    return result;
}

size_t InternalMetrics::packetsSent() const {
    LockType lock( m_mutex );
    return m_packets;
}

size_t InternalMetrics::sentSize() {
    LockType lock( m_mutex );
    return m_sentSize;
}

void InternalMetrics::registerSend( size_t packetSize ) {
    LockType lock( m_mutex );
    m_packets++;
    m_sentSize += packetSize;
    m_sentAt = std::chrono::system_clock::now();
}

void InternalMetrics::registerError() {
    LockType lock( m_mutex );
    m_packetSentErrors++;
    m_errorAt = std::chrono::system_clock::now();
}

/** Sequence number: a packet number used to identify position of a packet in sequence of sent packets.
 * The sequence number is incremented every time packet is sent or an error during send occurred. */
size_t InternalMetrics::sequenceNumber() const {
    LockType lock( m_mutex );
    return m_packets + m_packetSentErrors;
}

size_t InternalMetrics::sentErrors() const {
    LockType lock( m_mutex );
    return m_packetSentErrors;
}

void InternalMetrics::saveLatestReport( Json report ) {
    LockType lock( m_mutex );
    m_latestReport = std::move( report );
}

const Json& InternalMetrics::getLatestReport() const {
    LockType lock( m_mutex );
    return m_latestReport;
}

UbusMetrics& InternalMetrics::bus() {
    return m_busMetrics;
}

const UbusMetrics& InternalMetrics::bus() const {
    return m_busMetrics;
}

}  // namespace Metrics
}  // namespace HubPlatform