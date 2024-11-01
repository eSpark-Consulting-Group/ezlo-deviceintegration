#pragma once

namespace HubPlatform {
namespace HttpLinks {
namespace Resources {

namespace HttpLinkFields {
constexpr const char linkId[] = "linkId";
constexpr const char handler[] = "handler";
constexpr const char userData[] = "userData";
constexpr const char timeout[] = "timeout";
constexpr const char answer[] = "answer";
constexpr const char url[] = "url";
constexpr const char secureUrl[] = "secureUrl";
constexpr const char pluginId[] = "pluginId";
constexpr const char connections[] = "connections";
constexpr const char connectionId[] = "connectionId";
constexpr const char deleted[] = "deleted";
}  // namespace HttpLinkFields

namespace ConnectionDbFields {
constexpr const char linkId[] = "linkId";
constexpr const char id[] = "_id";
constexpr const char created[] = "created";
constexpr const char closed[] = "closed";
constexpr const char clientIp[] = "clientIp";
constexpr const char headers[] = "headers";
constexpr const char params[] = "params";
constexpr const char status[] = "status";
}  // namespace ConnectionDbFields

constexpr const char linksPluginsEndpointPath[] = "/v1/links/plugins";

namespace Events {
const char* const HttpDataReceivedEvent = "http_data_received";
const char* const HttpConnectionClosedEvent = "http_connection_closed";
const char* const HttpDataSendEvent = "http_data_send";
const char* const HttpLinkUpdatedEvent = "http_link_updated";
}  // namespace Events

namespace Limits {
/** Limits size of http_data_received data to pass to a bus. Larger values breaks all. */
const int MAX_DATA_RECEIVED_PORTION = 32000;
/** Limits size of http_data_send data to pass to a bus. */
const int MAX_DATA_SEND_PORTION = 32000;

const int RESPONSE_TIMEOUT_DEFAULT = 30000;  // 30 seconds
const int RESPONSE_TIMEOUT_LIMIT = 3600000;  // 1 hour

const int HttpLinksConnectionsCleanUpIntervalSec = 60;
const int HttpLinksConnectionsLifetimeAfterClosedSec = 60;
const int HttpLinksLifetimeAfterDeletedSec = 60;

}  // namespace Limits

namespace HttpCodes {
static constexpr int Continue = 100;
static constexpr int Ok = 200;
static constexpr int BadRequest = 400;
static constexpr int MethodNotAllowed = 405;
static constexpr int RequestTimeout = 408;
static constexpr int ClientClosedRequest = 499;
static constexpr int InternalServerError = 500;
static constexpr int NotFound = 404;
static constexpr int ServiceUnavailable = 503;
static constexpr int NetworkAuthenticationRequired = 511;
}  // namespace HttpCodes

}  // namespace Resources
}  // namespace HttpLinks
}  // namespace HubPlatform