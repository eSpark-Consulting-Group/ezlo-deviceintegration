#pragma once

#include "json_utils/json_utils.h"
#include "ubus_interface/ubus_impl.h"
#include "metrics/internal_metrics/internal_metrics.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <memory>

namespace HubPlatform {
namespace Metrics {

class DataSender;

class DataReceiver {
public:
    using TimePoint = std::chrono::system_clock::time_point;

public:
    explicit DataReceiver( InternalMetricsPtr internalMetricsPtr );
    ~DataReceiver();

    void startGetDataLoop( const int& requestIntervalInSec );

    void stopGetDataLoop();

    DataSender* getSender() const;
    const InternalMetrics* getInternalMetrics() const;
    bool isRunning() const;
    void shutdown();

private:
    void printMetricsStat();
    void emitConnectionCheckFailed();
    void emitUbusReconnectFailed();
    void emitUbusReconnectSuccess();

private:
    std::unique_ptr< DataSender > m_sender;
    InternalMetricsPtr m_internalMetricsPtr;
    std::atomic_bool m_isRunning{ false };
    std::mutex m_intervalMutex;
    std::condition_variable m_intervalCondition;
};

}  // namespace Metrics
}  // namespace HubPlatform
