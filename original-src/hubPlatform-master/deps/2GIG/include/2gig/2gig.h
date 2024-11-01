#pragma once

#include <memory>
#include <vector>

namespace TwoGig {

struct Client;
struct InstrumentationClient;
struct DeviceDescriptor;
class DevicesRepository;
class DevicesReachabilityManager;

struct TwoGig {
    virtual ~TwoGig() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void reset() = 0;

    virtual void startDeviceDiscovering() = 0;
    virtual void stopDeviceDiscovering() = 0;

    virtual void addDevice( const DeviceDescriptor& descriptor ) = 0;
    virtual void removeDevice( const std::vector< uint32_t >& txIds ) = 0;
};

extern std::unique_ptr< TwoGig > createInstance( const std::string& devicePath,
                                                 const std::string& resetPinGpio,
                                                 Client& client,
                                                 DevicesRepository& devicesRepository,
                                                 DevicesReachabilityManager& reachabilityManager,
                                                 InstrumentationClient* instrumentationClient = nullptr );

}  // namespace TwoGig
