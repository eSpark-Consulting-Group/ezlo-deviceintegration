#include "cameras/input_output/camera_archiver.h"
#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_output.h"
#include "cameras/packet/packet_publisher.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {

CameraArchiver::CameraArchiver( StorageSharedPtr storage,
                                int prerecordedDurationSec,
                                int segmentDurationSec,
                                int segmentsLimit,
                                const std::string& format,
                                PacketHandlerSharedPtr packetHandler,
                                std::string outputFile )
: m_packetHandler( std::move( packetHandler ) )
, m_cameraOutput( std::make_unique< CameraOutput >( std::move( storage ),
                                                    std::move( outputFile ),
                                                    std::chrono::seconds( prerecordedDurationSec ),
                                                    std::chrono::seconds( segmentDurationSec ),
                                                    segmentsLimit,
                                                    format ) ) {}

CameraArchiver::~CameraArchiver() {
    stop();
    release();
}

std::future< int > CameraArchiver::init( const ICameraInput& cameraInput ) {
    if( auto video = cameraInput.getVideoStream() ) {
        auto outputResolution = Resolution::fromStream( *video );
        m_cameraOutput->addVideoStream( video->index, *video, outputResolution );
    }
    if( auto audio = cameraInput.getAudioStream() ) {
        m_cameraOutput->addAudioStream( audio->index, *audio );
    }

    m_listenerId = m_packetHandler->addListener(
        [this]( const AVPacketPrototype& packet ) { m_cameraOutput->receivePacket( packet ); } );

    return m_cameraOutput->allowRecord();
}

void CameraArchiver::start( std::chrono::seconds maxDurationSec ) {
    m_cameraOutput->start( maxDurationSec );
}

void CameraArchiver::stop() {
    m_cameraOutput->stop();
}

StorageSharedPtr CameraArchiver::getStorage() const {
    return m_cameraOutput->getStorage();
}

int CameraArchiver::getPrerecordedDuration() const {
    return m_cameraOutput->getPrerecordedDuration();
}

int CameraArchiver::getSegmentDuration() const {
    return m_cameraOutput->getSegmentDuration();
}

int CameraArchiver::getSegmentsLimit() const {
    return m_cameraOutput->getSegmentsLimit();
}

void CameraArchiver::release() {
    m_packetHandler->removeListener( m_listenerId );
}
}  // namespace HubPlatform
