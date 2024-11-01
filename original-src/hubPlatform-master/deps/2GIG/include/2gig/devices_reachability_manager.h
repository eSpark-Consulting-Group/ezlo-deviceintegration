#pragma once

#include <cstdint>
#include <vector>

namespace TwoGig {

class DevicesReachabilityManager {
public:
    virtual ~DevicesReachabilityManager() = default;

    virtual void onDeviceAdded( const std::vector< uint32_t >& txIds ) = 0;
    virtual void onDeviceRemoved( const std::vector< uint32_t >& txIds ) = 0;

    virtual void checkAllDevicesReachableStates() = 0;
    virtual void onDeviceReceivedData( uint32_t txId ) = 0;
};

}  // namespace TwoGig
