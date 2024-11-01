#pragma once

#include <string>
#include <stdexcept>

namespace HubPlatform {
namespace Network {
namespace GSM {

class GSMModemException : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class GSMModemFailedToRunATInitializeString final : public GSMModemException {
public:
    GSMModemFailedToRunATInitializeString()
    : GSMModemException( description ) {}
    explicit GSMModemFailedToRunATInitializeString( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Failed to execute an AT initialize string: " );
};

class GSMModemHardwareFault final : public GSMModemException {
public:
    GSMModemHardwareFault()
    : GSMModemException( description ) {}
    explicit GSMModemHardwareFault( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Hardware fault: " );
};

class GSMModemNoSIMCard final : public GSMModemException {
public:
    GSMModemNoSIMCard()
    : GSMModemException( description ) {}
    explicit GSMModemNoSIMCard( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "SIM card no present: " );
};

class GSMModemSIMCardError final : public GSMModemException {
public:
    GSMModemSIMCardError()
    : GSMModemException( description ) {}
    explicit GSMModemSIMCardError( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "SIM card error: " );
};

class GSMModemBadSIMCard final : public GSMModemException {
public:
    GSMModemBadSIMCard()
    : GSMModemException( description ) {}
    explicit GSMModemBadSIMCard( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Bad SIM card: " );
};

class GSMModemPINCodeRequired final : public GSMModemException {
public:
    GSMModemPINCodeRequired()
    : GSMModemException( description ) {}
    explicit GSMModemPINCodeRequired( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "PIN code is required: " );
};

class GSMModemPUKCodeIsRequired final : public GSMModemException {
public:
    GSMModemPUKCodeIsRequired()
    : GSMModemException( description ) {}
    explicit GSMModemPUKCodeIsRequired( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "PUK code is required: " );
};

class GSMModemPUKCodeLastAttempt final : public GSMModemException {
public:
    GSMModemPUKCodeLastAttempt()
    : GSMModemException( description ) {}
    explicit GSMModemPUKCodeLastAttempt( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description =
        (char*) ( "Only one attempt to enter a PUK code is left, firmware won't try to enter it: " );
};

class GSMModemInvalidPINCode final : public GSMModemException {
public:
    GSMModemInvalidPINCode()
    : GSMModemException( description ) {}
    explicit GSMModemInvalidPINCode( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Invalid PIN code: " );
};

class GSMModemNotSupportedOperator final : public GSMModemException {
public:
    GSMModemNotSupportedOperator()
    : GSMModemException( description ) {}
    explicit GSMModemNotSupportedOperator( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Not supported mobile operator: " );
};

class GSMModemNoNetworkService final : public GSMModemException {
public:
    GSMModemNoNetworkService()
    : GSMModemException( description ) {}
    explicit GSMModemNoNetworkService( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "No network service: " );
};

class GSMModemOnlyEmergencyCalls final : public GSMModemException {
public:
    GSMModemOnlyEmergencyCalls()
    : GSMModemException( description ) {}
    explicit GSMModemOnlyEmergencyCalls( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Emergency calls only: " );
};

class GSMModemBadPPPAuthentication final : public GSMModemException {
public:
    GSMModemBadPPPAuthentication()
    : GSMModemException( description ) {}
    explicit GSMModemBadPPPAuthentication( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Bad PPP authentication: " );
};

class GSMModemBadInternetParameters final : public GSMModemException {
public:
    GSMModemBadInternetParameters()
    : GSMModemException( description ) {}
    explicit GSMModemBadInternetParameters( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Bad PPP internet configuration parameters: " );
};

class GSMOperationNotAllowed final : public GSMModemException {
public:
    GSMOperationNotAllowed()
    : GSMModemException( description ) {}
    explicit GSMOperationNotAllowed( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Requested operation is not allowed: " );
};

class GSMModemHangs final : public GSMModemException {
public:
    GSMModemHangs()
    : GSMModemException( description ) {}
    explicit GSMModemHangs( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Modem doesn't respond: " );
};

class GSMModemHalting final : public GSMModemException {
public:
    GSMModemHalting()
    : GSMModemException( description ) {}
    explicit GSMModemHalting( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Modem halting: " );
};

class GSMModemRebooting final : public GSMModemException {
public:
    GSMModemRebooting()
    : GSMModemException( description ) {}
    explicit GSMModemRebooting( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Modem rebooting: " );
};

class GSMModemFileDoesNotExist final : public GSMModemException {
public:
    GSMModemFileDoesNotExist()
    : GSMModemException( description ) {}
    explicit GSMModemFileDoesNotExist( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Modem tty file does not exists: " );
};

class GSMModemUnknownReason final : public GSMModemException {
public:
    GSMModemUnknownReason()
    : GSMModemException( description ) {}
    explicit GSMModemUnknownReason( const std::string& message )
    : GSMModemException( description + message ) {}

private:
    static constexpr char* const description = (char*) ( "Unknown reason: " );
};

class ATExecutorException : public std::runtime_error {
public:
    explicit ATExecutorException( const std::string& message )
    : runtime_error( "ATExecutor: " + message ) {}
};

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
