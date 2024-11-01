#include "hardware_manager/button/polling_button_event_listener.h"
#include "json_utils/json_utils.h"
#include "bus/messagesclasses.h"

#include <fstream>
#include <chrono>
#include <thread>

PollingButtonEventListener::PollingButtonEventListener( std::string buttonName, std::string handler )
: m_buttonName( std::move( buttonName ) )
, m_handler( std::move( handler ) )
, m_type( ButtonEventType::Unknown )
, m_secondsPassed()
, m_bus( "btnsrc", PollingButtonEventListener::bus_rx_cb ) {
    m_bus.join( "buttons_group" );
    m_bus.start();
    m_secondsPassed = std::chrono::system_clock::now();
}

void PollingButtonEventListener::processEvent( ButtonEventType newType ) {
    auto now = std::chrono::system_clock::now();
    int secondsPassed = std::chrono::duration_cast< std::chrono::seconds >( now - m_secondsPassed ).count();
    JsonObject eventData = JSON( "name" << m_buttonName << "type" << ButtonEvent::eventTypeString( newType )
                                        << "secondsPassed" << secondsPassed );
    if( m_type != ButtonEventType::Unknown ) {
        m_bus.send( "buttons_group", EZLO_EVENT, BUTTON_EVENT, eventData );
    }
    m_type = newType;
    m_secondsPassed = now;
}

void PollingButtonEventListener::listen() {
    std::ifstream in( m_handler + "/value" );
    std::string state;
    in >> state;
    in.close();
    if( state == "1" && m_type != ButtonEventType::Released ) {
        processEvent( ButtonEventType::Released );
    }
    else if( state == "0" && m_type != ButtonEventType::Pressed ) {
        processEvent( ButtonEventType::Pressed );
    }
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
}

void PollingButtonEventListener::bus_rx_cb( const bus_rx_data_t* rx_data ) {
    return;
}