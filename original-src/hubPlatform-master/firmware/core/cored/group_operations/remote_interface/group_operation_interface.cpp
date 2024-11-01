#include "group_operation_interface.h"
#include "cored/group_operations/remote_interface/device_group_operation.h"

namespace HubPlatform {
namespace CoreD {

GroupOperationInterface::GroupOperationInterface( Services::Service& hostService )
: m_deviceGroupOperations( hostService )
, m_itemGroupOperations( hostService ) {}

DeviceGroupOperation& GroupOperationInterface::deviceGroup() {
    return m_deviceGroupOperations;
}

ItemGroupOperation& GroupOperationInterface::itemGroup() {
    return m_itemGroupOperations;
}

}  // namespace CoreD
}  // namespace HubPlatform
