#include "bluetooth/exceptions/bad_property_type.h"

namespace HubPlatform {

const char* BadPropertyType::what() const noexcept {
    return "Bad property type";
}

}  // namespace HubPlatform
