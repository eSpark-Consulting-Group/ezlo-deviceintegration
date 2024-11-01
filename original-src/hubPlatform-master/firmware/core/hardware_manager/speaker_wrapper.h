#pragma once

#ifdef __HUB_WITH_SPEAKER__
#include "hardware_manager/speaker/speaker.h"
#include "hardware_manager/speaker/environment/speaker_environment_impl.h"
#endif

#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace HardwareManager {

class Service;

class SpeakerWrapper {
public:
    SpeakerWrapper( Service& service );

    bool setUp();
    void tearDown();

public:
    JsonObject play( const JsonObject& request );
    JsonObject stop( const JsonObject& request );

private:
#ifdef __HUB_WITH_SPEAKER__
    Speaker::EnvironmentImpl m_environment;
    Speaker::Speaker m_speaker;
#endif
};

}  // namespace HardwareManager
}  // namespace HubPlatform