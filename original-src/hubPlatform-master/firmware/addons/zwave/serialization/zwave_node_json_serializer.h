#pragma once

#include "json_utils/json_fwd.h"
#include <znet/enums/security_mode.hpp>

namespace znet {
class ZNode;
}

namespace HubPlatform {
namespace Zwave {

class ZwaveNodeJsonSerializer {
public:
    explicit ZwaveNodeJsonSerializer( const znet::ZNode& zwaveNode );

    Json serialize() const;

private:
    JsonArray serializeChannels() const;
    JsonArray serializeCommandClasses( std::uint8_t channelId ) const;

    Json serializeIconType( uint16_t iconType ) const;
    void serializeGenricAndSpecificTypes( uint8_t genericType, uint8_t specificType, Json& result ) const;
    JsonArray convertSecurityToToken( znet::SecurityMode::Class security ) const;

private:
    const znet::ZNode& m_zwaveNode;
};

}  // namespace Zwave
}  // namespace HubPlatform
