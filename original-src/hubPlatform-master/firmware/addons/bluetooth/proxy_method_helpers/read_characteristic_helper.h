#pragma once

#include "bluetooth/proxy_method_helpers/proxy_sync_method_helper.h"

#include <vector>

namespace HubPlatform {

class ReadCharacteristicHelper final : public ProxySyncMethodHelper {
public:
    ReadCharacteristicHelper( Proxy& proxy );

    const std::vector< std::uint8_t >& getResult() const;

protected:
    void setup( DBusMessageIter* iter ) override;
    void parseResult( DBusMessage* message ) override;

private:
    std::vector< std::uint8_t > m_value;
};

}  // namespace HubPlatform
