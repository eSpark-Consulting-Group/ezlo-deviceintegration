#pragma once

#include <znet/iznet_provisioning_list_controller.hpp>
#include <znet/znet_provisioning_list_entry.hpp>

#include <gmock/gmock.h>

namespace HubPlatform {
namespace Zwave {
namespace UT {

class ProvisioningListControllerMock : public ::znet::api::IProvisioningListController {
public:
    MOCK_METHOD2( addProvisioningListEntry,
                  OperationStatus( const znet::DSK& dsk, const znet::ProvisioningListEntryFields& fields ) );
    MOCK_METHOD2( setProvisioningListEntry,
                  OperationStatus( uint32_t provisioningId, const znet::ProvisioningListEntryFields& fields ) );
    MOCK_METHOD1( removeProvisioningListEntry, OperationStatus( uint32_t provisioningId ) );
    MOCK_CONST_METHOD1( findProvisioningListEntry, OperationStatus( const ProcessProvisioningCallback& callback ) );
};

}  // namespace UT
}  // namespace Zwave
}  // namespace HubPlatform
