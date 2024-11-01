#pragma once

#include "json_utils/json_fwd.h"
#include "smtp_client/smtp_client.h"
#include "service_interfaces/task_loop.h"

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <map>
#include <chrono>
#include <atomic>

class IDbClient;

namespace HubPlatform {
namespace Notifications {

// TODO add network monitor

class NotificationsManager {
public:
    struct Settings {
        size_t smtpSendRetryLimit{ 1 };
        size_t smtpSendPendingLimit{ 200 };
        std::chrono::milliseconds smtpSendInterval{ 300000 };
        std::chrono::milliseconds smtpSendTimeout{ 10000 };

        std::string certificatesDir;
    };

    struct NotificationMetrics {
        size_t emailsTotal{ 0 };
        size_t emailsPending{ 0 };
        size_t emailsSent{ 0 };
        size_t emailsFailed{ 0 };
        size_t emailsDropped{ 0 };
        std::time_t lastEmailSent{ 0 };
        std::time_t lastEmailFailed{ 0 };
        std::time_t lastEmailDropped{ 0 };
    };

    using SMTPClientFactory = std::function< std::unique_ptr< SMTPClientInterface >( SMTPClient::Settings ) >;

    NotificationsManager( Settings settings, IDbClient& dbClient );
    NotificationsManager( Settings settings, IDbClient& dbClient, SMTPClientFactory smtpClientFactory );
    ~NotificationsManager();

    void updateSMTPSettings( const Json& settings );

    void sendEmail( const std::string& sender,
                    const std::string& recipient,
                    const std::string& topic,
                    const std::string& body,
                    const std::time_t date );

    NotificationMetrics getMetrics() const;
    Json dumpMetrics() const;

    SMTPClient::Settings getSMTPClientSettings();

    static Settings loadSettings();

private:
    using TaskId = size_t;

    struct DataSMTP {
        std::string sender;
        std::string recipient;
        std::string topic;
        std::string body;
        std::time_t date;
    };

    struct TaskSMTP {
        TaskId taskId;
        size_t attempts = 0;
        HubPlatform::Services::PostponedTaskPtr taskPtr;
        DataSMTP data;
    };

    using SMTPMutex = std::mutex;
    using SMTPLocker = std::unique_lock< SMTPMutex >;

    using TaskMutex = std::mutex;
    using TaskLocker = std::unique_lock< TaskMutex >;

private:
    void initSMTPClient();
    TaskId generateTaskId();
    void rescheduleSmtpTasks();
    void printManagerSettings();

    void sendEmailImpl( std::weak_ptr< TaskSMTP > taskWeakPtr );

private:
    Settings m_settings;
    SMTPClient::Settings m_smtpClientSettings;
    IDbClient& m_dbClient;

    SMTPClientFactory m_smtpClientFactory;
    mutable SMTPMutex m_smtpClientMutex;
    std::unique_ptr< SMTPClientInterface > m_smtpClient;

    mutable TaskMutex m_tasksMutex;
    std::map< TaskId, std::shared_ptr< TaskSMTP > > m_smtpTasks;
    std::thread m_taskLoopThread;
    HubPlatform::Services::TaskLoop m_taskLoop;

    NotificationMetrics m_metrics;
};

}  // namespace Notifications
}  // namespace HubPlatform
