#pragma once

namespace HubPlatform {
namespace CoreD {
namespace Protect {

class ProtectInfo {
public:
    ProtectInfo() = default;
    virtual Json toJson() const = 0;
    virtual bool fromJson( const JsonObject& data ) = 0;
};
}  // namespace Protect
}  // namespace CoreD
}  // namespace HubPlatform