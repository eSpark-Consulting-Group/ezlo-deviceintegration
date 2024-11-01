#include "cameras/tests/http_file_server.h"

#include "cameras/tests/samples.h"

#include "cameras/cloud/stream_particle_recorder.h"
#include "cameras/input_output/camera_input.h"
#include "cameras/input_output/camera_input_options.h"
#include "cameras/input_output/camera_output.h"
#include "cameras/input_output/camera_output_options.h"
#include "cameras/packet/packet_publisher.h"
#include "cameras/storages/local_storage.h"
#include "system_utils/filesystem.h"
#include "test_utils/temporary_folder_wrapper.h"

#include "common_utils/cpp14.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace HubPlatform {
namespace UT {

class StreamParticleRecorderTestSuite : public Test {
public:
    void SetUp() override {
        CameraInputOptions::setMaxDelayBetweenPacketsMsec( 100 );
        CameraOutputOptions::setPreRecordBufferSize( 1000 );
        CameraOutputOptions::setRecordBufferSize( 1000 );
        ILocalStorage::setMaxStorageVolume( 100 );

        std::string outputFolder = getTempFolder() + "/preview_test";
        filesystem::createFolder( outputFolder );

        streamCameraInput = createCameraInput( PathVideo20secondsMp4 );
        particleRecorder = std::make_unique< StreamParticleRecorder >( *streamCameraInput, "", outputFolder );
    }

    std::string getTempFolder() const {
        return m_folderWrapper.getPath();
    }

    static CameraInputUniquePtr createCameraInput( std::string url ) {
        PacketHandlerSharedPtr packetHandler = std::make_shared< PacketHandler >();
        return std::make_unique< CameraInput >( packetHandler, std::move( url ) );
    }

    static std::chrono::seconds getStreamDuration( const AVStream& stream ) {
        constexpr auto rounding = static_cast< AVRounding >( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX );
        const int durationSec = av_rescale_rnd( stream.duration, stream.time_base.num, stream.time_base.den, rounding );
        return std::chrono::seconds( durationSec );
    }

    static StreamParticleSettings createRecorderSettings() {
        StreamParticleSettings settings;
        settings.preRecordDuration = std::chrono::seconds( 2 );
        settings.recordDuration = std::chrono::seconds( 3 );
        settings.resolutionDivisor = 1;
        return settings;
    }

    CameraInputUniquePtr streamCameraInput;
    std::unique_ptr< StreamParticleRecorder > particleRecorder;

private:
    Tests::TemporaryFolderWrapper m_folderWrapper;
};

TEST_F( StreamParticleRecorderTestSuite, isRecordingCreatedFlagCheck ) {
    EXPECT_FALSE( particleRecorder->isRecordingCreated() );
    particleRecorder->createRecording( createRecorderSettings(), SaveFileCallback() );
    EXPECT_TRUE( particleRecorder->isRecordingCreated() );
}

TEST_F( StreamParticleRecorderTestSuite, recreationOfRecording ) {
    particleRecorder->createRecording( createRecorderSettings(), SaveFileCallback() );
    particleRecorder->deleteRecording();
    EXPECT_FALSE( particleRecorder->isRecordingCreated() );

    particleRecorder->createRecording( createRecorderSettings(), SaveFileCallback() );
    EXPECT_TRUE( particleRecorder->isRecordingCreated() );
}

}  // namespace UT
}  // namespace HubPlatform
