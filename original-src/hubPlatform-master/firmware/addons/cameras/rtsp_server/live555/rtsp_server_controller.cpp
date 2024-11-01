#include "cameras/rtsp_server/live555/rtsp_server_controller.h"
#include "cameras/rtsp_server/live555/stream_media_subsession.h"
#include "cameras/rtsp_server/live555/server_media_session.h"
#include "cameras/rtsp_server/live555/stream_packet_passthrough.h"
#include "cameras/rtsp_server/rtsp_server_exception.h"
#include "cameras/packet/packet.h"

#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"

#include "BasicUsageEnvironment.hh"
#include "liveMedia/RTSPServer.hh"

#include <tiny/tiny_consolecolors.h>

#include <cassert>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

RtspServerController::RtspServerController( uint16_t serverListenPort,
                                            size_t packetCountLimit,
                                            IEventPublisher< EventBase >& eventPublisher )
: m_eventPublisher( eventPublisher )
, m_packetCountLimit( packetCountLimit )
, m_status( Status::Stopped ) {
    m_taskScheduler = std::unique_ptr< TaskScheduler >( BasicTaskScheduler::createNew() );
    if( !m_taskScheduler ) {
        throw RtspServerException( "BasicTaskScheduler::createNew() failed" );
    }

    m_env =
        UsageEnvironmentUniquePtr( BasicUsageEnvironment::createNew( *m_taskScheduler ), []( UsageEnvironment* ptr ) {
            assert( ptr );
            ptr->reclaim();
        } );

    if( !m_env ) {
        throw RtspServerException( "BasicUsageEnvironment::createNew() failed" );
    }

    m_server = RTSPServerUniquePtr( RTSPServer::createNew( *m_env, serverListenPort, nullptr /*auth*/ ),
                                    []( RTSPServer* ptr ) {
                                        assert( ptr );
                                        Medium::close( ptr );
                                    } );

    if( !m_server ) {
        throw RtspServerException( "Cannot start RTSP server on port " + std::to_string( serverListenPort ) );
    }

    m_createStreamTrigger = EventTrigger( m_env.get(), createCameraStreamEvent );
    m_deleteStreamTrigger = EventTrigger( m_env.get(), deleteCameraStreamEvent );

    // Increasing live555 output buffer.
    OutPacketBuffer::increaseMaxSizeTo( 1000000 );
}

RtspServerController::~RtspServerController() {
    stop();
}

void RtspServerController::createCameraStream( IPacketPublisher& packetPublisher,
                                               const std::string& streamId,
                                               Codecs&& codecs,
                                               const std::string& sdp ) {
    Log::info( cc::info( "Create live RTSP stream " ) + cc::note( streamId ) );
    std::lock_guard< std::mutex > lock( m_mutex );
    auto context =
        RtspStreamContext( streamId, packetPublisher, m_eventPublisher, std::move( codecs ), sdp, m_packetCountLimit );
    auto result = m_contexts.emplace( streamId, std::move( context ) );
    if( !result.second ) {
        Log::warn( cc::warn( "Live RTSP stream is already created: " ) + cc::note( streamId ) );
        return;
    }
    m_startContextQueue.push_back( result.first );
    if( !m_startContextQueue.empty() ) {
        triggerCameraStreamCreating();
    }
}

void RtspServerController::deleteCameraStream( const StreamId& streamId ) {
    Log::info( cc::info( "Delete live RTSP stream " ) + cc::note( streamId ) );
    std::lock_guard< std::mutex > lock( m_mutex );
    deleteStream( streamId );
}

void RtspServerController::stop() {
    deleteCameraAllStream();
    m_watch = 1;
    if( m_worker.joinable() ) {
        m_worker.join();
    }
    m_status = Status::Stopped;
}

void RtspServerController::start() {
    if( m_status != Status::Stopped ) {
        return;
    }
    m_watch = 0;
    m_status = Status::WaitingForStart;
    m_startPromise = std::make_shared< std::promise< bool > >();
    m_worker = std::thread( [this] {
        m_startPromise->set_value( true );
        m_taskScheduler->doEventLoop( &m_watch );
        m_status = Status::Running;
    } );
}

bool RtspServerController::waitForStart( std::chrono::seconds timeout ) {
    if( m_status == Status::Running ) {
        return true;
    }
    auto future = m_startPromise->get_future();
    auto futureStatus = future.wait_for( timeout );
    return futureStatus == std::future_status::ready;
}

std::vector< std::string > RtspServerController::getStreamIds() const {
    std::lock_guard< std::mutex > lock( m_mutex );
    std::vector< std::string > streamIds;
    streamIds.reserve( m_contexts.size() );
    for( const auto& context : m_contexts ) {
        streamIds.push_back( context.first );
    };
    return streamIds;
}

size_t RtspServerController::getStreamQuantity() const {
    return m_contexts.size();
}

IRtspServerController::Status RtspServerController::getStatus() const {
    return m_status;
}

bool RtspServerController::isStarted() const {
    return m_startPromise.operator bool();
}

void RtspServerController::createCameraStreamEvent( void* clientData ) {
    auto* serverPtr = reinterpret_cast< RtspServerController* >( clientData );
    if( !serverPtr ) {
        LOGGER_ERROR( cc::error( "No server" ) );
        return;
    }

    std::lock_guard< std::mutex > lock( serverPtr->m_mutex );
    if( serverPtr->m_startContextQueue.empty() ) {
        LOGGER_WARN( cc::warn( "Empty context queue" ) );
        return;
    }
    RtspStreamContext& context = serverPtr->m_startContextQueue.front()->second;
    serverPtr->m_startContextQueue.pop_front();
    Log::debug( cc::debug( "Creating live RTSP stream event " ) + cc::note( context.streamId ) );
    ServerMediaSession* sms = ServerMediaSessionImpl::createNew(
        *serverPtr->m_env, context.streamId.c_str(), nullptr /*info*/, nullptr /*description*/, True /*SSM*/ );

    auto streamPacketsPassthrough = std::make_shared< StreamPacketPassthrough >( context.packetPublisher );
    for( auto& codec : context.codecs ) {
        auto streamIndex = codec->getStreamIndex();
        auto mediaSubsession = StreamMediaSubsession::createNew( context.streamId,
                                                                 *serverPtr->m_env,
                                                                 std::move( codec ),
                                                                 streamPacketsPassthrough,
                                                                 streamIndex,
                                                                 context.sdp,
                                                                 context.packetCountLimit );
        sms->addSubsession( mediaSubsession );
    }

    serverPtr->m_server->addServerMediaSession( sms );

    std::unique_ptr< char[] > streamURLChar = std::unique_ptr< char[] >( serverPtr->m_server->rtspURL( sms ) );
    context.eventPublisher.sendEvent( EventBase( context.streamId, EventType::STREAM_CREATED, streamURLChar.get() ) );

    Log::info( cc::info( "Created live RTSP stream " ) + cc::note( context.streamId ) );

    if( !serverPtr->m_startContextQueue.empty() ) {
        serverPtr->triggerCameraStreamCreating();
    }
}

void RtspServerController::deleteCameraStreamEvent( void* clientData ) {
    auto* serverPtr = reinterpret_cast< RtspServerController* >( clientData );
    if( !serverPtr ) {
        LOGGER_ERROR( cc::error( "No server" ) );
        return;
    }
    std::unique_lock< std::mutex > lock( serverPtr->m_mutex );
    if( serverPtr->m_stopContextQueue.empty() ) {
        LOGGER_WARN( cc::warn( "Empty context queue" ) );
        return;
    }
    auto it = serverPtr->m_stopContextQueue.front();
    RtspStreamContext& context = serverPtr->m_stopContextQueue.front()->second;
    serverPtr->m_stopContextQueue.pop_front();

    LOGGER_DEBUG_RAW( cc::debug( "Deleting live RTSP stream event " ) + cc::note( context.streamId ) );

    serverPtr->m_server->deleteServerMediaSession( context.streamId.c_str() );

    context.eventPublisher.sendEvent( EventBase( context.streamId, EventType::STREAM_DELETED ) );

    Log::info( cc::info( "Deleted live RTSP stream event " ) + cc::note( context.streamId ) );

    serverPtr->m_contexts.erase( it );

    if( !serverPtr->m_stopContextQueue.empty() ) {
        serverPtr->triggerCameraStreamDeleting();
    }
    lock.unlock();
    serverPtr->m_deleteAllStreamFlag = true;
    serverPtr->m_deleteAllStream.notify_one();
}

void RtspServerController::triggerCameraStreamCreating() {
    m_env->taskScheduler().triggerEvent( m_createStreamTrigger.getId(), this );
}

void RtspServerController::triggerCameraStreamDeleting() {
    m_env->taskScheduler().triggerEvent( m_deleteStreamTrigger.getId(), this );
}

void RtspServerController::deleteStream( const StreamId& streamId ) {
    auto contextIt = m_contexts.find( streamId );
    if( contextIt == m_contexts.end() ) {
        auto it = std::find( m_startContextQueue.begin(), m_startContextQueue.end(), contextIt );
        if( it == m_startContextQueue.end() ) {
            Log::warn( cc::warn( "Stream is not created: " ) + cc::note( streamId ) );
            return;
        }
    }
    {  // if deleting request is in queue then return
        auto it = std::find( m_stopContextQueue.begin(), m_stopContextQueue.end(), contextIt );
        if( it != m_stopContextQueue.end() ) {
            return;
        }
    }
    m_stopContextQueue.push_back( contextIt );
    if( m_stopContextQueue.size() == 1 ) {
        triggerCameraStreamDeleting();
    }
}

void RtspServerController::deleteCameraAllStream() {
    std::unique_lock< std::mutex > lock( m_mutex );
    std::vector< std::string > streamIds;
    for( auto& context : m_contexts ) {
        streamIds.push_back( context.first );
    }
    for( auto& streamId : streamIds ) {
        Log::debug( cc::debug( "Delete live RTSP stream " ) + cc::note( streamId ) );
        deleteStream( streamId );
        m_deleteAllStreamFlag = false;
        if( !m_deleteAllStream.wait_for(
                lock, std::chrono::seconds( 5 ), [this]() { return m_deleteAllStreamFlag.load(); } ) ) {
            Log::warn( cc::warn( "Stream wasn't deleted: " ) + cc::note( streamId ) );
        }
    }
}

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
