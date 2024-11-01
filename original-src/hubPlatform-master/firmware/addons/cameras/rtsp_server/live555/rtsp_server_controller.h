#pragma once

#include "cameras/rtsp_server/rtsp_server_controller_interface.h"
#include "cameras/events/event_publisher_interface.h"
#include "cameras/rtsp_server/live555/rtsp_stream_context.h"
#include "cameras/rtsp_server/live555/event_trigger.h"

#include "cameras/codec/codec_interface.h"

#include "UsageEnvironment/UsageEnvironment.hh"

#include <thread>
#include <list>
#include <condition_variable>
#include <future>
#include <atomic>

class TaskScheduler;
class UsageEnvironment;
class RTSPServer;

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

/// \brief useful links:
/// https://live-devel.live555.narkive.com/to6ZEBDt/adding-streams-to-already-running-event-loop
/// https://github.com/rgaufman/live555/testProgs

class RtspServerController final : public IRtspServerController {
public:
    using Codec = std::unique_ptr< ICodec >;
    using Codecs = std::vector< Codec >;
    using UsageEnvironmentUniquePtr = std::unique_ptr< UsageEnvironment, std::function< void( UsageEnvironment* ) > >;
    using RTSPServerUniquePtr = std::unique_ptr< RTSPServer, std::function< void( RTSPServer* ) > >;
    using RtspStreamContexts = std::map< StreamId, RtspStreamContext >;

    explicit RtspServerController( uint16_t serverListenPort,
                                   size_t packetCountLimit,
                                   IEventPublisher< EventBase >& eventPublisher );
    RtspServerController( const RtspServerController& ) = delete;
    RtspServerController( RtspServerController&& ) = delete;
    ~RtspServerController() override;

    void createCameraStream( IPacketPublisher& packetHandler,
                             const std::string& streamId,
                             Codecs&& codecs,
                             const std::string& sdp ) override;
    void deleteCameraStream( const StreamId& streamId ) override;
    void stop() override;
    void start() override;
    bool isStarted() const override;
    bool waitForStart( std::chrono::seconds timeout ) override;
    std::vector< std::string > getStreamIds() const override;
    size_t getStreamQuantity() const override;
    Status getStatus() const override;

private:
    static void createCameraStreamEvent( void* clientData );
    static void deleteCameraStreamEvent( void* clientData );

    void triggerCameraStreamCreating();
    void triggerCameraStreamDeleting();

    void deleteStream( const StreamId& streamId );
    void deleteCameraAllStream();

private:
    IEventPublisher< EventBase >& m_eventPublisher;
    size_t m_packetCountLimit;
    std::unique_ptr< TaskScheduler > m_taskScheduler;
    UsageEnvironmentUniquePtr m_env;
    RTSPServerUniquePtr m_server;
    EventTrigger m_createStreamTrigger;
    EventTrigger m_deleteStreamTrigger;
    RtspStreamContexts m_contexts;
    std::list< RtspStreamContexts::iterator > m_startContextQueue;
    std::list< RtspStreamContexts::iterator > m_stopContextQueue;
    volatile char m_watch = 0;
    std::thread m_worker;
    mutable std::mutex m_mutex;
    std::condition_variable m_deleteAllStream;
    std::atomic_bool m_deleteAllStreamFlag;
    std::shared_ptr< std::promise< bool > > m_startPromise;
    std::atomic< Status > m_status;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
