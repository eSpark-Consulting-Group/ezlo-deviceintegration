#include "networkd/gsm/ec25/nm_ec25_sim_lock.h"
#include "networkd/gsm/ec25/nm_ec25.h"

#include "log_utils/ezlo_log.h"
#include "tiny/include/tiny/tiny_consolecolors.h"

#include <cstring>

namespace HubPlatform {
namespace Network {
namespace GSM {

SIMLock::SIMLock( IATExecutor& atExecutor, const std::string& ownerName )
: m_ATExecutor( atExecutor )
, m_ownerName( ownerName ) {}

bool SIMLock::isPinCodeRequired() const {
    if( getPinCodeStatus() == PinCodeStatus::READY ) {
        return false;
    }

    return true;
}

bool SIMLock::isLocked() const {
    return getLockStatus() == LockStatus::LOCKED;
}

void SIMLock::lock( const std::string& pinCode ) {
    if( !checkPINCode( pinCode ) ) {
        throw GSMModemInvalidPINCode();
    }

    if( getLockStatus() == LockStatus::UNLOCKED ) {
        const auto cmd = "at+clck=\"SC\",1,\"" + pinCode + "\"\r";

        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( cmd, std::chrono::milliseconds( 5000 ) ) );

        LOGGER_WARN_RAW( cc::warn( "Reboot modem to apply changes for sim lock, interface: " ) +
                         cc::note( m_ownerName ) );
        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( "at+cfun=1,1\r", std::chrono::milliseconds( 5000 ) ) );

        LOGGER_DEBUG_RAW( cc::debug( "SIM PIN lock was turned on, interface: " ) + cc::note( m_ownerName ) );

        return;
    }

    throw GSM::GSMOperationNotAllowed( "SIM lock is already on" );
}

void SIMLock::unlock( const std::string& pinCode ) {
    if( !checkPINCode( pinCode ) ) {
        throw GSMModemInvalidPINCode();
    }

    if( getLockStatus() == LockStatus::LOCKED ) {
        const auto cmd = "at+clck=\"SC\",0,\"" + pinCode + "\"\r";

        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( cmd, std::chrono::milliseconds( 5000 ) ) );

        LOGGER_WARN_RAW( cc::warn( "Reboot modem to apply changes for sim lock, interface: " ) +
                         cc::note( m_ownerName ) );
        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( "at+cfun=1,1\r", std::chrono::milliseconds( 5000 ) ) );

        LOGGER_DEBUG_RAW( cc::debug( "SIM PIN lock was turned off, interface: " ) + cc::note( m_ownerName ) );

        return;
    }

    throw GSM::GSMOperationNotAllowed( "SIM lock is already off" );
}

void SIMLock::enterPinCode( const std::string& pinCode ) {
    if( !checkPINCode( pinCode ) ) {
        throw GSMModemInvalidPINCode();
    }

    if( getLockStatus() == LockStatus::LOCKED && getPinCodeStatus() == PinCodeStatus::SIM_PIN ) {
        const auto cmd = "at+cpin=\"" + pinCode + "\"\r";

        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( cmd, std::chrono::milliseconds( 5000 ) ) );
        return;
    }

    throw GSM::GSMOperationNotAllowed(
        "SIM PIN code can be entered only when a SIM lock is on and a PIN code is required" );
}

void SIMLock::changePinCode( const std::string& currentPinCode, const std::string& newPinCode ) {
    if( !checkPINCode( currentPinCode ) || !checkPINCode( newPinCode ) ) {
        throw GSMModemInvalidPINCode();
    }

    if( getLockStatus() == LockStatus::LOCKED ) {
        if( getPinCodeStatus() != PinCodeStatus::READY ) {
            enterPinCode( currentPinCode );
        }

        const auto cmd = "at+cpwd=\"SC\",\"" + currentPinCode + "\",\"" + newPinCode + "\"\r";

        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( cmd, std::chrono::milliseconds( 5000 ) ) );
        LOGGER_DEBUG_RAW( cc::debug( "SIM PIN code was changed, interface: " ) + cc::note( m_ownerName ) );

        return;
    }

    throw GSM::GSMOperationNotAllowed( "SIM PIN code can be changed only when a SIM lock is on" );
}

void SIMLock::resetPinCode( const std::string& pukCode, const std::string& newPinCode ) {
    if( !checkPINCode( pukCode ) || !checkPINCode( newPinCode ) ) {
        throw GSMModemInvalidPINCode();
    }

    if( getLockStatus() == LockStatus::LOCKED && getPinCodeStatus() == PinCodeStatus::SIM_PUK ) {
        /* leave a last shot for a user to reset a SIM PIN code outside hubs */
        const auto answer = m_ATExecutor.execute( "at+qpinc?\r", std::chrono::milliseconds( 5000 ) );

        auto commaPositon = answer.find( "," );
        if( commaPositon != std::string::npos ) {
            auto position = answer.find( ",", commaPositon + 1 );
            auto pukAttemts = std::stoi( answer.substr( position + 1 ) );

            if( pukAttemts <= 1 ) {
                throw GSMModemPUKCodeLastAttempt();
            }
        }

        EC25::checkATAnswerAndThrow( answer );

        const auto cmd = "at+cpin=\"" + pukCode + "\",\"" + newPinCode + "\"\r";
        EC25::checkATAnswerAndThrow( m_ATExecutor.execute( cmd, std::chrono::milliseconds( 5000 ) ) );
        LOGGER_DEBUG_RAW( cc::debug( "SIM PIN code was reseted, interface: " ) + cc::note( m_ownerName ) );

        return;
    }

    throw GSM::GSMOperationNotAllowed(
        "SIM PIN code can be reset only when a SIM lock is on and a PUK code is required" );
}

// GSM SIM PIN/PUK code can contain 4-8 digits
bool SIMLock::checkPINCode( const std::string& pin ) const {
    if( pin.size() < 4 || pin.size() > 8 || !std::all_of( pin.begin(), pin.end(), ::isdigit ) ) {
        return false;
    }

    return true;
}

SIMLock::PinCodeStatus SIMLock::getPinCodeStatus() const {
    const auto answer = m_ATExecutor.execute( "at+cpin?\r", std::chrono::milliseconds( 5000 ) );

    if( answer.find( "+CPIN: READY" ) != std::string::npos ) {
        return PinCodeStatus::READY;
    }
    if( answer.find( "+CPIN: SIM PIN" ) != std::string::npos ) {
        return PinCodeStatus::SIM_PIN;
    }
    if( answer.find( "+CPIN: SIM PUK" ) != std::string::npos ) {
        return PinCodeStatus::SIM_PUK;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Failed to get SIM PIN code status, interface: " ) + cc::note( m_ownerName ) );
    return PinCodeStatus::ERROR;
}

SIMLock::LockStatus SIMLock::getLockStatus() const {
    const auto answer = m_ATExecutor.execute( "at+clck=\"SC\",2\r", std::chrono::milliseconds( 5000 ) );

    if( answer.find( "+CLCK: 1" ) != std::string::npos ) {
        return LockStatus::LOCKED;
    }
    if( answer.find( "+CLCK: 0" ) != std::string::npos ) {
        return LockStatus::UNLOCKED;
    }

    LOGGER_DEBUG_RAW( cc::debug( "Failed to get SIM lock status, interface: " ) + cc::note( m_ownerName ) );
    return LockStatus::ERROR;
}

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
