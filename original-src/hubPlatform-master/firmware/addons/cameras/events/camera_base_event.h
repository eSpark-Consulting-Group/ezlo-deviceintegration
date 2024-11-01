#pragma once
#include "service_interfaces/script_event.h"

#include <string>

namespace HubPlatform {

class CameraBaseEvent : public Services::ScriptEvent {
public:
    CameraBaseEvent( const std::string& eventName, Json&& json );
    Json toJson() const override;

private:
    Json m_json;
};

}  // namespace HubPlatform
