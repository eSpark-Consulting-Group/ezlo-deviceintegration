#include "hardware_manager/led/led.h"
#include "hardware_manager/led/led_sercomm_gpio.h"
#include <string>
#include <iostream>
#include <fstream>

std::string LedSercommGPIO::procfsHandler; /* = "/proc/sercomm_gpio";*/

LedSercommGPIO::LedSercommGPIO( std::string name, int id )
: Led( std::move( name ) )
, m_id( id ) {}

bool LedSercommGPIO::turnOn() {
    if( isLocked() ) {
        return false;
    }
    if( state == LedState::LedOn ) {
        return true;
    }
    std::ofstream cmd( LedSercommGPIO::procfsHandler );
    if( !cmd ) {
        std::cerr << "Failed to open " << LedSercommGPIO::procfsHandler << std::endl;
        return false;
    }
    cmd << "ledon " << m_id << std::endl;
    cmd.close();
    state = LedState::LedOn;
    return true;
}

bool LedSercommGPIO::turnOff() {
    if( isLocked() ) {
        return false;
    }
    if( state == LedState::LedOff ) {
        return true;
    }
    std::ofstream cmd( LedSercommGPIO::procfsHandler );
    if( !cmd ) {
        std::cerr << "Failed to open " << LedSercommGPIO::procfsHandler << std::endl;
        return false;
    }
    cmd << "ledoff " << m_id << std::endl;
    cmd.close();
    state = LedState::LedOff;
    return true;
}

bool LedSercommGPIO::turnBlinking( int msOff, int msOn ) {
    if( isLocked() ) {
        return false;
    }
    std::ofstream cmd( LedSercommGPIO::procfsHandler );
    if( !cmd ) {
        std::cerr << "Failed to open " << LedSercommGPIO::procfsHandler << std::endl;
        return false;
    }
    cmd << "ledblink " << m_id << " " << msOn << " " << msOff << std::endl;
    cmd.close();
    state = LedState::LedBlinking;
    return true;
}
