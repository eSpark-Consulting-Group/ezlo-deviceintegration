#pragma once

#include <atomic>
#include <string>

enum class BlinkingType { On, Blink, Breathe, None };

enum class Color { Red, Green, Blue, Yellow, Orange, Cyan, Magenta, White, None };

class Effect {
public:
    Effect();
    virtual ~Effect() = default;
    void start();
    void stop();
    static BlinkingType blinkingTypeFromString( const std::string& type );
    static Color colorFromString( const std::string& type );

protected:
    bool isRunning() const;
    virtual void effect() = 0;

private:
    std::atomic_bool m_shouldBeStopped;
    std::atomic_bool m_stopRequestHandled;
};
