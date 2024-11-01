#include "cored/model/resources/cored_manual_device_adding_capability.h"
#include "cored/model/resources/cored_resources.h"

#include <cassert>

namespace HubPlatform {
namespace CoreD {
namespace Model {

std::string toString( ManualDeviceAddingCapability capability ) {
    switch( capability ) {
    case ManualDeviceAddingCapability::No:
        return Resources::ManualDeviceAdding::No;

    case ManualDeviceAddingCapability::Wizard:
        return Resources::ManualDeviceAdding::Wizard;

    default:
        assert( false && "Unsupported manual device adding capability" );
        return "";
    }
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
