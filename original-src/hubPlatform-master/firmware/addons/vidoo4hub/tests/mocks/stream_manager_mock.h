#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "vidoo4hub/stream_manager/stream_manager.h"

namespace HubPlatform {
namespace UT {

class StreamManagerMock : public Vidoo4Hub::IStreamManager {
public:
    MOCK_METHOD3( addCameraStream,
                  void( const std::string& streamItemId, const std::string& streamName, const std::string& rtspURL ) );
    MOCK_METHOD1( removeCameraStream, void( const std::string& streamItemId ) );
    MOCK_METHOD1( createLiveRTSPStream, void( const std::string& streamItemId ) );
    MOCK_METHOD1( deleteLiveRTSPStream, void( const std::string& streamItemId ) );
    MOCK_METHOD0( getCameraStreams, Json() );
};

}  // namespace UT
}  // namespace HubPlatform
