#pragma once

#include "cored/house_modes/protect_service_enum.h"
#include <utility>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace CoreDHouseModes {

class ProtectButton {
public:
    ProtectButton() = default;
    ProtectButton( std::string deviceId, ProtectServiceEnum service );

    bool empty() const;
    const std::string& deviceId() const;
    void deviceId( const std::string& deviceId );
    ProtectServiceEnum service() const;
    void service( ProtectServiceEnum service );

    Json toJson() const;
    bool fromJson( const Json& source );
    bool fromDbJson( const Json& source );

    bool operator==( const ProtectButton& r ) const;

private:
    std::string m_deviceId;
    ProtectServiceEnum m_service = ProtectServiceEnum::NONE;
};

}  // namespace CoreDHouseModes
}  // namespace HubPlatform
