#include "bluetooth/exceptions/bad_property.h"

namespace HubPlatform {

const char* BadProperty::what() const noexcept {
    return "Bad property";
}

}  // namespace HubPlatform
