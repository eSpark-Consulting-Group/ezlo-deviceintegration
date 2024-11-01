#pragma once

#include "zwave/resources/zwave_resources.h"

#include "service_interfaces/script_event.h"

#include "znet/iznet.hpp"

namespace HubPlatform {
namespace Zwave {

class ActionBusy : public Services::ScriptEvent {
public:
    ActionBusy( znet::IZNet::Mode mode );

    Json toJson() const override;

private:
    const znet::IZNet::Mode m_mode;
};

}  // namespace Zwave
}  // namespace HubPlatform
