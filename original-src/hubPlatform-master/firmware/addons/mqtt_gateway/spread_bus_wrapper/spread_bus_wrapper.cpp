#include "mqtt_gateway/spread_bus_wrapper/spread_bus_wrapper.h"

#include "bus/spread_bus.h"
#include "common_utils/cpp14.h"

class SpreadBus;

namespace HubPlatform {
namespace MQTTGateway {

SpreadBusWrapper::SpreadBusWrapper( const std::string& pname,
                                    const RxCallBack& rxCallback,
                                    bool silent,
                                    int connectionAttempts )
: BaseBus( pname, rxCallback ) {
    auto callback = [this, rxCallback]( const bus_rx_data_t* data ) {
        if( !m_forwardCallback || !m_forwardCallback( data ) )
            rxCallback( data );
    };

    m_bus = std::make_unique< SpreadBus >( pname, std::move( callback ), silent, connectionAttempts );
}

SpreadBusWrapper::~SpreadBusWrapper() = default;

bool SpreadBusWrapper::isRegularMessage( int msgType ) const {
    return m_bus->isRegularMessage( msgType );
}

void SpreadBusWrapper::start() {
    m_bus->start();
}

void SpreadBusWrapper::stop() {
    m_bus->stop();
}

void SpreadBusWrapper::join( const char* group ) {
    m_bus->join( group );
}

bool SpreadBusWrapper::multicast( const char* group, int msgClass, const std::string& msg ) {
    return m_bus->multicast( group, msgClass, msg );
}

void SpreadBusWrapper::setForwardCallback( ForwardCallback callback ) {
    m_forwardCallback = std::move( callback );
}

}  // namespace MQTTGateway
}  // namespace HubPlatform
