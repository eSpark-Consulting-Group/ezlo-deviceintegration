#include "hardware_manager/effect/effect_handler.h"

EffectHandler::EffectHandler( std::shared_ptr< Effect > effect, int priority ) noexcept
: m_effect( std::move( effect ) )
, m_priority( priority ) {}

void EffectHandler::operator()() {
    m_effect->start();
}

void EffectHandler::stop() {
    m_effect->stop();
}

bool EffectHandler::operator>( const EffectHandler& a ) const {
    return m_priority > a.m_priority;
}
