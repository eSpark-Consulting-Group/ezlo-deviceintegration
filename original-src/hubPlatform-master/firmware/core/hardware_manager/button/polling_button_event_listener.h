#pragma once

#include "hardware_manager/button/button_event.h"
#include "hardware_manager/button/event_listener.h"
#include "bus/spread_bus.h"
#include <string>
#include <chrono>

class PollingButtonEventListener final : public EventListener {
public:
    static void bus_rx_cb( const bus_rx_data_t* rx_data );

public:
    PollingButtonEventListener( std::string buttonName, std::string handler );
    void listen() override;

private:
    std::string m_buttonName;
    std::string m_handler;
    ButtonEventType m_type;
    std::chrono::system_clock::time_point m_secondsPassed;
    SpreadBus m_bus;

private:
    void processEvent( ButtonEventType newType );
};