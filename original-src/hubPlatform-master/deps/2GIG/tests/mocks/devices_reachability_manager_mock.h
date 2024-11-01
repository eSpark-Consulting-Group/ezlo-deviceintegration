#pragma once

#include "2gig/devices_reachability_manager.h"

#include <gmock/gmock.h>

namespace TwoGig {
namespace Test {

class DevicesReachabilityManagerMock : public DevicesReachabilityManager {
public:
    MOCK_METHOD1( onDeviceAdded, void( const std::vector< uint32_t >& ) );
    MOCK_METHOD1( onDeviceRemoved, void( const std::vector< uint32_t >& ) );

    MOCK_METHOD0( checkAllDevicesReachableStates, void() );
    MOCK_METHOD1( onDeviceReceivedData, void( uint32_t ) );
};

}  // namespace Test
}  // namespace TwoGig
