#include "hardware_manager/button/event_listener.h"
#include <algorithm>

EventListener::EventListener()
: Runnable() {}

void EventListener::startInternal() {
    while( isRunning() ) {
        listen();
    }
}
