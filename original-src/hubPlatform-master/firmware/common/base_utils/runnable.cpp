
#include "base_utils/runnable.h"

namespace common {

Runnable::Runnable()
: m_isRunning( false ) {}

bool Runnable::start() {
    if( !isRunning() ) {
        m_isRunning = true;
        startInternal();
    }

    return m_isRunning;
}

bool Runnable::stop() {
    if( isRunning() ) {
        m_isRunning = false;
        stopInternal();
    }

    return m_isRunning;
}

bool Runnable::isRunning() const {
    return m_isRunning;
}

void Runnable::startInternal() {}

void Runnable::stopInternal() {}
Runnable::~Runnable() = default;

}  // namespace common
