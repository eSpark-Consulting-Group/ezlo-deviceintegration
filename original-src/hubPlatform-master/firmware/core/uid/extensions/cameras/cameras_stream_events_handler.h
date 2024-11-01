#pragma once

#include "json_utils/json_fwd.h"
#include <string>

namespace HubPlatform {
namespace Cameras {

// These events can be triggered asynchronously
class StreamEventsHandler {
public:
    virtual ~StreamEventsHandler() = default;

    virtual void onStreamStarted( const std::string& streamId, const JsonObject& userData ) = 0;
    virtual void onStreamStopped( const std::string& streamId, const JsonObject& userData ) = 0;
    virtual void onStreamError( const std::string& streamId,
                                const JsonObject& userData,
                                const std::string& errorMessage ) = 0;
};

}  // namespace Cameras
}  // namespace HubPlatform
