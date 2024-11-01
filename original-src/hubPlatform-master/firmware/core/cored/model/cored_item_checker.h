#pragma once

#include "cored/model/values/value_type.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_wrapper.h"
#include "provision_data_utils/provision_data.h"

#include <string>

namespace HubPlatform {
namespace CoreD {
namespace Model {
namespace ItemChecker {
std::string itemValueTest( const std::string& type, const Json& value );
};

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
