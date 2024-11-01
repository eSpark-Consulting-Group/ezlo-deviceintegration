#pragma once

#include "2gig/devices_repository.h"
#include "json_utils/json_fwd.h"

#include <fstream>
#include <memory>

class IDbClient;

namespace TwoGig {

class PersistenceClient;
}

namespace HubPlatform {
namespace TwoGig {

class DevicesRepositoryImpl : public ::TwoGig::DevicesRepository {
public:
    DevicesRepositoryImpl( IDbClient& database );

public:
    ~DevicesRepositoryImpl() override;

    bool addDevice( const std::vector< uint32_t >& txIds ) override;
    bool addDevice( const std::vector< uint32_t >& txIds, ::TwoGig::DeviceType::Enum deviceType ) override;
    bool removeDevice( const std::vector< uint32_t >& txIds ) override;
    bool hasDevice( uint32_t txId ) const override;
    void forEachDevice( std::function< void( uint32_t ) > callback ) const override;

    ::TwoGig::DeviceType::Enum getDeviceType( uint32_t txId ) const override;

    uint8_t getPinCodeLength( uint32_t txId ) const override;
    void setPinCodeLength( uint32_t txId, uint8_t length ) override;

    std::chrono::seconds getPinCodeInputTimeout( uint32_t txId ) const override;
    void setPinCodeInputTimeout( uint32_t txId, std::chrono::seconds duration ) override;

    void reset() override;

    bool migrateOldDevices(::TwoGig::PersistenceClient& oldStorage );

private:
    void updateDevice( uint32_t txId, const Json& diff );

private:
    IDbClient& m_database;
};

}  // namespace TwoGig
}  // namespace HubPlatform
