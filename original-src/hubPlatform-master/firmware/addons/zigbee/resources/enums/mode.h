#pragma once

#include "zigbee/resources/zigbee_resources.h"

namespace HubPlatform {
namespace Zigbee {

enum class Mode { Initializing = 0, Including, Idle };

inline char const* toString( Mode mode ) {
    switch( mode ) {
    case Mode::Initializing:
        return "initializing";
    case Mode::Including:
        return "including";
    case Mode::Idle:
        return "idle";

    default:
        assert( !"Unsupported ModeStatus" );
        return "";
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
