#pragma once

#include "service_interfaces/script_event.h"
namespace HubPlatform {
namespace Zigbee {

class NetworkInfoUpdated : public Services::ScriptEvent {
public:
    explicit NetworkInfoUpdated();
};

}  // namespace Zigbee
}  // namespace HubPlatform
