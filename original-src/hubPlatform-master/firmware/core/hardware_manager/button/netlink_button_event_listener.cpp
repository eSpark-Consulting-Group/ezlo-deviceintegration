#include "hardware_manager/button/netlink_button_event_listener.h"
#include "json_utils/json_utils.h"
#include "bus/messagesclasses.h"

NetlinkButtonEventListener::NetlinkButtonEventListener()
: m_netlinkStream()
, m_bus( "btnsrc", NetlinkButtonEventListener::bus_rx_cb ) {
    m_bus.join( "buttons_group" );
    m_bus.start();
}

void NetlinkButtonEventListener::listen() {
    ButtonEvent evt( 0 );
    m_netlinkStream >> evt;
    JsonObject eventData = JSON( "name" << evt.getName() << "type" << ButtonEvent::eventTypeString( evt.getType() )
                                        << "secondsPassed" << evt.getSecondsPassed() );
    m_bus.send( "buttons_group", EZLO_EVENT, BUTTON_EVENT, eventData );
}

void NetlinkButtonEventListener::bus_rx_cb( const bus_rx_data_t* rx_data ) {
    return;
}
