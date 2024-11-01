#pragma once

#include "json_utils/json_fwd.h"
#include "service_interfaces/resources.h"

#include <string>

namespace HubPlatform {
namespace Services {

class ScriptEvent {
public:
    ScriptEvent( std::string eventName, const std::string& moduleName = Resources::DefaultModuleName );
    virtual ~ScriptEvent();

    virtual Json toJson() const;

    virtual const std::string& getName() const;
    virtual const std::string& getModuleName() const;

    bool operator==( const ScriptEvent& rhs ) const;
    bool operator!=( const ScriptEvent& rhs ) const;

private:
    std::string m_eventName;
    std::string m_moduleName;
};

}  // namespace Services
}  // namespace HubPlatform
