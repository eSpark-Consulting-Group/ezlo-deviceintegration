#pragma once

#include <string>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace HubPlatform {
namespace Zwave {

class BackupHelper {
public:
    static constexpr uint8_t MaxBackupStickInitializationAttemptsNumber = 10;

    BackupHelper() = default;

    void setFinished( std::string errorDescription = {} );

    bool onStickInitFailed();

    bool isFinished() const;

    const std::string& getErrorDescription() const;

    void waitForFinishing();

private:
    std::atomic_bool m_finished{ false };
    uint8_t m_stickInitializationAttemptsNumber = MaxBackupStickInitializationAttemptsNumber;
    std::string m_errorDescription;

    std::mutex m_mutex;
    std::condition_variable m_condVar;
};

}  // namespace Zwave
}  // namespace HubPlatform
