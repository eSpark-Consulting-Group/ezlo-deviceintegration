#include "zwave/serialization/zwave_node_base64_serializer.h"

#include "zwave/serialization/memory_file.h"
#include "common_utils/cpp14.h"

#include <tiny/tiny_utils.h>
#include <znet/znet_node.hpp>

namespace HubPlatform {
namespace Zwave {

std::unique_ptr< znet::ZNode > ZwaveNodeBase64Serializer::deserialize(
    const std::string& base64Data,
    znet::RequestsManager* requestManager,
    znet::IAssociationsConfigurator* associationsConfigurator,
    znet::ReachableCheckTimeoutSettings* reachableCheckTimeoutSettings ) {
    auto node =
        std::make_unique< znet::ZNode >( requestManager, associationsConfigurator, reachableCheckTimeoutSettings );

    const std::string rawData = tiny::tools::base64::decode( base64Data );
    MemoryFile file( rawData.data(), rawData.size() );
    node->restoreData( file );

    return node;
}

void ZwaveNodeBase64Serializer::deserializeExistingNode( znet::ZNode& node, const std::string& base64Data ) {
    const std::string rawData = tiny::tools::base64::decode( base64Data );
    MemoryFile file( rawData.data(), rawData.size() );
    node.restoreData( file );
}

std::string ZwaveNodeBase64Serializer::serialize( const znet::ZNode& node ) {
    MemoryFile file;
    node.storeData( file );
    const auto& fileData = file.getData();
    return tiny::tools::base64::encode( reinterpret_cast< const unsigned char* >( fileData.data() ), fileData.size() );
}

}  // namespace Zwave
}  // namespace HubPlatform
