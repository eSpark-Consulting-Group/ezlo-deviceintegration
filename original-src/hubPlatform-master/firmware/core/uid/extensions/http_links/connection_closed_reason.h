#pragma once

namespace HubPlatform {
namespace HttpLinks {

enum class ConnectionClosedReason {
    NONE = -1,
    OK = 0,
    CLOSED_BY_CLIENT = 4,
    CLOSED_BY_SERVER = 5,
    TIMEOUT_IS_REACHED = 6,
    UNKNOWN = 7,
};

std::string reasonText( ConnectionClosedReason reason );
int toInt( ConnectionClosedReason reason );

inline std::string reasonText( ConnectionClosedReason reason ) {
    switch( reason ) {
    case ConnectionClosedReason::OK:
        return "Ok";
    case ConnectionClosedReason::CLOSED_BY_CLIENT:
        return "Closed by client";
    case ConnectionClosedReason::CLOSED_BY_SERVER:
        return "Closed by server";
    case ConnectionClosedReason::TIMEOUT_IS_REACHED:
        return "Timeout of response is reached";
    case ConnectionClosedReason::UNKNOWN:
        return "Unknown error";
    default:
        return "";
    }
}

inline int toInt( ConnectionClosedReason reason ) {
    return static_cast< int >( reason );
}

}  // namespace HttpLinks
}  // namespace HubPlatform