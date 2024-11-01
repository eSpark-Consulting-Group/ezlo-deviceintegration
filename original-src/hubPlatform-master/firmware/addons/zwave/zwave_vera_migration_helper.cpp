#include "zwave/zwave_vera_migration_helper.h"

#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include "base_utils/lzo_utils.h"

#include <functional>
#include <fstream>
#include <memory>
#include <cstring>

namespace HubPlatform {
namespace Zwave {

constexpr size_t KeyLength = 16;

VeraMigrationHelper::VeraMigrationHelper( const std::string& pathToS0KeyFile, const std::string& pathToNodesInfoFile )
: m_pathToS0KeyFile( pathToS0KeyFile )
, m_pathToNodesInfoFile( pathToNodesInfoFile ) {}

void VeraMigrationHelper::addNodeToRediscover( uint8_t nodeId ) {
    m_nodesToRediscover.insert( nodeId );
}

void VeraMigrationHelper::onNodeRediscovered( uint8_t nodeId ) {
    m_nodesToRediscover.erase( nodeId );
}

znet::utils::optional< uint8_t > VeraMigrationHelper::getNextNodeToRediscover() const {
    if( !m_nodesToRediscover.empty() ) {
        return *m_nodesToRediscover.begin();
    }

    return znet::utils::optional< uint8_t >();
}

bool VeraMigrationHelper::isRediscoverProcessCompleted() const {
    return m_nodesToRediscover.empty();
}

bool VeraMigrationHelper::fillS0Key( uint8_t* buf ) const {
    std::ifstream stream( m_pathToS0KeyFile, std::ifstream::binary );
    if( !stream.is_open() ||
        std::distance( buf, std::copy_n( std::istreambuf_iterator< char >( stream ), KeyLength, buf ) ) != KeyLength ) {
        Log::error( cc::error( "Vera S0 key read fail!" ) );
        memset( buf, 0, KeyLength );
        return false;
    }

    return true;
}

Json VeraMigrationHelper::readVeraNodesInformation() {
    std::string target;
    if( common::lzo_utils::decompressFile( m_pathToNodesInfoFile, target ) ) {
        try {
            auto json = Json::parse( target );
            return json;
        }
        catch( const std::exception& e ) {
            return JSON_EMPTY_OBJ;
        }
    }

    return JSON_EMPTY_OBJ;
}

}  // namespace Zwave
}  // namespace HubPlatform
