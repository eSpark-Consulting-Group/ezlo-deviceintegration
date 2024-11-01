#pragma once

#include <string>

namespace HubPlatform {
namespace Zigbee {
namespace AlertNotificationCategory {

enum class Category : uint8_t { Reserved = 0x00, Warning = 0x01, Danger = 0x02, Failure = 0x03 };

const char* toString( Category id );
Category AlertNotificationCategorySourceFromString( const std::string& id );

}  // namespace AlertNotificationCategory
}  // namespace Zigbee
}  // namespace HubPlatform
