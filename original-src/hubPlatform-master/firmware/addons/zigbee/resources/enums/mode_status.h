#pragma once

#include "zigbee/resources/zigbee_resources.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

enum class ModeStatus { Invoked = 0, Started, Finished, InProgress, Failed, Timeout, Busy, Empty, Undefined };

inline char const* toString( ModeStatus status ) {
    switch( status ) {
    case ModeStatus::Invoked:
        return "invoked";
    case ModeStatus::Started:
        return "started";
    case ModeStatus::Finished:
        return "finished";
    case ModeStatus::InProgress:
        return "in_progress";
    case ModeStatus::Failed:
        return "failed";
    case ModeStatus::Timeout:
        return "timeout";
    case ModeStatus::Busy:
        return "busy";
    case ModeStatus::Empty:
        return "empty";
    case ModeStatus::Undefined:
        return "undefined";

    default:
        assert( !"Unsupported ModeStatus" );
        return "";
    }
}

}  // namespace Zigbee
}  // namespace HubPlatform
