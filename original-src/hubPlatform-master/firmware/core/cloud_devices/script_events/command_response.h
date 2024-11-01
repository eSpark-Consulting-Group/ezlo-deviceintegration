#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace CloudDevices {

class CommandResponse : public Services::ScriptEvent {
public:
    explicit CommandResponse( std::string commandId, Json result, Json error );

    Json toJson() const override;

private:
    std::string m_commandId;
    Json m_result;
    Json m_error;
};

}  // namespace CloudDevices
}  // namespace HubPlatform
