#include <signal.h>
#include <errno.h>
#include <iostream>

#include "base_utils/signal_handler.h"

volatile sig_atomic_t SignalHandler::m_gotExitSignal = 0;

/**
 * Default Contructor.
 */
SignalHandler::SignalHandler() {}

/**
 * Destructor.
 */
SignalHandler::~SignalHandler() {}

/**
 * Returns the bool flag indicating whether we received an exit signal
 * @return Flag indicating shutdown of program
 */
sig_atomic_t SignalHandler::gotExitSignal() {
    return m_gotExitSignal;
}

/**
 * Sets the bool flag indicating whether we received an exit signal
 */
void SignalHandler::setExitSignal( bool bExitSignal ) {
    m_gotExitSignal = bExitSignal;
}

const volatile sig_atomic_t& SignalHandler::gotExitSignalRef() {
    return m_gotExitSignal;
}

/**
 * Sets exit signal to true.
 * @param[in] _ignored Not used but required by function prototype
 *                     to match required handler.
 */
void SignalHandler::exitSigIntHandler( int ignored ) {
    m_gotExitSignal = true;
}
/**
 * Sets exit signal to true.
 * @param[in] _ignored Not used but required by function prototype
 *                     to match required handler.
 */
void SignalHandler::exitSigTermHandler( int ignored ) {
    m_gotExitSignal = true;
}

/**
 * Sets exit signal to true.
 * @param[in] _ignored Not used but required by function prototype
 *                     to match required handler.
 */
void SignalHandler::exitSigUserHandler( int ignored ) {
    m_gotExitSignal = true;
}

/**
 * Set up the signal handlers for CTRL-C.
 */
void SignalHandler::setupSignalHandlers() {
    if( signal( static_cast< int >( SIGINT ), SignalHandler::exitSigIntHandler ) == SIG_ERR ) {
        throw SignalException( "Error setting up signal handlers - SIGINT !" );
    }
    if( signal( static_cast< int >( SIGTERM ), SignalHandler::exitSigTermHandler ) == SIG_ERR ) {
        throw SignalException( "Error setting up signal handlers - SIGTERM !" );
    }
    if( signal( static_cast< int >( SIGUSR1 ), SignalHandler::exitSigUserHandler ) == SIG_ERR ) {
        throw SignalException( "Error setting up signal handlers - SIGUSR1 !" );
    }
    if( signal( (int) SIGABRT, SignalHandler::exitSigTermHandler ) == SIG_ERR ) {
        throw SignalException( "Error setting up signal handlers - SIGABRT !" );
    }
}
