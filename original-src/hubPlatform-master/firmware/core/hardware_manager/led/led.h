#pragma once
#include <string>
#include <cstdint>

enum class LedState : std::uint8_t { LedOn = 1, LedOff = 0, LedUnknown = 2, LedBlinking = 3 };

class Led {
public:
    Led( std::string name );
    virtual ~Led() = default;
    virtual bool turnOn() = 0;
    virtual bool turnOff() = 0;
    virtual bool turn( bool enable ) = 0;
    virtual bool turnBlinking( int msOff, int msOn ) = 0;
    LedState getState() const;
    bool isLocked() const;
    void lock();
    void unlock();
    const std::string& getName() const;

private:
    std::string m_name;
    bool m_locked;

protected:
    LedState state;
};
