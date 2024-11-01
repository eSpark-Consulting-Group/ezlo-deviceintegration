#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class FeatureAdded : public BaseEvent {
public:
    FeatureAdded( const Json& json, std::string name, std::string status );

    Json toJson() const override;

private:
    std::string m_name;
    std::string m_status;
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
