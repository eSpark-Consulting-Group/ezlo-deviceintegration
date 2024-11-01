#pragma once

#include "base_utils/uart/uart.h"
#include "system_utils/environment.h"
#include "zigbee_flasher/xmodem_sender.h"
#include "database_client/idbclient.h"

#include <signal.h>
#include <memory>

namespace HubPlatform {
namespace ZigbeeFlasher {

class ZigbeeFlasher {
public:
    enum Result { UNDEFINED = -1, SUCCESS = 0, FAILURE, BOOTPROMPT_MISSING };

public:
    ZigbeeFlasher() = delete;
    explicit ZigbeeFlasher( const volatile sig_atomic_t& exitSignalRef );
    ~ZigbeeFlasher() = default;

public:
    int setUp( int argc, char* argv[] );
    int run();

private:
    int updateFirmware();
    void enterBootMode();
    void leaveBootMode();
    bool waitForBootloaderPrompt();
    void sendLoadFirmwareOption();

private:
    bool validateDbRecord( const Json& record );
    bool checkUpdateNeeded( const Json& record );
    void updateDbRecord();
    std::string calculateFileSha256Sum( const std::string& filepath ) const;

private:
    const volatile sig_atomic_t& m_exitSignalRef;
    Uart m_uart;
    std::string m_firmwareFilepath;
    XmodemSender m_xmodemSender;
    std::unique_ptr< IDbClient > m_dbClient;
};

}  // namespace ZigbeeFlasher
}  // namespace HubPlatform