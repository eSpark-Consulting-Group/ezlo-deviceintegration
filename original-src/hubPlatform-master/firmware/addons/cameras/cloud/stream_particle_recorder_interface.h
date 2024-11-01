#pragma once

#include "cameras/defs.h"

namespace HubPlatform {

struct StreamParticleSettings {
    std::chrono::seconds preRecordDuration{ 0 };
    std::chrono::seconds recordDuration{ 0 };
    uint32_t resolutionDivisor{ 0 };
    std::string format;
};

class IStreamParticleRecorder {
public:
    virtual ~IStreamParticleRecorder() = default;

    virtual void createRecording( const StreamParticleSettings& settings, SaveFileCallback saveFile ) = 0;
    virtual void deleteRecording() = 0;
    virtual void makeRecord( const Json& metaData ) = 0;
    virtual void makeRecord() = 0;
    virtual void stopMakingRecord() = 0;

    virtual bool isRecordingCreated() const = 0;
    virtual bool isMakingRecord() const = 0;
};

}  // namespace HubPlatform
