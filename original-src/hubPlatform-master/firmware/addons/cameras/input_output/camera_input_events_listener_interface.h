#pragma once

namespace HubPlatform {

class ICameraInputEventsListener {
public:
    virtual ~ICameraInputEventsListener() = default;

    virtual void onInputReadingStopped( int errorCode ) = 0;
    virtual void onMaxPacketsDelayViolated() = 0;
    virtual void onAccumulatedDelayThresholdReached() = 0;
};

}  // namespace HubPlatform
