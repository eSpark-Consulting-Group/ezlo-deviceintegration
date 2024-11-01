#pragma once

#include "cameras/defs.h"

namespace HubPlatform {

class ICloudStreamSaver {
public:
    virtual bool isSavingCreated() const = 0;
    virtual bool isStarted() const = 0;
    virtual void createRecording( int segmentDurationSec,
                                  int64_t secondsBeforeStart,
                                  int segmentsLimit,
                                  const std::string& format,
                                  SaveFileCallback saveFile ) = 0;
    virtual void deleteRecording() = 0;
    virtual void startRecording( const Json& metaData, SaveFileCallback saveFile ) = 0;
    virtual void startRecording() = 0;
    virtual void stopRecording() = 0;

    virtual ~ICloudStreamSaver() = default;
};

}  // namespace HubPlatform
