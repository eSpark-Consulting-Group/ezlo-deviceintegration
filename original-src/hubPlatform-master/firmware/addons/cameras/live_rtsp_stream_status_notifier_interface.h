#pragma once

#include "defs.h"

#include <string>

namespace HubPlatform {

class ILiveRTSPStreamStatusNotifier {
public:
    virtual ~ILiveRTSPStreamStatusNotifier() = default;

    using StreamId = std::string;
    using LiveRTSPStreamLink = std::string;

    virtual void onRTSPStreamStarted( const StreamId& streamId, const LiveRTSPStreamLink& link ) = 0;
    virtual void onRTSPStreamStopped( const StreamId& streamId ) = 0;
    virtual void onRTSPStreamCreationFailed( const StreamId& streamId, const std::string& errorMessage ) = 0;
    virtual void onRTSPStreamError( const StreamId& streamId, StreamError error ) = 0;
};

}  // namespace HubPlatform
