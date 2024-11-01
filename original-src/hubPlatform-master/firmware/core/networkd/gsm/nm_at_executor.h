#pragma once

#include "base_utils/uart/uart.h"
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <mutex>

namespace HubPlatform {
namespace Network {
namespace GSM {

struct SerialSettings final {
    std::string controlTTY;
    std::string dataTTY;
    std::string speed;
    bool hwFlowControl;
};

class IATExecutor {
public:
    virtual ~IATExecutor();

    virtual std::string execute(
        const std::string& command,
        const std::chrono::milliseconds& waitForAnswer = std::chrono::milliseconds( 300 ) ) = 0;
    virtual bool isInitialized() const = 0;

    virtual bool lock() = 0;
    virtual void unlock() = 0;
    virtual bool isAccessible() const = 0;
};

class ATExecutor final : public IATExecutor {
public:
    ATExecutor() = default;
    explicit ATExecutor( SerialSettings serialSettings );
    ~ATExecutor() override;

    void setSerialSettings( SerialSettings serialSettings );

    // lock/unlock to perform async operations
    bool lock() override;
    void unlock() override;
    bool isAccessible() const override;

    std::string execute( const std::string& command,
                         const std::chrono::milliseconds& waitForAnswer = std::chrono::milliseconds( 300 ) ) override;
    std::string consume();

    bool isInitialized() const override;

private:
    mutable std::timed_mutex m_mutex;
    SerialSettings m_serialSettings;
    bool m_isInitialized = false;
    bool m_locked = false;
    std::chrono::seconds m_maxChannelLockDuration = std::chrono::minutes( 5 );
};

namespace ATUtils {

std::vector< std::string > toStrings( const std::string& str );
bool isOK( const std::vector< std::string >& answer, size_t expectedSize );

/** Valid ICCID should contain HEX digits only; should start from "89" prefix. */
bool isValidICCID( const std::string& value );

namespace AtResponse {

/** Extracts AT command parameter value from a response.
 * @param atAnswer Answer for the AT Command
 * @param key A key to return value for.
 * */
std::pair< std::string, bool > extractATParameter( const std::vector< std::string >& atAnswer, const std::string& key );

/** Extracts AT command parameter value from a response.
 * @param atAnswer Answer for the AT Command
 * @param key A key to return value for. */
std::pair< std::string, bool > extractATParameter( const std::string& atAnswer, const std::string& key );

}  // namespace AtResponse

}  // namespace ATUtils

}  // namespace GSM
}  // namespace Network
}  // namespace HubPlatform
