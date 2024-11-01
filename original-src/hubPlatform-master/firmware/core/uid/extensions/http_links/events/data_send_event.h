#pragma once
#include "uid/extensions/http_links/events/event_base.h"

namespace HubPlatform {
namespace HttpLinks {

class DataSendEvent : public EventBase {
public:
    DataSendEvent() = default;
    DataSendEvent( std::string id, std::string linkId, Json userData, int maxDataLength );

    Json toJson() const override;
    const char* name() const override;
    void maxDataLength( bool value );
    bool maxDataLength() const;

private:
    int m_maxDataLength{ 500000 };
};

}  // namespace HttpLinks
}  // namespace HubPlatform