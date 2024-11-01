#pragma once

#include "json_utils/json_fwd.h"

extern "C" {
#include <Zigbee/definitions.h>
#include <protocol/zigbee/app/framework/include/af.h>
#include <protocol/zigbee/app/framework/include/af-types.h>
}

namespace HubPlatform {
namespace Zigbee {
namespace Backup {

struct ZigbeeNetworkInfo {
    int8u nodeId;             /// The ID of the trust center node.
    int16u panId;             /// 16-bit PAN ID for the network.
    int8s radioPower;         /// The network's operating radio power.
    int8u radioChannel;       /// The network's operating radio channel.
    int8u ePanId[ 8 ];        /// 64-bit extended PAN ID for the network.
    EmberEUI64 eui64;         /// EUI64 Manufacturing ID for the TC device.
    EmberKeyData globalKey;   /// Global key: value stored in globalKey.contents.
    int32u globalFC;          /// The frame counter for the global key.
    EmberKeyData networkKey;  /// Network key: value stored in networkKey.contents.
    int32u networkFC;         /// The frame counter for the network key.
    int8u networkKeySeqNum;   /// The sequence number for the network key.

    inline bool operator!=( const ZigbeeNetworkInfo& rhs ) {
        return panId != rhs.panId || nodeId != rhs.nodeId || radioChannel != rhs.radioChannel || ePanId != rhs.ePanId ||
               eui64 != rhs.eui64;
    }
};

Json networkInfoToJson( const ZigbeeNetworkInfo& info );
ZigbeeNetworkInfo networkInfoFromJson( const Json& info );

}  // namespace Backup
}  // namespace Zigbee
}  // namespace HubPlatform
