#include "cameras/cloud/stream_particle_recorder.h"
#include "cameras/input_output/camera_input_interface.h"
#include "cameras/input_output/camera_output.h"
#include "cameras/storages/cloud_storage.h"

#include "log_utils/ezlo_log.h"
#include "common_utils/cpp14.h"
#include <tiny/tiny_consolecolors.h>

namespace HubPlatform {

StreamParticleRecorder::StreamParticleRecorder( ICameraInput& cameraInput, StreamId streamId, RecordsDir recordsDir )
: m_cameraInput( cameraInput )
, m_recordsDir( std::move( recordsDir ) )
, m_streamId( std::move( streamId ) )
, m_listenerId( PacketHandler::UnregisteredListenerId )
, m_recordDuration( 0 ) {}

StreamParticleRecorder::~StreamParticleRecorder() {
    if( m_cameraOutput ) {
        m_cameraOutput->stop();
    }

    if( m_listenerId != PacketHandler::UnregisteredListenerId ) {
        const PacketHandlerSharedPtr& packetHandler = m_cameraInput.getPacketHandler();
        if( packetHandler ) {
            packetHandler->removeListener( m_listenerId );
        }
    }
}

void StreamParticleRecorder::createRecording( const StreamParticleSettings& settings, SaveFileCallback saveFile ) {
    m_recordDuration = settings.preRecordDuration + settings.recordDuration;

    auto storage = std::make_shared< CloudStorage >( m_recordsDir );
    storage->setSaveFileCallback( std::move( saveFile ) );
    m_cameraOutput = createCameraOutput( std::move( storage ), settings );

    if( m_listenerId == PacketHandler::UnregisteredListenerId ) {
        const PacketHandlerSharedPtr& packetHandler = m_cameraInput.getPacketHandler();
        if( packetHandler ) {
            m_listenerId = packetHandler->addListener(
                [this]( const AVPacketPrototype& packet ) { m_cameraOutput->receivePacket( packet ); } );
        }
    }

    m_cameraOutput->allowRecord();
}

void StreamParticleRecorder::makeRecord( const Json& metaData ) {
    if( m_cameraOutput ) {
        auto& storage = dynamic_cast< CloudStorage& >( *m_cameraOutput->getStorage() );
        storage.setMetaData( metaData );

        makeRecord();
    }
}

void StreamParticleRecorder::makeRecord() {
    if( m_cameraOutput ) {
        m_cameraOutput->start( m_recordDuration );
    }
}

void StreamParticleRecorder::stopMakingRecord() {
    if( m_cameraOutput ) {
        m_cameraOutput->stop();
    }
}

bool StreamParticleRecorder::isRecordingCreated() const {
    return m_cameraOutput != nullptr;
}

bool StreamParticleRecorder::isMakingRecord() const {
    return m_cameraOutput && m_cameraOutput->isRecording();
}

void StreamParticleRecorder::deleteRecording() {
    LOGGER_DEBUG_RAW( cc::debug( "Delete particle recording" ) );
    if( m_listenerId != PacketHandler::UnregisteredListenerId ) {
        const PacketHandlerSharedPtr& packetHandler = m_cameraInput.getPacketHandler();
        if( packetHandler ) {
            packetHandler->removeListener( m_listenerId );
        }
        m_listenerId = PacketHandler::UnregisteredListenerId;
    }

    m_cameraOutput.reset();
}

CameraOutputUniquePtr StreamParticleRecorder::createCameraOutput( StorageSharedPtr storage,
                                                                  const StreamParticleSettings& settings ) const {
    const int segmentsLimit = 1;
    std::string fileNameStart = m_streamId + "_preview";
    auto cameraOutput = std::make_unique< CameraOutput >( std::move( storage ),
                                                          std::move( fileNameStart ),
                                                          settings.preRecordDuration,
                                                          m_recordDuration,
                                                          segmentsLimit,
                                                          settings.format );

    if( auto video = m_cameraInput.getVideoStream() ) {
        const Resolution outputResolution = determineOutputResolution( *video, settings.resolutionDivisor );
        cameraOutput->addVideoStream( video->index, *video, outputResolution );
    }
    if( auto audio = m_cameraInput.getAudioStream() ) {
        cameraOutput->addAudioStream( audio->index, *audio );
    }

    return cameraOutput;
}

Resolution StreamParticleRecorder::determineOutputResolution( const AVStream& sourceVideoStream, uint32_t divisor ) {
    const AVCodecParameters* codecParameters = sourceVideoStream.codecpar;
    return Resolution( codecParameters->width / static_cast< int >( divisor ),
                       codecParameters->height / static_cast< int >( divisor ) );
}

}  // namespace HubPlatform
