#pragma once
#include "uid/extensions/http_links/events/event_base.h"
#include "uid/extensions/http_links/connection_info.h"

namespace HubPlatform {
namespace HttpLinks {

class DataReceivedEvent : public EventBase {
public:
    using Headers = ConnectionInfo::Headers;
    using Params = ConnectionInfo::Params;

public:
    DataReceivedEvent() = default;
    DataReceivedEvent( std::string id,
                       std::string linkId,
                       Json userData,
                       Headers headers,
                       Params params,
                       std::string binaryData,
                       bool last,
                       std::string method );

    Json toJson() const override;
    Json toJsonFull() const override;
    const char* name() const override;

    void headers( Headers headers );
    const Headers& headers() const;
    void params( Params params );
    const Params& params() const;
    void binaryData( const char* ptr, unsigned length );
    const std::string& binaryData() const;
    void last( bool value );
    bool last() const;
    const std::string& method() const;
    void method( std::string value );

private:
    Headers m_headers;
    Params m_params;
    std::string m_binaryData;
    bool m_last{ true };
    std::string m_method;
};

}  // namespace HttpLinks
}  // namespace HubPlatform