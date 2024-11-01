#pragma once

#include "bluetooth/proxy_method_helpers/proxy_sync_method_helper.h"

#include <vector>

namespace HubPlatform {

class WriteCharacteristicHelper final : public ProxySyncMethodHelper {
public:
    WriteCharacteristicHelper( Proxy& proxy, const std::vector< std::uint8_t >& data );

protected:
    void setup( DBusMessageIter* iter ) override;

private:
    std::vector< std::uint8_t > m_data;
};

}  // namespace HubPlatform
