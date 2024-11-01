#pragma once

#include "cored/tests/service/test_core_service.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/house_modes/house_mode_utils.h"

#include "config_reader/tests/config_reader_factory.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"

#include <gtest/gtest.h>

namespace HubPlatform {

namespace CoreD {
class CoreService;
class HouseModeProcessor;
class DeviceDbModifier;
namespace Model {
class Device;
class Item;
}  // namespace Model
}  // namespace CoreD

namespace UT {

class HouseModesProcessorFixture : public ::testing::Test, public ConfigReaderFactory {
public:
    using Service = HubPlatform::CoreD::CoreService;
    using HouseModeProcessor = HubPlatform::CoreD::HouseModeProcessor;
    using RemoteInterface = HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface;
    using Device = HubPlatform::CoreD::Model::Device;
    using Item = HubPlatform::CoreD::Model::Item;
    using DeviceDbModifier = HubPlatform::CoreD::DeviceDbModifier;
    using DeviceIDs = CoreDHouseModes::DeviceIDs;

protected:
    void TearDown() override;

    Service& getService();
    Service& getBroadcaster();
    IDbClient& getDBClient();
    DeviceDbModifier& getDevicesDbModifier();
    const Services::UT::FakeResponseBus& getTestBus();

    void initService( const std::string& data );
    DeviceIDs initDevices( const std::vector< Device >& devices );
    void initItems( const std::vector< Json >& items );
    void initHouseModeProcessor( const std::string& data );
    void initRemoteInterface();
    void waitSwitchingHouseMode();

    static JsonObject generateHouseModes();
    static std::vector< Device > generateDevices( std::string gatewayId = "" );
    std::vector< Json > generateSecurityItem( const std::string& deviceId );
    Json generateSwitchItem( const std::string& deviceId );
    CoreD::CoreService& getCoreDService();
    bool setItemValue( const JsonObject& idValue, const std::string& operationId );
    CoreD::CoreService::GatewayID ensureGateway( const Json& gate );

protected:
    std::unique_ptr< CoreD::Tests::TestCoreService > m_service;
    std::unique_ptr< HouseModeProcessor > m_houseModeProcessor;
    std::unique_ptr< RemoteInterface > m_remoteInterface;
};

}  // namespace UT
}  // namespace HubPlatform
