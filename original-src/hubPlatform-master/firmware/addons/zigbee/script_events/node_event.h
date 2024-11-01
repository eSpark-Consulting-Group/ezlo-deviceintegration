#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zigbee {

class ZigbeeNode;

class NodeEvent : public Services::ScriptEvent {
protected:
    explicit NodeEvent( std::string eventName, uint16_t nodeId );

    Json formatObject( Json&& object ) const;

public:
    Json toJson() const override;

private:
    uint16_t m_nodeId;
};

}  // namespace Zigbee
}  // namespace HubPlatform
