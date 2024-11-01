#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <functional>

#include "2gig/device_type.h"

namespace TwoGig {
namespace DefaultConfiguration {

constexpr uint8_t PinCodeLength = 8;
constexpr std::chrono::seconds PinCodeInputTimeout( 5 );
constexpr ::TwoGig::DeviceType::Enum DeviceType = ::TwoGig::DeviceType::Enum::SmokeDetector;

}  // namespace DefaultConfiguration

class DevicesRepository {
public:
    virtual ~DevicesRepository() = default;

    virtual bool addDevice( const std::vector< uint32_t >& txIds ) = 0;
    virtual bool addDevice( const std::vector< uint32_t >& txIds, DeviceType::Enum deviceType ) = 0;
    virtual bool removeDevice( const std::vector< uint32_t >& txIds ) = 0;
    virtual bool hasDevice( uint32_t txId ) const = 0;
    virtual void forEachDevice( std::function< void( uint32_t ) > callback ) const = 0;

    virtual ::TwoGig::DeviceType::Enum getDeviceType( uint32_t txId ) const = 0;

    virtual uint8_t getPinCodeLength( uint32_t txId ) const = 0;
    virtual void setPinCodeLength( uint32_t txId, uint8_t length ) = 0;

    virtual std::chrono::seconds getPinCodeInputTimeout( uint32_t txId ) const = 0;
    virtual void setPinCodeInputTimeout( uint32_t txId, std::chrono::seconds duration ) = 0;

    virtual void reset() = 0;
};

}  // namespace TwoGig
