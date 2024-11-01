#pragma once

#include <stdint.h>
#include "zigbee/model/enums/commands/alert_notification_cotegory_source.h"
#include "zigbee/model/enums/commands/get_alerts_responce_source.h"

namespace HubPlatform {
namespace Zigbee {

struct AlertStructure {
    uint8_t alert_id;
    AlertNotificationCategory::Category category;
    AlertNotificationPresenceRecovery::PresenceRecovery presence_recovery;
    uint8_t manufacturer_specific_bits;
};

enum class AlertSource { Response = 0x00, Notification = 0x01 };

}  // namespace Zigbee
}  // namespace HubPlatform
