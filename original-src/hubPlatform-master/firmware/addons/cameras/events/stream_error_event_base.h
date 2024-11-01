#pragma once
#include "service_interfaces/script_event.h"
#include <string>

namespace HubPlatform {

class StreamErrorEventBase : public Services::ScriptEvent {
public:
    Json toJson() const override;

protected:
    StreamErrorEventBase( std::string eventName, std::string errorDescription );

protected:
    const std::string m_errorDescription;
};

}  // namespace HubPlatform
