#pragma once
#include "protect_remote_interface.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace Services {
class Service;
}

namespace CoreD {
namespace Protect {
class ProtectSecurityConfigInterface final : public ProtectRemoteInterface {
public:
    ProtectSecurityConfigInterface( Services::Service& service );
    ~ProtectSecurityConfigInterface() override;

    JsonObject list( const JsonObject& params ) override;
    bool isBypassSet( const std::string& securityConfigId );

private:
    Services::Service& m_hostService;
};

}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform