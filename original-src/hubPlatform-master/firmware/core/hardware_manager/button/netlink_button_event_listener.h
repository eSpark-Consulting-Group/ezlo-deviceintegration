#pragma once

#include "hardware_manager/button/button_event.h"
#include "hardware_manager/button/event_listener.h"
#include "hardware_manager/button/netlink_istream.h"
#include "bus/spread_bus.h"

class NetlinkButtonEventListener final : public EventListener {
public:
    static void bus_rx_cb( const bus_rx_data_t* rx_data );

public:
    NetlinkButtonEventListener();
    void listen() override;

private:
    NetlinkIStream m_netlinkStream;
    SpreadBus m_bus;
};
