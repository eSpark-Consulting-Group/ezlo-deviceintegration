#pragma once

#include "vidoo4hub/tests/utils/test_utils.h"

#include "cored/exceptions.h"
#include "cored/remote_interface/cored_remote_interface.h"

namespace HubPlatform {
namespace UT {

class FakeCoreDRemoteInterface : public HubPlatform::CoreDRemoteInterface::CoreDRemoteInterface {
public:
    using HubPlatform::CoreDRemoteInterface::CoreDInterface::DeviceAddResult;
    using HubPlatform::CoreDRemoteInterface::CoreDInterface::GatewayEnsureResult;
    using HubPlatform::CoreDRemoteInterface::CoreDInterface::ItemAddResult;

    FakeCoreDRemoteInterface( Services::Service& hostService )
    : CoreDRemoteInterface( hostService ) {}

    DeviceAddResult addDevice( const CoreD::Model::Device& device ) override {
        auto id = getUniqueID();
        m_devices.emplace( id );
        return { id, true };
    }

    ItemAddResult addItem( const CoreD::Model::Item& item ) override {
        auto id = getUniqueID();
        m_items.emplace( id, item.toJson() );
        return { id, true };
    }

    bool modifyItem( const ItemID& itemId, const CoreD::Model::Item& item ) override {
        m_items.erase( itemId );
        m_items.emplace( itemId, item.toJson() );
        return true;
    }

    void removeDevice( const DeviceID& deviceId ) override {
        m_devices.erase( deviceId );
    }

    void removeItem( const ItemID& itemId ) override {
        m_items.erase( itemId );
    }

    JsonObject getItem( const ItemID& itemId ) const override {
        return m_items.at( itemId );
    }

    GatewayEnsureResult ensureGateway( const CoreD::Model::Gateway& gateway ) override {
        return { "12345", true };
    }

public:
    std::set< std::string > m_devices;
    std::map< std::string, Json > m_items;
};

}  // namespace UT
}  // namespace HubPlatform
