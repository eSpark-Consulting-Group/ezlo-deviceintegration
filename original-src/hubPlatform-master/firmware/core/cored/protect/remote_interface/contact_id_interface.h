#pragma once
#include "protect_remote_interface.h"
#include "json_utils/json_fwd.h"

namespace HubPlatform {

namespace Services {
class Service;
}

namespace CoreD {
namespace Protect {
class ProtectContactIdInterface final : public ProtectRemoteInterface {
public:
    ProtectContactIdInterface( Services::Service& service );
    ~ProtectContactIdInterface() override;

    JsonObject list( const JsonObject& params ) override;

private:
    Services::Service& m_hostService;
};

}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform