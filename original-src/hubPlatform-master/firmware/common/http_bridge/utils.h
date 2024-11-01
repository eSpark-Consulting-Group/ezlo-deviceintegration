#pragma once

#include <vector>
#include <string>

class lws;

namespace HubPlatform {
namespace HttpBridge {
namespace Utils {

using KeyValue = std::pair< std::string, std::string >;
using KeyValueVector = std::vector< KeyValue >;

KeyValueVector extractUriParams( lws* wsi );
const char* extractMethod( lws* wsi );
KeyValueVector extractHeaders( lws* wsi );
KeyValue extractAuthData( lws* wsi );

}  // namespace Utils
}  // namespace HttpBridge
}  // namespace HubPlatform