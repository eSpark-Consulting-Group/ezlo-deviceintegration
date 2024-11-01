#pragma once

#include "bus/base_bus.h"

class SpreadBus;

namespace HubPlatform {
namespace MQTTGateway {

class SpreadBusWrapper : public BaseBus {
public:
    using ForwardCallback = std::function< bool( const bus_rx_data_t* ) >;

    SpreadBusWrapper( const std::string& pname, const RxCallBack& rxCallback, bool silent, int connectionAttempts );
    ~SpreadBusWrapper();

    bool isRegularMessage( int msgType ) const override;
    void start() override;
    void stop() override;
    void join( const char* group ) override;
    bool multicast( const char* group, int msgClass, const std::string& msg ) override;

    void setForwardCallback( ForwardCallback callback );

private:
    std::unique_ptr< SpreadBus > m_bus;
    ForwardCallback m_forwardCallback;
};

}  // namespace MQTTGateway
}  // namespace HubPlatform
