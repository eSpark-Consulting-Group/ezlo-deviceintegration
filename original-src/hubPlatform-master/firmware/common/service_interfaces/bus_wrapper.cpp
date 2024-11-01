
#include "service_interfaces/bus_wrapper.h"
#include "service_interfaces/groups_names.h"

#include "log_utils/ezlo_log.h"

#include <thread>
#include <cassert>

namespace HubPlatform {
namespace Services {

BusWrapper::BusWrapper( int connectionAttempts )
: m_bus( nullptr, nullptr )
, m_connectionAttempts( connectionAttempts )
, m_createBus( []( const std::string& privateName, const RxCallBack& rxCallBack, bool silent, int connectionAttempts )
                   -> BaseBusPtr { return createBus( privateName, rxCallBack, silent, connectionAttempts ); } ) {}

BusWrapper::~BusWrapper() {}

void BusWrapper::setUp( const std::string& name, const RxCallBack& busCallBack ) {
    assert( !m_bus );
    m_bus = ( *m_createBus )( name, busCallBack, false, m_connectionAttempts );
    m_bus->join( GroupsNames::BROADCAST_GROUP );
    m_bus->join( name.c_str() );
    m_bus->start();
}

void BusWrapper::tearDown() {
    if( m_bus ) {
        m_bus->stop();
        m_bus.reset();
    }
}

const BaseBus& BusWrapper::getBus() const {
    assert( m_bus );
    return *m_bus;
}

BaseBus& BusWrapper::getBus() {
    assert( m_bus );
    return *m_bus;
}

}  // namespace Services
}  // namespace HubPlatform
