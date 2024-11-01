#pragma once

#include "service_interfaces/tests/fixtures/service_fixture.h"
#include "service_interfaces/tests/fakes/fake_response_bus.h"
#include "service_interfaces/bus_wrapper.h"

#include "zwave/zwave_addon.h"
#include "zwave/tests/mocks/znet_mock.h"
#include "zwave/znet_factory.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Zwave {
namespace UT {

class TestZwaveAddon : public ZwaveAddon {
public:
    TestZwaveAddon()
    : ZwaveAddon( "zwave_addon_name",
                  "tty_path",
                  "zwave_store_file_path",
                  std::make_unique< Services::BusWrapper >(
                      Services::BusWrapper::BusTypeTag< Services::UT::FakeResponseBus >{} ),
                  std::make_unique< HubPlatform::UT::FakeDbClient >(),
                  std::make_unique< ZWaveStick >(),
                  std::make_unique< ZNetFactory >( ZNetFactory::ZNetTypeTag< ZNetMock >{} ) ) {
        setTestMode();
    }
};

class ZwaveAddonFixture : public Services::UT::ServiceFixture {
public:
    TestZwaveAddon& getHostService() final {
        return m_zwaveAddon;
    }

public:
    TestZwaveAddon& getAddon() {
        return getHostService();
    }

public:
    znet::ZNode& createNode( uint8_t nodeId ) {
        getAddon().nodeCreate( nodeId );
        return *getAddon().onFindNode( nodeId );
    }

    void addNode( znet::ZNode& node ) {
        getAddon().interviewDone( node.getId() );
    }

    znet::ZNode& addNode( uint8_t nodeId ) {
        auto& node = createNode( ( nodeId ) );
        addNode( node );
        return node;
    }

private:
    TestZwaveAddon m_zwaveAddon;
};

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
