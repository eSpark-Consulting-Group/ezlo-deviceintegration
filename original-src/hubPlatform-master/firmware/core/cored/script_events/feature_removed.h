#pragma once

#include "cored/script_events/base_event.h"

namespace HubPlatform {
namespace CoreD {
namespace ScriptEvents {

class FeatureRemoved : public BaseEvent {
public:
    FeatureRemoved( const Json& json, std::string name );

    Json toJson() const override;

private:
    std::string m_name;
};

}  // namespace ScriptEvents
}  // namespace CoreD
}  // namespace HubPlatform
