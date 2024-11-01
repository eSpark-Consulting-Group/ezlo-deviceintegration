#pragma once

#include "vidoo4hub/tests/utils/test_utils.h"

#include "cored/remote_interface/cored_remote_interface.h"
#include "vidoo4hub/vidoo4hub_exception.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/exceptions.h"
#include "cored/exceptions.h"

namespace HubPlatform {
namespace UT {

class FakeCoreDRemoteInterface : public CoreDRemoteInterface::CoreDRemoteInterface {
public:
    using HubPlatform::CoreDRemoteInterface::CoreDInterface::DeviceAddResult;
    using HubPlatform::CoreDRemoteInterface::CoreDInterface::ItemAddResult;
    using ValueLoader = HubPlatform::CoreDRemoteInterface::ValueLoader;

    FakeCoreDRemoteInterface( Services::Service& hostService )
    : CoreDRemoteInterface::CoreDRemoteInterface( hostService ) {}

    DeviceAddResult addDevice( const CoreD::Model::Device& device ) override {
        auto id = getUniqueID();
        auto json = JSON( CoreD::Model::Resources::DeviceFields::IdFieldName << id );
        m_devices.emplace( id, std::move( json ) );
        return { id, true };
    }

    JsonObject setDeviceArmed( const DeviceID& deviceId, bool armed ) override {
        if( !m_devices.count( deviceId ) )
            throw HubPlatform::CoreD::DeviceNotFoundError{ deviceId };
        return {};
    }

    ItemAddResult addItem( const CoreD::Model::Item& item ) override {
        auto id = getUniqueID();
        m_items.insert( { id, item.toJson() } );
        return { id, true };
    }

    JsonObject getItem( const ItemID& itemId ) const override {
        return m_items.at( itemId );
    }

    void removeDevice( const DeviceID& deviceId ) override {
        m_devices.erase( deviceId );
    }

    void removeItem( const ItemID& itemId ) override {
        m_items.erase( itemId );
    }

    void updateItemValue( const ItemID& itemId, ValueLoader valueLoader ) override {
        using namespace HubPlatform::CoreD::Model;

        auto it = m_items.find( itemId );
        if( it == m_items.cend() )
            throw CoreD::ItemNotFoundError{ itemId };

        auto& itemJson = it->second;

        const auto typeStr = json_get_str( itemJson, CoreD::Model::Resources::ItemFields::TypeFieldName );
        const auto valueType = CoreD::Model::ValueType::fromString( typeStr );

        auto newValue = valueLoader( valueType );
        if( newValue == nullptr )
            throw CoreD::Model::ValueError{ "Invalid value for item: " + itemId };

        itemJson.at( CoreD::Model::Resources::ItemFields::ValueFieldName ) = newValue->toJson();
    }

    Json& getItemValue( const std::string& itemId ) {
        return m_items.at( itemId ).at( CoreD::Model::Resources::ItemFields::ValueFieldName );
    }

    void updateDeviceName( const DeviceID& deviceId, const std::string& deviceName ) override {
        // CoreD::Device have private members
        auto it = m_devices.find( deviceId );
        if( it == m_devices.cend() )
            return;
        auto& json = it->second;
        json[ CoreD::Model::Resources::DeviceFields::NameFieldName ] = deviceName;
    }

public:
    std::map< std::string, JsonObject > m_devices;
    std::map< std::string, JsonObject > m_items;
};

}  // namespace UT
}  // namespace HubPlatform
