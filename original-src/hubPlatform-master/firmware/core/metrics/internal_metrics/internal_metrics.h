#pragma once

#include "bus_metrics.h"
#include <cstddef>
#include <chrono>
#include "json_utils/json_fwd.h"
#include <mutex>

namespace HubPlatform {
namespace Metrics {

using TimePoint = std::chrono::system_clock::time_point;

class InternalMetrics {
public:
    InternalMetrics();

    size_t packetsSent() const;
    size_t sentSize();
    size_t sentErrors() const;

    void registerSend( size_t packetSize );
    void registerError();
    size_t sequenceNumber() const;

    UbusMetrics& bus();
    const UbusMetrics& bus() const;

    virtual Json toJson() const;

    // Not a part of metrics, used to print report periodically in logs
    void saveLatestReport( Json report );
    const Json& getLatestReport() const;

private:
    using MutexType = std::mutex;
    using LockType = std::lock_guard< MutexType >;
    mutable MutexType m_mutex;

    size_t m_packets{ 0 };
    size_t m_sentSize{ 0 };
    size_t m_packetSentErrors{ 0 };

    TimePoint m_sentAt;
    TimePoint m_errorAt;

    UbusMetrics m_busMetrics;

    Json m_latestReport;
};

using InternalMetricsPtr = std::shared_ptr< InternalMetrics >;

}  // namespace Metrics
}  // namespace HubPlatform