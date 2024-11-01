#pragma once

#include <memory>

class Event {
public:
    Event( int priority );
    virtual ~Event() = default;
    int getPriority() const;
    void setPriority( int priority );

private:
    int m_priority;
};

using EventPtr = std::shared_ptr< Event >;

class EventPriorityCmp {
public:
    bool operator()( EventPtr const& a, EventPtr const& b ) const;
    bool operator()( Event const& a, Event const& b ) const;
};