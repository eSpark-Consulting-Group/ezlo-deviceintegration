#pragma once

#include "cored/model/cored_device.h"
#include "cored/model/cored_item.h"
#include "json_utils/json_fwd.h"
#include "cored/remote_interface/cored_interface.h"

#include <functional>

namespace HubPlatform {
namespace CloudDevices {

class ISubscriptionHelper {
public:
    using Id = std::string;
    using Result = std::pair< Id, bool >;
    using RequestResponseHandler = std::function< void( const JsonObject& ) >;
    using RequestTimeoutHandler = std::function< void() >;

    virtual Result addDevice( const CoreD::Model::Device& device ) = 0;
    virtual Result addItem( const CoreD::Model::Item& item ) = 0;

    virtual void modifyItem( const std::string& itemId, const CoreD::Model::Item& item ) = 0;

    virtual void removeDevice( const Id& deviceId ) = 0;
    virtual void removeItem( const Id& itemId ) = 0;

    virtual Json getItem( const Id& itemId ) = 0;
    virtual Json getDevice( const Id& deviceId ) = 0;
    virtual std::string getGatewayId() = 0;
    virtual Json getGatewayByName( const std::string& gatewayName ) = 0;
    virtual Json getGatewayById( const std::string& id ) = 0;
    virtual bool itemBelongsToGateway( const std::string& itemId, const std::string& gatewayName ) = 0;
    virtual void sendExecutePluginScript( const std::string& script, const Json& scriptParams ) = 0;

    virtual void sendSubscriptionList( Json message ) = 0;
    virtual void tearDown() = 0;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
