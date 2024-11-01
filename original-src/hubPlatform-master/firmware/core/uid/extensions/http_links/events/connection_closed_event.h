#pragma once
#include "uid/extensions/http_links/events/event_base.h"
#include "uid/extensions/http_links/connection_closed_reason.h"

namespace HubPlatform {
namespace HttpLinks {

class ConnectionClosedEvent : public EventBase {
public:
    ConnectionClosedEvent() = default;
    ConnectionClosedEvent( std::string id,
                           std::string linkId,
                           Json userData,
                           ConnectionClosedReason code,
                           std::string reason );

    void reason( ConnectionClosedReason code, std::string message );

    Json toJson() const override;
    const char* name() const override;

private:
    ConnectionClosedReason m_reasonCode{ ConnectionClosedReason::OK };
    std::string m_reasonText;
};

}  // namespace HttpLinks
}  // namespace HubPlatform