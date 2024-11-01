#include "zwave/zwave_backup_helper.h"

#include "log_utils/ezlo_log.h"

#include <thread>

namespace HubPlatform {
namespace Zwave {

void BackupHelper::setFinished( std::string errorDescription ) {
    m_errorDescription = std::move( errorDescription );

    if( !m_errorDescription.empty() ) {
        LOGGER_ERROR( m_errorDescription );
    }

    {
        std::lock_guard< std::mutex > guard( m_mutex );
        m_finished = true;
    }

    m_condVar.notify_one();
}

bool BackupHelper::onStickInitFailed() {
    if( m_finished ) {
        return false;
    }

    LOGGER_ERROR( "Cannot initialize stick" );

    if( m_stickInitializationAttemptsNumber == 0 ) {
        setFinished( "Zwave stick initialization failed" );
    }
    else {
        --m_stickInitializationAttemptsNumber;
    }
    return !m_finished;
}

bool BackupHelper::isFinished() const {
    return m_finished;
}

const std::string& BackupHelper::getErrorDescription() const {
    return m_errorDescription;
}

void BackupHelper::waitForFinishing() {
    std::unique_lock< std::mutex > lock( m_mutex );
    m_condVar.wait( lock, [&]() { return static_cast< bool >( m_finished ); } );
}

}  // namespace Zwave
}  // namespace HubPlatform
