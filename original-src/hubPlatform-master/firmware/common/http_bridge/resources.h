#pragma once

namespace HubPlatform {
namespace HttpBridge {

namespace Resources {
constexpr const char apiBridgeEndPoint[] = "/v1";
constexpr const char apiBridgeProtocol[] = "api-bridge-http";
constexpr const char sessionTypeHttp[] = "http";

constexpr const char eventStreamEndPoint[] = "/v1/broadcasts";
constexpr const char eventStreamProtocol[] = "event-stream-api-bridge-http";
constexpr const char sessionTypeEventStream[] = "event-stream";

constexpr const char camerasAddonEndPoint[] = "/v1/addons/cameras/stream/snapshot";
constexpr const char camerasAddonProtocol[] = "addons-cameras-http";

constexpr const char addonsEndPoint[] = "/v1/addons";
constexpr const char addonsProtocol[] = "addons-http";
constexpr const char httpLinksEndPoint[] = "/v1/links";
constexpr const char httpLinksProtocol[] = "http-links-http";

constexpr const char eventMessage[] = "message";
constexpr const char eventListenBroadcasts[] = "listen_broadcasts";
constexpr const char eventAuthorization[] = "authorization";

constexpr const char httpMethodGet[] = "GET";
constexpr const char httpMethodPost[] = "POST";

namespace Metrics {
constexpr const char eventRequestStarted[] = "request_started";
constexpr const char eventRequestFinished[] = "request_finished";
constexpr const char eventRequestFailed[] = "request_failed";
constexpr const char eventRequestsLimit[] = "requests_limit";
constexpr const char eventPayloadRx[] = "request_rx_payload";
constexpr const char eventPayloadTx[] = "request_tx_payload";
constexpr const char eventRequestUnauthorized[] = "request_not_auth";
constexpr const char eventRequestTimeOut[] = "request_time_out";
constexpr const char eventRequestUnhandled[] = "request_unhandled";
constexpr const char eventParallelRequests[] = "parallel_requests";
}  // namespace Metrics

namespace EndpointHandlers {
constexpr const char Main[] = "main";
constexpr const char Broadcasts[] = "broadcasts";
constexpr const char Cameras[] = "cameras";
constexpr const char UiExtensions[] = "ui_extensions";
}  // namespace EndpointHandlers

constexpr int PARALLEL_SESSIONS_LIMIT = 100;

constexpr int HTTP_STATUS_CONTROLLER_PARALLEL_LIMIT_REACHED = 429;

constexpr int REQUEST_DURATION_LIMIT = 60 * 1000;
constexpr int HTTP_STATUS_REQUEST_TIMEOUT = 408;

#ifdef _DEBUG
constexpr const int JSON_INDENT = 4;
#else
constexpr const int JSON_INDENT = -1;
#endif

};  // namespace Resources

}  // namespace HttpBridge
}  // namespace HubPlatform