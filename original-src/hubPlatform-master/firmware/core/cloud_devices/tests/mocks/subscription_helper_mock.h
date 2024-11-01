#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "cloud_devices/subscription_manager/subscription_helper.h"

namespace HubPlatform {
namespace UT {

class SubscriptionHelperMock : public HubPlatform::CloudDevices::SubscriptionHelper {
public:
    SubscriptionHelperMock( Services::Service& hostService, CoreDRemoteInterface::CoreDInterface& core )
    : HubPlatform::CloudDevices::SubscriptionHelper( hostService, core ) {}

    MOCK_METHOD( Json, getGatewayByName, ( const std::string& gatewayName ), ( override ) );
    MOCK_METHOD( bool,
                 itemBelongsToGateway,
                 ( const std::string& itemId, const std::string& gatewayName ),
                 ( override ) );
    MOCK_METHOD( void, sendExecutePluginScript, ( const std::string& script, const Json& scriptParams ), ( override ) );
};

}  // namespace UT
}  // namespace HubPlatform
