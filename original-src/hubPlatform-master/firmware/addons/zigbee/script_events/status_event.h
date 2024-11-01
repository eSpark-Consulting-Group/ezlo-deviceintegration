#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zigbee {

enum class ModeStatus;

class StatusEvent : public Services::ScriptEvent {
protected:
    StatusEvent( std::string eventName, ModeStatus status );

public:
    Json toJson() const override;

private:
    const ModeStatus m_status;
};

}  // namespace Zigbee
}  // namespace HubPlatform
