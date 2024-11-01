#pragma once

#include "json_utils/json_fwd.h"
#include "luad/modules/network/dhcp/dhcp.h"
#include "luad/modules/network/network_scan.h"
#include <memory>

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

class Subscription final {
public:
    using Operations = std::vector< std::string >;

    Subscription( std::string handlerPath, Operations operations );
    Subscription() = default;
    ~Subscription() = default;

    bool hasOperations() const;
    bool contains( const std::string& eventName ) const;
    const std::string& scriptPath() const;

private:
    void sortUnique();

private:
    std::string m_path;
    Operations m_operations;
};

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform