#include "notifications/notifications_manager/notifications_manager.h"

#include "notifications/database_settings/database_settings.h"
#include "notifications/resources/notifications_resources.h"
#include "notifications/exceptions.h"

#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "system_utils/environment.h"
#include "common_utils/exception_utils.h"
#include "common_utils/cpp14.h"
#include "base_utils/utils.h"
#include "base_utils/string_utils.h"
#include "config_reader/config_reader.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <fstream>

namespace HubPlatform {
namespace Notifications {
namespace {

const char* const SMTPServerCACertificateName = "smtp_server_ca.crt";
const char* const SMTPClientCertificateName = "smtp_client.crt";
const char* const SMTPClientCertificateKeyName = "smtp_client_key.crt";

std::unique_ptr< SMTPClient > defaultSMTPClientFactory( SMTPClient::Settings settings ) {
    return std::make_unique< SMTPClient >( std::move( settings ) );
}

void createCertificateFile( const std::string& path, const std::string& content ) {
    std::ofstream file( path, std::ios::out );
    if( !file.is_open() )
        throw NotificationsManagerError( "Unable to create sertificate file: " + path );
    file << content;
}

}  // namespace

NotificationsManager::NotificationsManager( Settings settings, IDbClient& dbClient )
: NotificationsManager( std::move( settings ), dbClient, defaultSMTPClientFactory ){};

NotificationsManager::NotificationsManager( Settings settings,
                                            IDbClient& dbClient,
                                            SMTPClientFactory smtpClientFactory )
: m_settings( std::move( settings ) )
, m_dbClient( dbClient )
, m_smtpClientFactory( std::move( smtpClientFactory ) ) {
    if( m_settings.certificatesDir.empty() )
        m_settings.certificatesDir = Environment::getCertificateFolder();

    printManagerSettings();

    initSMTPClient();
    m_taskLoopThread = std::thread( [this]() { m_taskLoop.run(); } );
    waitForCondition(
        [this]() { return m_taskLoop.isRunning(); }, std::chrono::seconds( 5 ), std::chrono::milliseconds( 50 ) );
}

NotificationsManager::~NotificationsManager() {
    m_taskLoop.stop();
    if( m_taskLoopThread.joinable() )
        m_taskLoopThread.join();
}

void NotificationsManager::updateSMTPSettings( const Json& settings ) {
    try {
        DatabaseSettings dbSettings( m_dbClient );
        LOGGER_INFO( cc::info( "Updating SMTP settings" ) );
        dbSettings.setSMTPSettings( settings );
        LOGGER_DEBUG( cc::debug( "SMTP database settings updated: " ) + cc::j( dbSettings.getHashedSMTPSettings() ) );
        initSMTPClient();
        rescheduleSmtpTasks();
    }
    catch( const SMTPClientSettingsError& e ) {
        LOGGER_ERROR( cc::error( "Exception while updating SMTP settings: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
        throw SettingsError( e.what() );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Exception while updating SMTP settings: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
        throw;
    }
}

void NotificationsManager::initSMTPClient() {
    try {
        namespace Fields = Resources::DB::Settings::FieldsSMTP;

        SMTPLocker smtpLocker( m_smtpClientMutex );
        m_smtpClient.reset();
        smtpLocker.unlock();

        auto dbSettings = DatabaseSettings( m_dbClient ).getSMTPSettings();
        if( dbSettings.empty() ) {
            LOGGER_WARN( cc::warn( "SMTP settings are empty, client is not created" ) );
            return;
        }

        auto& settings = m_smtpClientSettings;
        settings = SMTPClient::Settings();
        settings.smtpServerUrl = json_get_str( dbSettings, Fields::SMTPServerUrl );
        settings.encryption = json_get_bool( dbSettings, Fields::Encryption, settings.encryption );
        settings.verifyServerCertificateOrigin =
            json_get_bool( dbSettings, Fields::VerifyServerCertificate, settings.verifyServerCertificateOrigin );
        settings.verifyServerHostname =
            json_get_bool( dbSettings, Fields::VerifyServerHostname, settings.verifyServerHostname );
        settings.clientUsername = json_get_str( dbSettings, Fields::ClientUsername );
        settings.clientPassword = json_get_str( dbSettings, Fields::ClientPassword );

        // NOTE: On newer versions of curl it is possible to use in-memory certificates (without files)

        auto serverCACertificate = json_get_str( dbSettings, Fields::ServerCACertificate );
        if( !serverCACertificate.empty() ) {
            settings.serverCertificateAuthorityFile = m_settings.certificatesDir + "/" + SMTPServerCACertificateName;
            createCertificateFile( settings.serverCertificateAuthorityFile, serverCACertificate );
        }

        auto clientCertificate = json_get_str( dbSettings, Fields::ClientCertificate );
        if( !clientCertificate.empty() ) {
            settings.clientCertificateFile = m_settings.certificatesDir + "/" + SMTPClientCertificateName;
            createCertificateFile( settings.clientCertificateFile, clientCertificate );
        }

        auto clientCertificateKey = json_get_str( dbSettings, Fields::ClientCertificateKey );
        if( !clientCertificateKey.empty() ) {
            settings.clientCertificateKeyFile = m_settings.certificatesDir + "/" + SMTPClientCertificateKeyName;
            createCertificateFile( settings.clientCertificateKeyFile, clientCertificateKey );
        }

        settings.timeout = m_settings.smtpSendTimeout;

        smtpLocker.lock();
        m_smtpClient = m_smtpClientFactory( settings );
        smtpLocker.unlock();
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Exception while creating SMTP client: " ) + cc::note( CommonUtils::whatError( e ) ) );
        SMTPLocker smtpLocker( m_smtpClientMutex );
        m_smtpClient.reset();
    }
}

void NotificationsManager::sendEmail( const std::string& sender,
                                      const std::string& recipient,
                                      const std::string& topic,
                                      const std::string& body,
                                      const std::time_t date ) {
    TaskLocker taskLocker( m_tasksMutex );
    if( m_smtpTasks.size() >= m_settings.smtpSendPendingLimit ) {
        LOGGER_WARN( cc::warn( "Reached SMTP tasks pending limit. Dropping oldest pending emails" ) );
        auto it = m_smtpTasks.begin();
        auto& task = it->second;
        if( task->taskPtr )
            task->taskPtr->stop();
        m_smtpTasks.erase( it );
        m_metrics.emailsPending--;
        m_metrics.emailsDropped++;
        m_metrics.lastEmailDropped = std::time( nullptr );
    }

    auto taskId = generateTaskId();
    auto it = m_smtpTasks.emplace( taskId, std::make_shared< TaskSMTP >() );
    auto task = it.first->second;

    m_metrics.emailsTotal++;
    m_metrics.emailsPending++;

    task->taskId = taskId;
    task->data.sender = sender;
    task->data.recipient = recipient;
    task->data.topic = topic;
    task->data.body = body;
    task->data.date = date;

    std::weak_ptr< TaskSMTP > taskWeakPtr = task;

    task->taskPtr = m_taskLoop.createPostponedTask(
        [this, taskWeakPtr]( Services::PostponedTask& ) { sendEmailImpl( taskWeakPtr ); } );
    task->taskPtr->start( std::chrono::milliseconds( 0 ), m_settings.smtpSendInterval );
}

void NotificationsManager::sendEmailImpl( std::weak_ptr< TaskSMTP > taskWeakPtr ) {
    SMTPLocker smtpLocker( m_smtpClientMutex );
    if( !m_smtpClient ) {
        LOGGER_WARN( cc::warn( "Unable to send email: SMTP client is not initialized" ) );
        return;
    }

    TaskLocker taskLocker( m_tasksMutex );
    auto task = taskWeakPtr.lock();

    if( !task ) {
        LOGGER_ERROR( cc::error( "Send email task unexpectedly removed" ) );
        return;
    }

    taskLocker.unlock();

    auto& data = task->data;
    auto result = m_smtpClient->send( data.sender, data.recipient, data.topic, data.body, data.date );

    taskLocker.lock();

    // NOTE: Task might be already removed is pending limit was reached
    if( task.use_count() == 1 ) {
        return;
    }

    if( !result ) {
        LOGGER_WARN( cc::warn( "Send email failed." ) );

        if( ++task->attempts > m_settings.smtpSendRetryLimit ) {
            LOGGER_WARN( cc::warn( "Email send retry limit reached" ) );
            if( task->taskPtr )
                task->taskPtr->stop();

            m_smtpTasks.erase( task->taskId );
            m_metrics.emailsPending--;
            m_metrics.emailsFailed++;
            m_metrics.lastEmailFailed = std::time( nullptr );
        }

        return;
    }

    if( task->taskPtr )
        task->taskPtr->stop();
    m_smtpTasks.erase( task->taskId );
    m_metrics.emailsPending--;
    m_metrics.emailsSent++;
    m_metrics.lastEmailSent = std::time( nullptr );
}

void NotificationsManager::rescheduleSmtpTasks() {
    TaskLocker locker( m_tasksMutex );
    for( auto& element : m_smtpTasks ) {
        auto& task = element.second;
        if( task->taskPtr ) {
            task->taskPtr->start( std::chrono::milliseconds( 0 ), m_settings.smtpSendInterval );
        }
    }
}

size_t NotificationsManager::generateTaskId() {
    static size_t id{ 0 };
    return ++id;
}

Json NotificationsManager::dumpMetrics() const {
    TaskLocker locker( m_tasksMutex );
    Json j;

    Json metrics;
    metrics[ "total" ] = m_metrics.emailsTotal;
    metrics[ "sent" ] = m_metrics.emailsSent;
    metrics[ "failed" ] = m_metrics.emailsFailed;
    metrics[ "dropped" ] = m_metrics.emailsDropped;
    metrics[ "pending" ] = m_metrics.emailsPending;
    metrics[ "lastSent" ] = int( m_metrics.lastEmailSent );
    metrics[ "lastFailed" ] = int( m_metrics.lastEmailFailed );
    metrics[ "lastDropped" ] = int( m_metrics.lastEmailDropped );

    j[ "emails" ] = metrics;
    return j;
}

NotificationsManager::NotificationMetrics NotificationsManager::getMetrics() const {
    TaskLocker locker( m_tasksMutex );
    return m_metrics;
}

SMTPClient::Settings NotificationsManager::getSMTPClientSettings() {
    SMTPLocker smtpLocker( m_smtpClientMutex );
    return m_smtpClientSettings;
}

NotificationsManager::Settings NotificationsManager::loadSettings() {
    ConfigReader configReader;
    auto readInt = [&configReader]( const std::string& path, int defaultValue ) -> int {
        try {
            return configReader.getInt( path.c_str() );
        }
        catch( const std::exception& e ) {
            LOGGER_WARN( cc::warn( "Unable to read config value: " ) + cc::note( path ) + cc::warn( ", error: " ) +
                         cc::note( CommonUtils::whatError( e ) ) );
        }
        return defaultValue;
    };

    namespace Keys = Resources::ConfigKeys;

    Settings settings;
    settings.smtpSendRetryLimit = readInt( Keys::NotificationsSmtpSendRetryLimit, settings.smtpSendRetryLimit );
    settings.smtpSendPendingLimit = readInt( Keys::NotificationsSmtpSendPendingLimit, settings.smtpSendPendingLimit );

    int defaultSmtpInterval = std::chrono::duration_cast< std::chrono::seconds >( settings.smtpSendInterval ).count();
    settings.smtpSendInterval =
        std::chrono::seconds( readInt( Keys::NotificationsSmtpSendInterval, defaultSmtpInterval ) );

    int defaultSmtpTimeout = std::chrono::duration_cast< std::chrono::seconds >( settings.smtpSendTimeout ).count();
    settings.smtpSendTimeout =
        std::chrono::seconds( readInt( Keys::NotificationsSmtpSendTimeout, defaultSmtpTimeout ) );

    return settings;
}

void NotificationsManager::printManagerSettings() {
    namespace Keys = Resources::ConfigKeys;

    auto key = []( const std::string& path ) { return StringUtils::split( path, "/" ).back(); };

    std::stringstream ss;
    ss << "NotificationsManager settings\n";
    ss << key( Keys::NotificationsSmtpSendRetryLimit ) << " " << m_settings.smtpSendRetryLimit << "\n";
    ss << key( Keys::NotificationsSmtpSendPendingLimit ) << " " << m_settings.smtpSendPendingLimit << "\n";
    ss << key( Keys::NotificationsSmtpSendInterval ) << " " << m_settings.smtpSendInterval.count() << " ms\n";
    ss << key( Keys::NotificationsSmtpSendTimeout ) << " " << m_settings.smtpSendTimeout.count() << " ms\n";
    ss << "certificatesDir"
       << " " << m_settings.certificatesDir;

    LOGGER_INFO_RAW( cc::info( ss.str() ) );
}

}  // namespace Notifications
}  // namespace HubPlatform
