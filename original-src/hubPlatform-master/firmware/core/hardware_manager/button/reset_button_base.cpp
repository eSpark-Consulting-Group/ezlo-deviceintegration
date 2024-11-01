#include "hardware_manager/button/reset_button_base.h"

namespace {
constexpr const std::chrono::milliseconds ShortPressMaxDuration( 500 );
constexpr const std::chrono::milliseconds ShortPressesMaxPeriod( 1500 );
constexpr const std::chrono::milliseconds RebootMinDuration( 2000 );
constexpr const std::chrono::milliseconds RebootMaxDuration( 5000 );
constexpr const std::chrono::milliseconds SoftResetMinDuration( 5000 );
constexpr const std::chrono::milliseconds SoftResetMaxDuration( 10000 );
constexpr const std::chrono::milliseconds FactoryResetMinDuration( 10000 );
constexpr const std::chrono::milliseconds FactoryResetMaxDuration( 20000 );
constexpr const std::chrono::milliseconds CancelMinDuration( 20000 );
}  // namespace

ResetButtonBase::ResetButtonBase( ResetButtonBase::OnCombinationExecuted handler )
: m_handler( std::move( handler ) )
, m_shortPressesPeriod( 0 )
, m_shortPressCounter( 0 ) {}

void ResetButtonBase::processButton() {
    getButtonState().process();
    auto pulseDuration = getButtonState().pulseDuration();
    if( pulseDuration != ButtonState::emptyDuration() ) {
        if( CancelMinDuration <= pulseDuration ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::CANCEL );
            return;
        }

        if( ( FactoryResetMinDuration <= pulseDuration ) && ( pulseDuration < FactoryResetMaxDuration ) ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::FACTORY_RESET );
            return;
        }

        if( ( SoftResetMinDuration <= pulseDuration ) && ( pulseDuration < SoftResetMaxDuration ) ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::SOFT_RESET );
            return;
        }

        if( ( RebootMinDuration <= pulseDuration ) && ( pulseDuration < RebootMaxDuration ) ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::REBOOT );
            return;
        }

        if( pulseDuration < ShortPressMaxDuration ) {
            if( m_shortPressCounter == 0 ) {
                m_shortPressesPeriod = pulseDuration;
            }
            ++m_shortPressCounter;
        }
    }
    else {
        auto holdDuration = getButtonState().holdDuration();
        if( holdDuration != ButtonState::emptyDuration() ) {
            if( CancelMinDuration <= holdDuration ) {
                m_handler( PressingPicture::CANCEL );
                return;
            }

            if( ( FactoryResetMinDuration <= holdDuration ) && ( holdDuration < FactoryResetMaxDuration ) ) {
                m_handler( PressingPicture::FACTORY_RESET_READY );
                return;
            }

            if( ( SoftResetMinDuration <= holdDuration ) && ( holdDuration < SoftResetMaxDuration ) ) {
                m_handler( PressingPicture::SOFT_RESET_READY );
                return;
            }

            if( ( RebootMinDuration <= holdDuration ) && ( holdDuration < RebootMaxDuration ) ) {
                m_handler( PressingPicture::REBOOT_READY );
                return;
            }
        }
    }

    if( m_shortPressCounter > 0 && m_shortPressesPeriod < ShortPressesMaxPeriod ) {
        m_shortPressesPeriod += ButtonCheckInterval;
    }
    else {
        if( m_shortPressCounter == 1 ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::FIND_HUB );
            m_handler( PressingPicture::SAVE_LOGS );
            return;
        }
        if( m_shortPressCounter == 2 ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::WIFI_RESET );
            return;
        }
        if( m_shortPressCounter == 3 ) {
            m_shortPressCounter = 0;
            m_handler( PressingPicture::POWEROFF );
            return;
        }

        m_shortPressCounter = 0;
    }
}
