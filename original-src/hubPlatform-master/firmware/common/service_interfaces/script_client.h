#pragma once

#include "bus/messagesclasses.h"
#include "json_utils/json_fwd.h"
#include "service_interfaces/service.h"

enum MessageSubClasses : int;

namespace HubPlatform {

class PluginEventsSubscriber;

namespace Services {
class Service;
}

class ScriptClient {
public:
    explicit ScriptClient( std::string remoteAddonQualifiedName, Services::Service& host );
    virtual ~ScriptClient();

protected:
    Services::Service& getHost() const;
    void sendMethodCall( Json data ) const;
    Json sendMethodCall( MessageSubClasses messageSubClasses, Json&& data ) const;
    Json sendRequest( MessageSubClasses messageSubClasses, Json&& data ) const;

private:
    Services::Service& m_host;
    const std::string m_remoteAddonQualifiedName;
};

}  // namespace HubPlatform
