#pragma once

#include "networkd/gsm/nm_gsm.h"
#include "networkd/gsm/nm_at_executor.h"
#include "networkd/gsm/ec25/nm_ec25_sim_lock.h"
#include "networkd/configuration/nm_config_file_manager.h"

#include <thread>
#include <atomic>

/* This file describes abstractions to work with Quectel EC25 LTE module */

namespace HubPlatform {
namespace Network {
namespace GSM {

class EC25 final : public GSMModem {
public:
    EC25( const NetworkConfigFileManager::MobileConfiguration& configuration,
          const SerialSettings& serialSettings,
          bool embeddedModem,
          bool linkedToPhysicalModem = true,
          const std::string& ownerName = "" );
    ~EC25() override;

    GSMModem::State getState() const override;
    bool isAlive() const override;
    std::string getDataTTYDevice() const override;

    std::string getGSMModemIMEI() const override;
    virtual Json getDeviceInfo() const override;
    void executeATInitializeString( const std::string& ATInitialize ) override;

    bool isPINCodeRequired() const override;
    bool isLocked() const override;
    void setSIMLock( const std::string& pinCode, bool lock ) override;
    void enterPINCode( const std::string& pinCode ) override;
    void changeSIMPINCode( const std::string& currentPinCode, const std::string& newPinCode ) override;
    void resetSIMPINCode( const std::string& pukCode, const std::string& newPinCode ) override;

    SignalQuality getSignalQuality( const AccessTechnology& accessTechnology = AccessTechnology() ) const override;
    AccessTechnology getAccessTechnology() const override;
    FrequencyBand getFrequencyBand() const override;

    void reboot() override;
    void powerOn() override;
    void powerOff() override;
    void startSession() override;

    SIMInfo getSIMInfo( bool force = false ) const override;
    MobileNetworkInfo getMobileNetworkInfo() const override;
    NetworkId getCurrentNetworkId() const override;
    bool isSIMInserted() const override;
    void clearFPLMNList() override;
    void reRegister( bool clearFPLMN = false ) override;
    bool activatePDPContext( int contextIndex ) override;
    bool deactivatePDPContext( int contextIndex ) override;
    bool removePDPContext( int contextIndex ) override;
    bool resetPDPContext( int contextIndex ) override;
    std::vector< std::string > getPDPContexts() override;
    bool isRegistered() override;

    static void checkATAnswerAndThrow( const std::string& ATAnswer );

    void startScanNetworks() override;
    std::string consumeScanNetworksResult() override;
    void completeScanNetworks() override;

    void deregisterFromNetwork() override;
    void startAutoRegistration();
    void startRegisterToNetwork( NetworkId networkId = "" ) override;
    std::string consumeRegisterToNetworkResult() override;
    void completeRegisterToNetwork() override;

    CRSMCommandResult executeCRSMCommand( int command,
                                          int fileId,
                                          int parameter1,
                                          int parameter2,
                                          int parameter3 ) override;
    std::string executeCustomATCommand( const std::string& ATCommand,
                                        const std::chrono::milliseconds& timeout ) override;

private:
    void retrieveIMEI();
    void executeATCommandOrThrow( const std::string& command,
                                  const std::string& modemHangsExceptionDescription,
                                  const std::chrono::milliseconds& waitForAnswer = std::chrono::milliseconds( 300 ) );

    void checkSIMCard();
    void setEchoMode( bool echoMode );
    bool checkPINCode( const std::string& pin ) const;
    void rebootExternalModem();

    enum class SimStatus : int { NotInserted = 0, Inserted = 1, Unknown = 2 };
    SimStatus getSimStatus();
    std::string simStatusStr( SimStatus status );
    std::string getSimICCID();

    int getPINCodeAmountsLeft() const;
    int getPUKCodeAmountsLeft() const;

    void setupModem();
    bool setupAvailableBands();
    bool disableIMSSupport();
    bool setupDataCentricMode();
    void disableVoiceSupport();

    SignalQuality getLTESignalQuality( const AccessTechnology& accessTechnology ) const;
    SignalQuality getNoLTESignalQuality( const AccessTechnology& accessTechnology ) const;

    void monitoringThread();

private:
    const NetworkConfigFileManager::MobileConfiguration& m_configuration;
    mutable ATExecutor m_ATExecutor;
    SerialSettings m_serialSettings;
    SIMLock m_simLock;
    mutable bool m_simInfoPresent{ false };
    mutable SIMInfo m_simInfo;
    bool m_simInserted;
    std::string m_IMEI;
    const std::string m_interfaceName;

    std::atomic_bool m_isMonitoringTreadUp;
    std::thread m_monitoringThread;
    std::atomic_bool m_isRebooting;
    static std::atomic_bool m_isRebootingEmbedded;
    static std::atomic_bool m_rebootAnywayEmbedded;
};

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
