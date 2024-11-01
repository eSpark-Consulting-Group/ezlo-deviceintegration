#pragma once

#include "networkd/gsm/nm_gsm_exceptions.h"
#include "networkd/utils/nm_generic_modem.h"
#include "json_utils/json_fwd.h"

#include <string>
#include <vector>
#include <tuple>
#include <memory>

namespace HubPlatform {
namespace Network {
namespace GSM {

struct ModemPPPConfiguration final {
    std::string dialNumber;
    std::string accessPointName;
};

/* TODO:
 *   . mobile operator selecting
 *   . authentication pass
 *   . wireless technology selecting */
class GSMModem : public GenericModem {
public:
    enum class State { On, Off, Unknown };

    using FrequencyBand = std::string;

    struct AccessTechnology {
        /*
         *  generation: 2g     2.5g    2.75g   3g             3.5g    3.75g    3.9g    4g
         *  technology: GSM -> GPRS -> EDGE -> UMTS(WCDMA) -> HSPA -> HSPA+ -> LTE  -> LTE Advance
         */
        enum class Enum { NONE, GSM, GPRS, EDGE, WCDMA, HSPA_UP, HSPA_DL, HSPA_PLUS, LTE_FDD, LTE_TDD, OTHER };

        AccessTechnology();
        AccessTechnology( const std::string& accessTechnology );

        std::string toString() const;
        bool operator!=( const AccessTechnology& rhs ) const;

        bool isLTE() const;
        bool isHSPA() const;

        Enum m_accessTechnology;
    };

    struct SIMInfo {
        SIMInfo();
        SIMInfo( std::string iccid, std::string imsi, bool simLocked );

        operator bool() const;
        bool operator!() const;

        std::string m_iccid;
        std::string m_imsi;
        bool m_isSIMLocked;
    };

    struct SignalQuality {
        SignalQuality();
        SignalQuality( const AccessTechnology& accessTechnology, int rssi );
        SignalQuality( const AccessTechnology& accessTechnology, int rssi, int rsrp, int rsrq );

        operator bool() const;
        bool operator!() const;

        AccessTechnology m_accessTechnology;

        int m_rssi;  // dBm (re 1mW)

        int m_rsrp;  // dBm (re 1mW)
        int m_rsrq;  // dB

        bool m_valid;
    };

    struct MobileNetworkInfo {
        MobileNetworkInfo() = default;
        MobileNetworkInfo( std::string fnn, std::string spn, std::string rplmn );
        Json toJson() const;

        operator bool() const;
        bool operator!() const;

        std::string m_fnn;    // Full Network Name
        std::string m_spn;    // Service Provider Name
        std::string m_rplmn;  // Registered Public Land Mobile Network
    };

    struct CRSMCommandResult {
        bool m_valid = false;

        int m_statusWord1 = -1;
        int m_statusWord2 = -1;
        std::string m_response;
    };

    explicit GSMModem( bool embeddedModem );
    virtual ~GSMModem() = default;

    virtual GSMModem::State getState() const = 0;
    virtual bool isAlive() const = 0;                  // whether a modem answers on requests or not
    virtual std::string getDataTTYDevice() const = 0;  // provide a backed tty device for data channel

    virtual std::string getGSMModemIMEI() const = 0;
    virtual void executeATInitializeString( const std::string& ATInitialize ) = 0;

    virtual bool isPINCodeRequired() const = 0;
    virtual bool isLocked() const = 0;
    virtual void setSIMLock( const std::string& pinCode, bool lock ) = 0;
    virtual void enterPINCode( const std::string& pinCode ) = 0;
    virtual void changeSIMPINCode( const std::string& currentPinCode, const std::string& newPinCode ) = 0;
    virtual void resetSIMPINCode( const std::string& pukCode, const std::string& newPinCode ) = 0;

    virtual SignalQuality getSignalQuality( const AccessTechnology& accessTechnology = AccessTechnology() ) const = 0;
    virtual AccessTechnology getAccessTechnology() const = 0;
    virtual FrequencyBand getFrequencyBand() const = 0;

    virtual void reboot() = 0;  // modems sometimes hang out, so we need some way to bring them back to life...
    virtual void powerOn() = 0;
    virtual void powerOff() = 0;
    virtual void startSession() = 0;

    using NetworkId = std::string;  // rplmn
    virtual SIMInfo getSIMInfo( bool realHwRequest = false ) const = 0;
    virtual MobileNetworkInfo getMobileNetworkInfo() const = 0;
    virtual NetworkId getCurrentNetworkId() const = 0;

    virtual bool isSIMInserted() const = 0;
    virtual void clearFPLMNList() = 0;
    virtual void reRegister( bool clearFPLMN = false ) = 0;
    virtual bool activatePDPContext( int contexIndex ) = 0;
    virtual bool deactivatePDPContext( int contexIndex ) = 0;
    virtual bool removePDPContext( int contexIndex ) = 0;
    virtual bool resetPDPContext( int contexIndex ) = 0;
    virtual std::vector< std::string > getPDPContexts() = 0;
    virtual void deregisterFromNetwork() = 0;
    virtual bool isRegistered() = 0;

    bool isEmbedded() const override final;
    virtual Json getDeviceInfo() const = 0;

    virtual void startScanNetworks() = 0;
    virtual std::string consumeScanNetworksResult() = 0;
    virtual void completeScanNetworks() = 0;

    virtual void startRegisterToNetwork( NetworkId networkId = "" ) = 0;
    virtual std::string consumeRegisterToNetworkResult() = 0;
    virtual void completeRegisterToNetwork() = 0;

    virtual CRSMCommandResult executeCRSMCommand( int command,
                                                  int fileId,
                                                  int parameter1,
                                                  int parameter2,
                                                  int parameter3 ) = 0;

private:
    bool m_embeddedModem;
};

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
