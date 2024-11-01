#pragma once

#include "json_utils/json_fwd.h"

#include <memory>

namespace znet {
class ZNode;
class IAssociationsConfigurator;
class RequestsManager;
class ReachableCheckTimeoutSettings;
}  // namespace znet

namespace HubPlatform {
namespace Zwave {

class ZwaveNodeBase64Serializer {
public:
    static std::unique_ptr< znet::ZNode > deserialize(
        const std::string& base64Data,
        znet::RequestsManager* requestManager,
        znet::IAssociationsConfigurator* associationsConfigurator,
        znet::ReachableCheckTimeoutSettings* reachableCheckTimeoutSettings );

    static void deserializeExistingNode( znet::ZNode& node, const std::string& base64Data );

    static std::string serialize( const znet::ZNode& node );
};

}  // namespace Zwave
}  // namespace HubPlatform
