#pragma once

#include <string>
#include <set>

namespace HubPlatform {
namespace Vidoo4Hub {
namespace DetectableTypes {

enum class Type { Object, Person, QRCode };
const std::set< Type >& getKnownTypes();
const std::string& getHotzoneTypeName( Type type );  // TODO add tests

}  // namespace DetectableTypes
}  // namespace Vidoo4Hub
}  // namespace HubPlatform
