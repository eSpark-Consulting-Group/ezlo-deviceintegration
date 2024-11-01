#pragma once

namespace HubPlatform {
namespace Network {

const char* const BssidKey = "bssid";
const char* const SsidKey = "ssid";
const char* const EncrypyionKey = "encryption";
const char* const PassKey = "passkey";
const char* const WifiListKey = "networks";

const char* const JsonAnswerSuccessStatusPattern = "{\"status\": \"success\", \"state\":\"%s\", \"SN\": \"%s\", "
                                                   "\"SW\": \"%s\", \"MAC\": \"%s\", \"UUID\": \"%s\"}\r\n\r\n";

const char* const JsonConnectSuccessAnswer = "{\"status\": \"success\",\"state\": \"connecting\"}\r\n\r\n";

const char* const JsonResponseHeader =
    "HTTP/1.1 %s\r\nAccess-Control-Allow-Origin: *\r\nUser-Agent: eZlo HUB Platform\r\n";
const char* const BadRequest = "400 Bad Request";
const char* const InternalError = "500 Internal Server Error";
const char* const NotFound = "404 Not Found";

}  // namespace Network
}  // namespace HubPlatform
