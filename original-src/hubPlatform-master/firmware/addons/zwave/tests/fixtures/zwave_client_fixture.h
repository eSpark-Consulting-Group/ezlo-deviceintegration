#pragma once

#include "zwave/tests/fixtures/zwave_addon_fixture.h"

#include "zwave/zwave_client.h"

namespace HubPlatform {
namespace Zwave {
namespace UT {

class ZwaveClientFixture : public ZwaveAddonFixture {
public:
    ZwaveClient createZwaveClient() {
        return ZwaveClient( getHostService() );
    }
};

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
