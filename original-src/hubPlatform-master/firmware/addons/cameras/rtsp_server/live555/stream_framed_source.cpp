#include "cameras/rtsp_server/live555/stream_framed_source.h"
#include "cameras/rtsp_server/camera_framed_source_options.h"
#include "cameras/packet/packet_publisher.h"
#include "log_utils/ezlo_log.h"

#include <tiny/tiny_consolecolors.h>

#include <utility>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

void StreamFramedSource::deliverFrame0( void* clientData ) {
    reinterpret_cast< StreamFramedSource* >( clientData )->deliverFrame();
}

StreamFramedSource* StreamFramedSource::createNew( UsageEnvironment& env,
                                                   StreamPacketPassthrough& packetsPassthrough,
                                                   std::unique_ptr< IPacketBuffer > packetBuffer,
                                                   std::string streamId,
                                                   int streamIndex,
                                                   bool isVideoStream ) {
    return new StreamFramedSource(
        env, packetsPassthrough, std::move( packetBuffer ), streamId, streamIndex, isVideoStream );
}

void StreamFramedSource::receivePacket( const Packet& packet ) {
    // @note: called from external thread.

    if( m_packetBuffer->push( packet ) ) {
        notifyAboutNewFrame();
    }
    else {
        Log::warn( cc::warn( "Queue size limit exceeded, dropping packet, index " ) +
                   cc::note( std::to_string( packet.streamIndex ) ) );
    }
}

StreamFramedSource::StreamFramedSource( UsageEnvironment& env,
                                        StreamPacketPassthrough& packetsPassthrough,
                                        std::unique_ptr< IPacketBuffer > packetBuffer,
                                        std::string streamId,
                                        int streamIndex,
                                        bool isVideoStream )
: FramedSource( env )
, m_newFrameTriggerId( env.taskScheduler().createEventTrigger( deliverFrame0 ) )
, m_packetBuffer( std::move( packetBuffer ) )
, m_packetsPassthrough( packetsPassthrough )
, m_streamId( std::move( streamId ) )
, m_streamIndex( streamIndex )
, m_isVideoStream( isVideoStream )
, m_everReceivedKey( false )
, m_everNotified( false ) {
    if( m_newFrameTriggerId ) {
        m_listenerId = m_packetsPassthrough.addListener( [this]( const Packet& packet ) {
            if( packet.streamIndex != m_streamIndex ) {
                return;
            }
            if( m_isVideoStream ) {
                if( packet.isPreCached ) {
                    if( m_packetBuffer->push( packet ) ) {
                        if( !m_everNotified ) {
                            m_everNotified = true;
                            notifyAboutNewFrame();
                        }
                    }
                    else {
                        Log::warn( cc::warn( "Queue size limit exceeded while faking, dropping packet, index " ) +
                                   cc::note( std::to_string( packet.streamIndex ) ) );
                    }
                }
                else {
                    if( packet.isKey ) {
                        if( !m_everReceivedKey ) {
                            m_packetBuffer->clear();
                            m_everReceivedKey = true;
                        }
                    }
                    receivePacket( packet );
                }
            }
            else {  // audio packet
                if( !packet.isPreCached ) {
                    receivePacket( packet );
                }
            }
        } );
        Log::info( cc::info( "Created stream source for " ) +
                   cc::note( m_streamId + "_" + std::to_string( m_streamIndex ) ) );
        LOGGER_TRACE_RAW( cc::trace( "Add framed source listener: " ) + cc::num10( m_listenerId ) );
    }
    else {
        Log::error( cc::error( "Failed to create event trigger" ) );
    }
}

StreamFramedSource::~StreamFramedSource() {
    Log::info( cc::info( "Deleted stream source for " ) +
               cc::note( m_streamId + "_" + std::to_string( m_streamIndex ) ) );
    Log::trace( cc::trace( "Remove framed source listener: " ) + cc::num10( m_listenerId ) );
    if( m_listenerId != -1 ) {
        m_packetsPassthrough.removeListener( m_listenerId );
        m_listenerId = -1;
    }

    if( m_newFrameTriggerId ) {
        envir().taskScheduler().deleteEventTrigger( m_newFrameTriggerId );
        m_newFrameTriggerId = 0;
    }
}

void StreamFramedSource::doGetNextFrame() {
    if( !m_isVideoStream || m_everReceivedKey ) {
        deliverFrame();
    }
}

void StreamFramedSource::deliverFrame() {
    if( !isCurrentlyAwaitingData() ) {
        return;
    }

    // Check if new frame is available.
    Packet packet;
    if( !m_packetBuffer->pop( packet ) ) {
        return;
    }

    uint8_t* newFrameDataStart = packet.data.data();
    unsigned newFrameSize = packet.data.size();

    // Skip H264/H265 start code.
    if( newFrameSize > 4 && newFrameDataStart[ 0 ] == 0 && newFrameDataStart[ 1 ] == 0 ) {
        if( newFrameDataStart[ 2 ] == 1 ) {  // H265
            newFrameDataStart += 3;
            newFrameSize -= 3;
        }
        else if( newFrameDataStart[ 2 ] == 0 && newFrameDataStart[ 3 ] == 1 ) {  // H264
            newFrameDataStart += 4;
            newFrameSize -= 4;
        }
    }

    if( newFrameSize > fMaxSize ) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = newFrameSize - fMaxSize;
    }
    else {
        fFrameSize = newFrameSize;
    }

    // If you have a more accurate time - e.g., from an encoder - then use that instead.
    // If the device is *not* a 'live source' (e.g., it comes instead from a file or buffer), then set
    // "fDurationInMicroseconds" here.
    gettimeofday( &fPresentationTime, nullptr );

    memcpy( fTo, newFrameDataStart, fFrameSize );
    FramedSource::afterGetting( this );
}

void StreamFramedSource::notifyAboutNewFrame() {
    envir().taskScheduler().triggerEvent( m_newFrameTriggerId, this /*client data*/ );
}

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
