#pragma once

#include <string>

class lws;

namespace HubPlatform {
namespace Uid {
namespace Extensions {
namespace Utils {

int writeError( struct lws* wsi, int code, const std::string& errorMessage = "" );

}  // namespace Utils
}  // namespace Extensions
}  // namespace Uid
}  // namespace HubPlatform
