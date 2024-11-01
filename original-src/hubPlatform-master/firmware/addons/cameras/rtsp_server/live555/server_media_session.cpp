#include "cameras/rtsp_server/live555/server_media_session.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

ServerMediaSessionImpl* ServerMediaSessionImpl::createNew( UsageEnvironment& env,
                                                           const char* streamName,
                                                           const char* info,
                                                           const char* description,
                                                           Boolean isSSM,
                                                           const char* miscSDPLines ) {
    return new ServerMediaSessionImpl( env, streamName, info, description, isSSM, miscSDPLines );
}

ServerMediaSessionImpl::ServerMediaSessionImpl( UsageEnvironment& env,
                                                const char* streamName,
                                                const char* info,
                                                const char* description,
                                                Boolean isSSM,
                                                const char* miscSDPLines )
: ServerMediaSession( env, streamName, info, description, isSSM, miscSDPLines )
, m_streamId( streamName ) {
    LOGGER_TRACE_RAW( cc::trace( "Created server media session: " ) + cc::note( m_streamId ) );
}

ServerMediaSessionImpl::~ServerMediaSessionImpl() {
    LOGGER_TRACE_RAW( cc::trace( "Deleted server media session: " ) + cc::note( m_streamId ) );
}

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
