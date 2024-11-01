#pragma once

#include "json_utils/json_wrapper.h"
#include <mutex>

namespace HubPlatform {
namespace HttpBridge {

class MetricsBase {
public:
    virtual ~MetricsBase() = default;

    virtual Json toJson() {
        return JSON_EMPTY_OBJ;
    }
    virtual void fromJson( const Json& data ) {}

public:
};

/** Class represents HTTP server metrics in general: number of requests, traffic, errors, cummulative
 * HTTP server
number of received requests
number of handled requests
number of received bytes
number of transmitted bytes
number of errors
*/
class HttpServerMetrics : public MetricsBase {
public:
    HttpServerMetrics() = default;
    ~HttpServerMetrics() override = default;

    Json toJson() override;
    void fromJson( const Json& data ) override;

public:
    void incrementReceivedRequest();
    void incrementHandledRequest();
    void incrementErrorRequest();
    void incrementWrongAuthRequest();
    void incrementRequestTimedOut();
    void incrementRequestsLimitReached();
    void appendRxBytes( size_t size );
    void appendTxBytes( size_t size );
    void incrementUnhandledRequest();
    void parallelRequests( size_t parallelCount );

    // Possible to add timeout, closed (not regularly finished)

private:
    using MutexType = std::mutex;
    using LockType = std::unique_lock< MutexType >;
    mutable MutexType m_mutex;

    size_t m_received{ 0 };
    size_t m_handled{ 0 };
    size_t m_errors{ 0 };
    size_t m_wrongAuth{ 0 };
    size_t m_timedOut{ 0 };
    size_t m_limitReached{ 0 };
    uint64_t m_rxBytes{ 0 };
    uint64_t m_txBytes{ 0 };
    size_t m_closedRequests{ 0 };  //!< request closed without being handled
    size_t m_parallelRequests{ 0 };
};

}  // namespace HttpBridge
}  // namespace HubPlatform