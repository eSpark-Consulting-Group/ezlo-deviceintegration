#include "bluetooth/exceptions/custom_setup_not_implemented.h"

namespace HubPlatform {

const char* CustomSetupNotImplemented::what() const noexcept {
    return "Proxy method has args, but custom setup doesn't implemented";
}

}  // namespace HubPlatform
