/*
 * signalHandler.h
 *
 *  Created on: 4 Aug 2017
 *      Author: vvs
 */

#ifndef TRUNK_HA_ZWAVED_SIGNALHANDLER_H_
#define TRUNK_HA_ZWAVED_SIGNALHANDLER_H_

#include <signal.h>
#include <stdexcept>

using std::runtime_error;

class SignalException final : public runtime_error {
public:
    SignalException( const std::string& message )
    : std::runtime_error( message ) {}
};

class SignalHandler final {
protected:
    static volatile sig_atomic_t m_gotExitSignal;

public:
    SignalHandler();
    ~SignalHandler();

    static sig_atomic_t gotExitSignal();
    static void setExitSignal( bool bExitSignal );

    static const volatile sig_atomic_t& gotExitSignalRef();

    void setupSignalHandlers();
    static void exitSigIntHandler( int ignored );
    static void exitSigTermHandler( int ignored );
    static void exitSigUserHandler( int ignored );
};

#endif /* TRUNK_HA_ZWAVED_SIGNALHANDLER_H_ */
