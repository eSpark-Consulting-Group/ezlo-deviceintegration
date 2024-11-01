#pragma once

#include "cored/group_operations/remote_interface/item_group_operation.h"
#include "cored/group_operations/remote_interface/device_group_operation.h"

namespace HubPlatform {
namespace Services {
class Service;
}
}  // namespace HubPlatform

namespace HubPlatform {
namespace CoreD {

class GroupOperationInterface {
public:
    explicit GroupOperationInterface( Services::Service& hostService );
    DeviceGroupOperation& deviceGroup();
    ItemGroupOperation& itemGroup();

private:
    DeviceGroupOperation m_deviceGroupOperations;
    ItemGroupOperation m_itemGroupOperations;
};

}  // namespace CoreD
}  // namespace HubPlatform
