#pragma once
#include "uid/extensions/http_links/events/event_base.h"

namespace HubPlatform {
namespace HttpLinks {

class LinkUpdatedEvent : public EventBase {
public:
    LinkUpdatedEvent() = default;
    LinkUpdatedEvent( std::string linkId, Json userData, std::string url, std::string secureUrl );
    Json toJson() const override;
    const char* name() const override;

private:
    std::string m_url;
    std::string m_secureUrl;
};

}  // namespace HttpLinks
}  // namespace HubPlatform