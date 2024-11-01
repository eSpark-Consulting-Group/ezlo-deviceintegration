#pragma once

namespace HubPlatform {
namespace CommonHttp {

namespace Resources {
namespace HttpEvents {
constexpr const char HttpDataReceived[] = "http_data_received";
constexpr const char HttpDataSend[] = "http_data_send";
constexpr const char HttpConnectionClosed[] = "http_connection_closed";
constexpr const char HttpProgressEvent[] = "http_progress_event";
}  // namespace HttpEvents
}  // namespace Resources

}  // namespace CommonHttp
}  // namespace HubPlatform