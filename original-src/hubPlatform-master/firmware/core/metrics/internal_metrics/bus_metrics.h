#pragma once

#include <cstddef>
#include <chrono>
#include "json_utils/json_fwd.h"
#include <mutex>

namespace HubPlatform {
namespace Metrics {

using TimePoint = std::chrono::system_clock::time_point;

class UbusMetrics {
public:
    UbusMetrics() = default;

    virtual Json toJson() const;

    void registerAttempt();
    void registerRequest();
    void registerError();
    size_t attempts() const;
    std::time_t attemptAt() const;
    ;
    size_t requests() const;
    size_t errors() const;
    void connectionCheckFailed();
    size_t connectionCheckFailed() const;
    std::time_t connectionCheckFailedAt() const;
    void reconnectFailed();
    size_t reconnectFailed() const;
    std::time_t reconnectFailedAt() const;
    void reconnectSuccess();
    size_t reconnectSuccess() const;
    std::time_t reconnectSuccessAt() const;

private:
    using MutexType = std::recursive_mutex;
    using LockType = std::lock_guard< MutexType >;
    mutable MutexType m_mutex;

    size_t m_attempts{ 0 };
    TimePoint m_attemptAt = TimePoint::min();
    size_t m_requests{ 0 };
    size_t m_errors{ 0 };

    size_t m_connectionChecks{ 0 };
    TimePoint m_connectionCheckAt = TimePoint::min();
    size_t m_reconnectSuccess{ 0 };
    TimePoint m_reconnectSuccessAt = TimePoint::min();
    size_t m_reconnectFailed{ 0 };
    TimePoint m_reconnectFailedAt = TimePoint::min();
};

}  // namespace Metrics
}  // namespace HubPlatform