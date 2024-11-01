#include "scenes_manager/scenes_engine/events/network_state_event.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

namespace SceneEngine {

std::string NetworkStateEvent::getMethodName() const {
    return Model::Resources::MethodNames::isNetworkState;
}

NetworkStateEvent::NetworkStateEvent( InterfaceId interfaceId )
: m_interfaceId( std::move( interfaceId ) ) {}

NetworkFailedStateEvent::NetworkFailedStateEvent( InterfaceId interfaceId )
: NetworkStateEvent( interfaceId ) {}

bool NetworkFailedStateEvent::compare( const InterfaceId& interfaceId, bool isConnected ) {
    InterfaceId id = m_interfaceId;
    if( id.empty() ) {
        if( !m_getMainInterface ) {
            return false;
        }
        id = m_getMainInterface();
    }
    if( id != interfaceId ) {
        return false;
    }
    return !isConnected;
}

void NetworkFailedStateEvent::setMainInterfaceCallback( GetMainInterface callback ) {
    m_getMainInterface = callback;
}

NetworkMainStateEvent::NetworkMainStateEvent( InterfaceId interfaceId )
: NetworkStateEvent( interfaceId ) {}

bool NetworkMainStateEvent::compare( const InterfaceId& interfaceId ) {
    bool result;
    if( m_interfaceId.empty() ) {
        result = true;
    }
    else {
        result = m_interfaceId == interfaceId;
    }
    return result;
}

}  // namespace SceneEngine
