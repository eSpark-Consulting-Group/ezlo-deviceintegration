#include "bus_metrics.h"
#include "json_utils/json_wrapper.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Metrics {

using namespace std::chrono;

Json UbusMetrics::toJson() const {
    LockType lock( m_mutex );

    Json result = JSON_EMPTY_OBJ;
    time_t timestamp;
    json_add_int64( result, "requests", m_requests );
    json_add_int64( result, "errors", m_errors );
    json_add_int64( result, "attempts", m_attempts );
    timestamp = system_clock::to_time_t( m_attemptAt );
    json_add_long( result, "attemptAt", timestamp < 0 ? 0 : timestamp );

    json_add_int64( result, "connectionChecks", m_connectionChecks );
    timestamp = system_clock::to_time_t( m_connectionCheckAt );
    json_add_int64( result, "connectionCheckAt", timestamp < 0 ? 0 : timestamp );

    json_add_int64( result, "reconnectSuccess", m_reconnectSuccess );
    timestamp = system_clock::to_time_t( m_reconnectSuccessAt );
    json_add_int64( result, "reconnectSuccessAt", timestamp < 0 ? 0 : timestamp );

    json_add_int64( result, "reconnectFailed", m_reconnectFailed );
    timestamp = system_clock::to_time_t( m_reconnectFailedAt );
    json_add_int64( result, "reconnectFailedAt", timestamp < 0 ? 0 : timestamp );

    return result;
}

void UbusMetrics::registerAttempt() {
    LockType lock( m_mutex );
    m_attempts++;
    m_attemptAt = system_clock::now();
}

void UbusMetrics::registerRequest() {
    LockType lock( m_mutex );
    m_requests++;
}

void UbusMetrics::registerError() {
    LockType lock( m_mutex );
    m_errors++;
}

size_t UbusMetrics::requests() const {
    LockType lock( m_mutex );
    return m_requests;
}

size_t UbusMetrics::errors() const {
    LockType lock( m_mutex );
    return m_errors;
}

size_t UbusMetrics::attempts() const {
    LockType lock( m_mutex );
    return m_attempts;
}

std::time_t UbusMetrics::attemptAt() const {
    LockType lock( m_mutex );
    return system_clock::to_time_t( m_attemptAt );
}

void UbusMetrics::connectionCheckFailed() {
    LockType lock( m_mutex );
    m_connectionChecks++;
    m_connectionCheckAt = system_clock::now();
}

size_t UbusMetrics::connectionCheckFailed() const {
    LockType lock( m_mutex );
    return m_connectionChecks;
}

std::time_t UbusMetrics::connectionCheckFailedAt() const {
    LockType lock( m_mutex );
    return system_clock::to_time_t( m_connectionCheckAt );
}

void UbusMetrics::reconnectFailed() {
    LockType lock( m_mutex );
    m_reconnectFailed++;
    m_reconnectFailedAt = system_clock::now();
}

size_t UbusMetrics::reconnectFailed() const {
    LockType lock( m_mutex );
    return m_reconnectFailed;
}

std::time_t UbusMetrics::reconnectFailedAt() const {
    LockType lock( m_mutex );
    return system_clock::to_time_t( m_connectionCheckAt );
}

void UbusMetrics::reconnectSuccess() {
    LockType lock( m_mutex );
    m_reconnectSuccess++;
    m_reconnectSuccessAt = system_clock::now();
}

size_t UbusMetrics::reconnectSuccess() const {
    LockType lock( m_mutex );
    return m_reconnectSuccess;
}

std::time_t UbusMetrics::reconnectSuccessAt() const {
    LockType lock( m_mutex );
    return system_clock::to_time_t( m_connectionCheckAt );
}

}  // namespace Metrics
}  // namespace HubPlatform