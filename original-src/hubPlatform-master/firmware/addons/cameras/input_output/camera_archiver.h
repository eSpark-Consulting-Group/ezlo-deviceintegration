#pragma once

#include "cameras/defs.h"
#include "cameras/input_output/camera_input.h"
#include <string>
#include <memory>
#include <future>
#include <chrono>

namespace HubPlatform {

namespace UT {
class CameraArchiverTestSuite;
}

class PacketHandler;
class CameraInput;
class CameraOutput;

class CameraArchiver {
public:
    CameraArchiver( StorageSharedPtr storage,
                    int prerecordedDurationSec,
                    int segmentDurationSec,
                    int segmentsLimit,
                    const std::string& format,
                    PacketHandlerSharedPtr packetHandler,
                    std::string outputFile );
    CameraArchiver( const CameraArchiver& ) = delete;
    CameraArchiver( CameraArchiver&& ) = delete;
    ~CameraArchiver();

    std::future< int > init( const ICameraInput& cameraInput );
    void start( std::chrono::seconds maxDurationSec = std::chrono::seconds( 0 ) );

    StorageSharedPtr getStorage() const;
    int getPrerecordedDuration() const;
    int getSegmentDuration() const;
    int getSegmentsLimit() const;

    void stop();

private:
    void release();

private:
    PacketHandlerSharedPtr m_packetHandler;
    int64_t m_listenerId = -1;
    CameraOutputUniquePtr m_cameraOutput;

    friend class UT::CameraArchiverTestSuite;
};
}  // namespace HubPlatform
