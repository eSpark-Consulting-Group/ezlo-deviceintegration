#pragma once

#include "cameras/packet/packet_buffer_interface.h"
#include "cameras/packet/packet_publisher_interface.h"
#include "cameras/rtsp_server/live555/stream_packet_passthrough.h"

#include "liveMedia/FramedSource.hh"

#include <memory>

namespace HubPlatform {
namespace Cameras {
namespace Live555 {

namespace UT {
class TestStreamFramedSourceDeleter;
}  // namespace UT

class StreamFramedSource : public FramedSource {
public:
    static StreamFramedSource* createNew( UsageEnvironment& env,
                                          StreamPacketPassthrough& packetsPassthrough,
                                          std::unique_ptr< IPacketBuffer > packetBuffer,
                                          std::string streamId,
                                          int streamIndex,
                                          bool isVideoStream );

private:
    static void deliverFrame0( void* clientData );
    StreamFramedSource( UsageEnvironment& env,
                        StreamPacketPassthrough& packetsPassthrough,
                        std::unique_ptr< IPacketBuffer > packetBuffer,
                        std::string streamId,
                        int streamIndex,
                        bool isVideoStream );
    ~StreamFramedSource() override;

    void doGetNextFrame() override;
    void deliverFrame();
    void notifyAboutNewFrame();

    void receivePacket( const Packet& packet );

private:
    EventTriggerId m_newFrameTriggerId;
    std::unique_ptr< IPacketBuffer > m_packetBuffer;
    StreamPacketPassthrough& m_packetsPassthrough;
    StreamPacketPassthrough::ListenerId m_listenerId = -1;
    std::string m_streamId;
    const int m_streamIndex;
    const bool m_isVideoStream;
    bool m_everReceivedKey;
    bool m_everNotified;

    friend class UT::TestStreamFramedSourceDeleter;
};

}  // namespace Live555
}  // namespace Cameras
}  // namespace HubPlatform
