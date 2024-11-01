#pragma once

#include "2gig/devices_reachability_manager.h"
#include "2gig/client.h"
#include "2gig/instrumentation_client.h"
#include "2gig/devices_repository.h"
#include "tiny/include/yamc/yamc_shared_lock.hpp"
#include "tiny/include/yamc/alternate_shared_mutex.hpp"
#include "json_utils/json_utils.h"

#include <unordered_set>
#include <unordered_map>

namespace HubPlatform {
namespace TwoGig {

namespace {}

class DevicesReachabilityManagerImpl : public ::TwoGig::DevicesReachabilityManager {
public:
    DevicesReachabilityManagerImpl(
        ::TwoGig::Client& client,
        ::TwoGig::DevicesRepository& devicesRepository,
        std::chrono::steady_clock::duration unreachableInterval = ms_DefaultUnreachableInterval );

    ~DevicesReachabilityManagerImpl() override;

    void onDeviceAdded( const std::vector< uint32_t >& txIds ) override;
    void onDeviceRemoved( const std::vector< uint32_t >& txIds ) override;

    void checkAllDevicesReachableStates() override;
    void onDeviceReceivedData( uint32_t txId ) override;

    void getDevicesUpdateTimes( JsonObject& twoGigDeviceUpdateTimes );

private:
    void init();

    ::TwoGig::Client& m_client;
    ::TwoGig::DevicesRepository& m_devices;

    std::chrono::steady_clock::duration m_unreachableInterval;

    std::unordered_map< uint32_t, std::chrono::steady_clock::time_point > m_nodeToLatsUpdateTime;
    std::unordered_set< uint32_t > m_nodesToIgnoreTrueReachable;

    mutable yamc::alternate::shared_mutex m_cacheMutex;
    static constexpr std::chrono::minutes ms_DefaultUnreachableInterval{ 130 };
};

}  // namespace TwoGig
}  // namespace HubPlatform
