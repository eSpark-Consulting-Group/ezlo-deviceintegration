#pragma once

#include <string>

namespace HubPlatform {
namespace Network {
namespace GSM {

class IATExecutor;

namespace UT {
class SIMLockTestSuite;
}

/* represents a SIM Lock functionality (PIN security functionality)
 * NOTE: changing SIM Lock state requires modem reboot, this is handled automatically */
class SIMLock final {
    friend class UT::SIMLockTestSuite;

public:
    SIMLock( IATExecutor& atExecutor, const std::string& ownerName = "" );

    bool isPinCodeRequired() const;
    bool isLocked() const;

    void lock( const std::string& pinCode );
    void unlock( const std::string& pinCode );
    void enterPinCode( const std::string& pinCode );
    void changePinCode( const std::string& currentPinCode, const std::string& newPinCode );
    void resetPinCode( const std::string& pukCode, const std::string& newPinCode );

private:
    enum class PinCodeStatus { ERROR, READY, SIM_PIN, SIM_PUK };
    enum class LockStatus { ERROR, UNLOCKED, LOCKED };

    bool checkPINCode( const std::string& pin ) const;
    PinCodeStatus getPinCodeStatus() const;
    LockStatus getLockStatus() const;

private:
    IATExecutor& m_ATExecutor;
    const std::string m_ownerName;
};

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
