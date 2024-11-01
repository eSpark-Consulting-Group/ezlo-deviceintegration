#pragma once

#include "json_utils/json_fwd.h"

#include <chrono>
#include <string>

namespace HubPlatform {
namespace Cameras {

struct StreamStatus {
    enum class Status { Started, Stopped, Removed };

    using Clock = std::chrono::system_clock;

    StreamStatus( std::string pluginId,
                  std::string streamName,
                  std::string description,
                  std::string cameraUrl,
                  Clock::time_point creatingDate );

    std::string dump( uint8_t indentSize = 4 ) const;
    JsonObject toJson() const;
    static std::string toString( Status status );
    static std::string toString( const Clock::time_point& timePoint );

    Status status;

    std::string pluginId;
    std::string streamName;
    std::string description;
    std::string cameraUrl;
    std::string controllerUrl;

    std::string lastStreamCreatingError;
    std::string lastSnapshotCreatingForUiError;
    std::string lastSnapshotCreatingForCloudError;
    std::string lastPreviewParticleCreatingError;

    Clock::time_point lastCreatingDate;
    Clock::time_point lastRemoveDate;
    Clock::time_point lastStartDate;
    Clock::time_point lastStopDate;
};

}  // namespace Cameras
}  // namespace HubPlatform
