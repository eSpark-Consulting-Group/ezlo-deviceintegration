#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "vidoo4hub/stream_manager/stream_manager.h"

namespace HubPlatform {
namespace UT {

class FakeStreamManager : public Vidoo4Hub::IStreamManager {
public:
    void addCameraStream( const std::string& streamItemId,
                          const std::string& streamName,
                          const std::string& rtspURL ) override{};
    void removeCameraStream( const std::string& streamItemId ) override{};
    void createLiveRTSPStream( const std::string& streamItemId ) override{};
    void deleteLiveRTSPStream( const std::string& streamItemId ) override{};
    Json getCameraStreams() override {
        return JSON_EMPTY_ARR;
    };
};

}  // namespace UT
}  // namespace HubPlatform
