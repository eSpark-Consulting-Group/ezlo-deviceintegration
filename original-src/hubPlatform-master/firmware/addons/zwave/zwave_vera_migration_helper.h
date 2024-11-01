#pragma once

#include "json_utils/json_utils.h"

#include <znet/optional.hpp>

#include <set>

namespace HubPlatform {
namespace Zwave {

class VeraMigrationHelper {
public:
    VeraMigrationHelper( const std::string& pathToS0KeyFile, const std::string& pathToNodesInfoFile );

    void addNodeToRediscover( uint8_t nodeId );

    void onNodeRediscovered( uint8_t nodeId );

    znet::utils::optional< uint8_t > getNextNodeToRediscover() const;

    bool isRediscoverProcessCompleted() const;

    bool fillS0Key( uint8_t* buf ) const;

    Json readVeraNodesInformation();

private:
    std::set< uint8_t > m_nodesToRediscover;
    std::string m_pathToS0KeyFile;
    std::string m_pathToNodesInfoFile;
};

}  // namespace Zwave
}  // namespace HubPlatform
