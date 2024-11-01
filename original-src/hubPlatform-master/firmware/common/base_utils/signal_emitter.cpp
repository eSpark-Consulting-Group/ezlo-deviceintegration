#include "signal_emitter.h"
#include <signal.h>

SignalEmitter::SignalEmitter() = default;
SignalEmitter::~SignalEmitter() = default;

void SignalEmitter::emitUserSignal() {
    raise( SIGUSR1 );
}

void SignalEmitter::emitIntSignal() {
    raise( SIGINT );
}

void SignalEmitter::emitTermSignal() {
    raise( SIGTERM );
}
