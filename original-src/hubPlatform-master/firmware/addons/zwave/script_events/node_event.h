#pragma once

#include "service_interfaces/script_event.h"

namespace HubPlatform {
namespace Zwave {

class NodeEvent : public Services::ScriptEvent {
public:
    NodeEvent( std::string eventName, uint8_t nodeId );

    Json toJson() const override;

protected:
    template < typename Func, typename... FuncArgs >
    void addOptionalStringField( Json& result,
                                 const char* const field,
                                 Func& func,
                                 const FuncArgs&... funcArgs ) const {
        if( func ) {
            if( auto name = func( funcArgs... ) ) {
                result[ field ] = name;
                return;
            }
        }
        result[ field ] = "unknown";
    }

private:
    uint8_t m_nodeId;
};

}  // namespace Zwave
}  // namespace HubPlatform
