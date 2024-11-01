#include "hardware_manager/led/led_sysfs.h"
#include <string>
#include <fstream>
#include <iostream>

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
namespace {
constexpr char ledPath[] = "/proc/sercomm_gpio";
}

LedSysfs::LedSysfs( std::string name, uint ledNumber )
: Led( std::move( name ) )
, m_ledNumber( ledNumber ) {}
#else
LedSysfs::LedSysfs( std::string name, std::string sysfsHandler )
: Led( std::move( name ) )
, m_sysfsHandler( std::move( sysfsHandler ) ) {}
#endif

bool LedSysfs::turnOn() {
    return turn( true );
}

bool LedSysfs::turnOff() {
    return turn( false );
}

#if defined( __HUB_TYPE_G150__ ) || defined( __HUB_TYPE_G450__ ) || defined( __HUB_TYPE_G550__ )
bool LedSysfs::turn( bool enable ) {
    if( isLocked() ) {
        return false;
    }

    LedState stateToBeSet = ( enable ) ? LedState::LedOn : LedState::LedOff;
    if( state == stateToBeSet ) {
        return true;
    }

    std::string cmdHandler = ledPath;
    std::ofstream cmd( cmdHandler );
    if( !cmd ) {
        std::cerr << "Failed to open " << cmdHandler << std::endl;
        return false;
    }

    if( enable ) {
        cmd << "ledon " + std::to_string( m_ledNumber ) << std::endl;
    }
    else {
        cmd << "ledoff " + std::to_string( m_ledNumber ) << std::endl;
    }
    state = stateToBeSet;
    return true;
}

bool LedSysfs::turnBlinking( int msOff, int msOn ) {
    if( isLocked() ) {
        return false;
    }

    std::string cmdHandler = ledPath;
    std::ofstream cmd( cmdHandler );
    if( !cmd ) {
        std::cerr << "Failed to open " << cmdHandler << std::endl;
        return false;
    }
    cmd << "ledblink " + std::to_string( m_ledNumber ) + " " + std::to_string( msOn / 100 ) + " " +
               std::to_string( msOff / 100 );

    state = LedState::LedBlinking;
    return true;
}
#else
bool LedSysfs::turn( bool enable ) {
    if( isLocked() ) {
        return false;
    }
    LedState stateToBeSet = ( enable ) ? LedState::LedOn : LedState::LedOff;
    if( state == stateToBeSet ) {
        return true;
    }
    std::string cmdHandler = m_sysfsHandler + "/brightness";
    std::ofstream cmd( cmdHandler );
    if( !cmd ) {
        std::cerr << "Failed to open " << cmdHandler << std::endl;
        return false;
    }
    if( enable ) {
        cmd << "255" << std::endl;
    }
    else {
        cmd << "0" << std::endl;
    }
    state = stateToBeSet;
    return true;
}

bool LedSysfs::turnBlinking( int msOff, int msOn ) {
    if( isLocked() ) {
        return false;
    }
    std::ofstream delayOn( m_sysfsHandler + "/delay_on" );
    std::ofstream delayOff( m_sysfsHandler + "/delay_off" );
    if( ( !delayOn ) || ( !delayOff ) ) {
        std::cerr << "Failed to open " << m_sysfsHandler << " delay handlers\n";
        return false; /*TODO add own thread here*/
    }
    delayOn << msOn << std::endl;
    delayOff << msOff << std::endl;
    delayOn.close();
    delayOff.close();
    state = LedState::LedBlinking;
    return true;
}
#endif
