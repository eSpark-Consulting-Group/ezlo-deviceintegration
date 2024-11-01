#pragma once

#include "liveMedia/ServerMediaSession.hh"

#include <string>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

class ServerMediaSessionImpl : public ServerMediaSession {
public:
    static ServerMediaSessionImpl* createNew( UsageEnvironment& env,
                                              char const* streamName = NULL,
                                              char const* info = NULL,
                                              char const* description = NULL,
                                              Boolean isSSM = False,
                                              char const* miscSDPLines = NULL );

protected:
    ServerMediaSessionImpl( UsageEnvironment& env,
                            char const* streamName = NULL,
                            char const* info = NULL,
                            char const* description = NULL,
                            Boolean isSSM = False,
                            char const* miscSDPLines = NULL );
    ~ServerMediaSessionImpl() override;

private:
    std::string m_streamId;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
