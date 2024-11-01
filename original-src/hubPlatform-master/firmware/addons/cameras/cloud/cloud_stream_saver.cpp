#include "cameras/cloud/cloud_stream_saver.h"

#include "cameras/input_output/camera_input_interface.h"
#include "cameras/input_output/camera_archiver.h"
#include "cameras/storages/cloud_storage.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {

CloudStreamSaver::CloudStreamSaver( ICameraInput& cameraInput,
                                    CloudStreamSaver::StreamId streamId,
                                    CloudStreamSaver::VideoPath videoPath )
: m_cameraInput( cameraInput )
, m_videoPath( std::move( videoPath ) )
, m_streamId( std::move( streamId ) ) {}

bool CloudStreamSaver::isSavingCreated() const {
    return m_cameraArchiver != nullptr;
}

bool CloudStreamSaver::isStarted() const {
    return m_isStarted;
}

void CloudStreamSaver::createRecording( int segmentDurationSec,
                                        int64_t secondsBeforeStart,
                                        int segmentsLimit,
                                        const std::string& format,
                                        SaveFileCallback saveFile ) {
    auto storage = std::make_shared< CloudStorage >( m_videoPath );
    storage->setSaveFileCallback( saveFile );
    m_cameraArchiver = std::make_unique< CameraArchiver >( storage,
                                                           secondsBeforeStart,
                                                           segmentDurationSec,
                                                           segmentsLimit,
                                                           format,
                                                           m_cameraInput.getPacketHandler(),
                                                           m_streamId );
    m_cameraArchiver->init( m_cameraInput );
    m_isStarted = false;
}

void CloudStreamSaver::deleteRecording() {
    m_cameraArchiver.reset();
    m_isStarted = false;
}

void CloudStreamSaver::startRecording( const Json& metaData, SaveFileCallback saveFile ) {
    auto& storage = dynamic_cast< CloudStorage& >( *m_cameraArchiver->getStorage() );
    storage.setSaveFileCallback( std::move( saveFile ) );
    storage.setMetaData( metaData );

    startRecording();
}

void CloudStreamSaver::startRecording() {
    m_cameraArchiver->start();
    m_isStarted = true;
}

void CloudStreamSaver::stopRecording() {
    if( !m_isStarted ) {
        return;
    }
    m_cameraArchiver->stop();
    m_isStarted = false;
}

}  // namespace HubPlatform
