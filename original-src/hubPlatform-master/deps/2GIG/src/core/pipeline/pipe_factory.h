#pragma once

#include "utils/thread/thread_safe_wrapper.h"

#include <memory>

namespace TwoGig {

struct Client;
struct Timer;
class Logger;
class SerialPort;
class DevicesRepository;
class DevicesReachabilityManager;

class SerialPortReader;
class PacketCollector;
class PacketValidator;
class PacketSorter;
template < typename T >
class PacketConverter;
class DeviceFilter;
class DeviceCollector;
class PacketDuplicationFilter;
class PinCodeFilter;
class PinCodeController;
class TransmissionHandler;
class ReachabilityHandler;

class PipeFactory {
public:
    PipeFactory( const Logger& logger,
                 const Timer& timer,
                 const DevicesRepository& devices,
                 SerialPort& port,
                 Client& client,
                 DevicesReachabilityManager& reachabilityManager );

    std::unique_ptr< SerialPortReader > createSerialPortReader() const;

    std::unique_ptr< PacketCollector > createPacketCollector() const;

    std::unique_ptr< PacketValidator > createPacketValidator() const;

    std::unique_ptr< PacketSorter > createPacketSorter() const;

    template < typename To >
    std::unique_ptr< PacketConverter< To > > createPacketConverter() const;

    std::unique_ptr< DeviceFilter > createDeviceFilter() const;

    std::unique_ptr< DeviceCollector > createDeviceCollector() const;

    std::unique_ptr< PacketDuplicationFilter > createPacketDuplicationFilter() const;

    std::unique_ptr< PinCodeFilter > createPinCodeFilter() const;

    std::unique_ptr< PinCodeController > createPinCodeController() const;

    std::unique_ptr< TransmissionHandler > createTransmissionHandler() const;

    std::unique_ptr< ReachabilityHandler > createReachabilityHandler() const;

private:
    const Logger& m_logger;
    const Timer& m_timer;
    const DevicesRepository& m_devices;
    SerialPort& m_port;
    Client& m_client;
    DevicesReachabilityManager& m_reachabilityManager;
};

}  // namespace TwoGig
