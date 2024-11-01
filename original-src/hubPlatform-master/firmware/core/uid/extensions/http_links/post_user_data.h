#pragma once
#include "uid/extensions/ui_http_session.h"
#include "uid/extensions/http_links/events.h"
#include "http_bridge/utils.h"
#include <atomic>
#include <string>
#include <vector>

namespace HubPlatform {
namespace HttpLinks {

struct PostUserData : public Uid::Extensions::HttpSession::UserData {
    std::vector< char > m_body;      // buffer to append data of a POST request to
    size_t m_bodyStreamedSize{ 0 };  //!< http body size sent, full size = bodyStreamedSize + body.size()
    int m_bodyPortions{ 0 };
    HttpBridge::Utils::KeyValueVector m_uriParams;
    std::string m_clientIp;
    uint64_t m_created{ 0 };
    time_t m_finished{ 0 };

    uint32_t m_timeout{ Resources::Limits::RESPONSE_TIMEOUT_DEFAULT };
    size_t m_position{ 0 };          // current position of written data to a lws output stream
    std::vector< char > m_response;  // serialized response

public:
    size_t leftInResponse() const {
        return m_response.size() - m_position;
    }
    void moveResponsePosition( size_t offset ) {
        m_position += offset;
    }
    void clearResponse() {
        m_position = 0;
        m_response.clear();
    }
    bool waitForData() const {
        return m_waitForData;
    }
    void waitForData( bool value ) {
        m_waitForData = value;
    }
    void closedReason( ConnectionClosedReason value, const char* reasonText = nullptr ) {
        m_closedReason = value;

        m_closedReasonText.clear();
        if( reasonText && *reasonText ) {
            m_closedReasonText = reasonText;
        }
    }
    ConnectionClosedReason reason() const {
        return m_closedReason;
    }
    std::string reasonText() const {
        return m_closedReasonText;
    }
    const std::string& pluginId() const {
        return m_pluginId;
    }
    void pluginId( std::string v ) {
        m_pluginId = std::move( v );
    }

    const std::string& linkId() const {
        return m_linkId;
    }
    void linkId( std::string v ) {
        m_linkId = std::move( v );
    }

    const std::string& connectionId() const {
        return m_connectionId;
    }
    void connectionId( std::string v ) {
        m_connectionId = std::move( v );
    }
    bool dataReceivedSent() const {
        return m_dataReceivedSent;
    }
    void dataReceivedSent( bool value ) {
        m_dataReceivedSent = value;
    }

private:
    std::string m_pluginId;
    std::string m_linkId;
    std::string m_connectionId;
    std::atomic_bool m_connectionClosed{ false };

    std::atomic_bool m_waitForData{ false };
    ConnectionClosedReason m_closedReason{ ConnectionClosedReason::NONE };
    std::string m_closedReasonText;
    bool m_dataReceivedSent{ false };
};

}  // namespace HttpLinks
}  // namespace HubPlatform