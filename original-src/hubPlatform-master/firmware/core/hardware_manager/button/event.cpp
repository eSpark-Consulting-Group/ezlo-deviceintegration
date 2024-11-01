#include "hardware_manager/button/event.h"

Event::Event( int priority )
: m_priority( priority ) {}

int Event::getPriority() const {
    return m_priority;
}

void Event::setPriority( int priority ) {
    m_priority = priority;
}

bool EventPriorityCmp::operator()( EventPtr const& a, EventPtr const& b ) const {
    return a->getPriority() > b->getPriority();
}

bool EventPriorityCmp::operator()( Event const& a, Event const& b ) const {
    return a.getPriority() > b.getPriority();
}