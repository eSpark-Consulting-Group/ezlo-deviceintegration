#pragma once

#include "2gig/devices_repository.h"

#include <set>
#include <map>

namespace TwoGig {
namespace Test {

class FakeDevicesRepository : public DevicesRepository {
    struct Configuration {
        uint8_t pinLength = DefaultConfiguration::PinCodeLength;
        std::chrono::seconds pinInputTimeout = DefaultConfiguration::PinCodeInputTimeout;
        DeviceType::Enum deviceType = DeviceType::SmokeDetector;
    };

public:
    bool addDevice( const std::vector< uint32_t >& txIds ) override {
        const size_t sizeBefore = m_devices.size();

        std::transform( txIds.begin(), txIds.end(), std::inserter( m_devices, m_devices.end() ), []( uint32_t txId ) {
            return std::make_pair( txId, Configuration{} );
        } );

        return m_devices.size() - sizeBefore == txIds.size();
    }

    bool addDevice( const std::vector< uint32_t >& txIds, DeviceType::Enum deviceType ) override {
        const size_t sizeBefore = m_devices.size();

        Configuration config;
        config.deviceType = deviceType;

        std::transform(
            txIds.begin(), txIds.end(), std::inserter( m_devices, m_devices.end() ), [config]( uint32_t txId ) {
                return std::make_pair( txId, config );
            } );

        return m_devices.size() - sizeBefore == txIds.size();
    }

    bool removeDevice( const std::vector< uint32_t >& txIds ) override {
        size_t elementsErased = 0;

        for( uint32_t txId : txIds )
            elementsErased += m_devices.erase( txId );

        return elementsErased == txIds.size();
    }

    bool hasDevice( uint32_t txId ) const override {
        return m_devices.find( txId ) != m_devices.end();
    }

    void forEachDevice( std::function< void( uint32_t ) > callback ) const {
        for( auto const& device : m_devices ) {
            callback( device.first );
        }
    }

    ::TwoGig::DeviceType::Enum getDeviceType( uint32_t txId ) const {
        return getConfiguration( txId ).deviceType;
    }

    uint8_t getPinCodeLength( uint32_t txId ) const override {
        return getConfiguration( txId ).pinLength;
    }

    void setPinCodeLength( uint32_t txId, uint8_t length ) override {
        getConfiguration( txId ).pinLength = length;
    }

    std::chrono::seconds getPinCodeInputTimeout( uint32_t txId ) const override {
        return getConfiguration( txId ).pinInputTimeout;
    }

    void setPinCodeInputTimeout( uint32_t txId, std::chrono::seconds duration ) override {
        getConfiguration( txId ).pinInputTimeout = duration;
    }

    void reset() override {
        m_devices.clear();
    }

private:
    Configuration& getConfiguration( uint32_t txId ) {
        return m_devices[ txId ];
    }

    const Configuration& getConfiguration( uint32_t txId ) const {
        static const Configuration defaultConfiguration;

        auto it = m_devices.find( txId );
        return it != m_devices.end() ? it->second : defaultConfiguration;
    }

private:
    std::map< uint32_t, Configuration > m_devices;
};

}  // namespace Test
}  // namespace TwoGig
