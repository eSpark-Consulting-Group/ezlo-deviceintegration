#include "http_bridge/isession_manager.h"

namespace HubPlatform {
namespace HttpBridge {

class SessionManager : public ISessionManager {
public:
    SessionManager() = default;
    SessionPtr startSession( lws* wsi, const char* sessionType ) override;
    void finishSession( lws* wsi ) override;
    SessionPtr findSession( lws* wsi ) const override;
    void forEachSession( const std::string& msg,
                         std::function< void( const std::string&, SessionPtr ptr ) > callback ) override;

    size_t sessionCount() const override;
    size_t eventStreamsCount() const override;
    void incrementExternalSessions() override;
    void decrementExternalSessions() override;
    size_t externalSessions() const override;

private:
    mutable std::mutex m_mutex;
    using Sessions = std::map< lws*, SessionPtr >;
    Sessions m_sessions;
    int m_externalSessions{ 0 };
    int m_eventStreams{ 0 };
};

}  // namespace HttpBridge
}  // namespace HubPlatform