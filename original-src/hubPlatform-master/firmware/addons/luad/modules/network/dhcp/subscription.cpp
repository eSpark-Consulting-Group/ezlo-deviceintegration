#include "luad/modules/network/dhcp/subscription.h"

namespace HubPlatform {
namespace LuaNetwork {

namespace Dhcp {

bool Subscription::hasOperations() const {
    return !m_operations.empty();
}

bool Subscription::contains( const std::string& eventName ) const {
    return std::binary_search( m_operations.begin(), m_operations.end(), eventName );
}

const std::string& Subscription::scriptPath() const {
    return m_path;
}

Subscription::Subscription( std::string handlerPath, Operations operations )
: m_path( std::move( handlerPath ) )
, m_operations( std::move( operations ) ) {
    sortUnique();
}

void Subscription::sortUnique() {
    if( m_operations.empty() )
        return;

    auto eit = end( m_operations );
    std::sort( begin( m_operations ), eit );
    auto newEnd = std::unique( begin( m_operations ), eit );
    if( eit != newEnd ) {
        m_operations.erase( newEnd, eit );
        m_operations.shrink_to_fit();
    }
}

}  // namespace Dhcp
}  // namespace LuaNetwork
}  // namespace HubPlatform