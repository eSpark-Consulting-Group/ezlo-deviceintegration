#pragma once

#include "hardware_manager/button/event.h"
#include <string>
#include <cstdint>

enum class ButtonEventType : uint8_t { Pressed = 0, Released = 1, Unknown = 2 };

class ButtonEvent final : public Event {
public:
    static std::string eventTypeString( ButtonEventType eventType );

public:
    ButtonEvent( int priority );
    ButtonEvent( int priority, ButtonEventType type, std::string name, int secondsPassed );
    ButtonEventType getType() const;
    const std::string& getName() const;
    int getSecondsPassed() const;
    void setType( ButtonEventType type );
    void setName( const std::string& name );
    void setSecondsPassed( int secondsPassed );

private:
    ButtonEventType m_type;
    std::string m_name;
    int m_secondsPassed;
};