#pragma once

#include "hardware_manager/effect/effect.h"
#include <memory>

class EffectHandler {
public:
    EffectHandler( std::shared_ptr< Effect > effect, int priority ) noexcept;
    EffectHandler( const EffectHandler& obj ) = default;
    EffectHandler( EffectHandler&& obj ) noexcept = default;
    EffectHandler& operator=( const EffectHandler& obj ) = default;
    EffectHandler& operator=( EffectHandler&& obj ) noexcept = default;
    void operator()();
    void stop();
    bool operator>( const EffectHandler& a ) const;

private:
    std::shared_ptr< Effect > m_effect;
    int m_priority;
};
