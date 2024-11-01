#pragma once

#include "service_interfaces/script_event.h"
#include <string>

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class BaseEvent : public Services::ScriptEvent {
public:
    BaseEvent( const std::string& event, const Json& json );

    Json toJson() const override;

protected:
    Json m_json;
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
