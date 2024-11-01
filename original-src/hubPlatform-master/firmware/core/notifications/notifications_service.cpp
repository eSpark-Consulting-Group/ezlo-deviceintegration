#include "notifications/notifications_service.h"
#include "notifications/database_settings/database_settings.h"
#include "notifications/resources/notifications_resources.h"

#include "service_interfaces/bus_wrapper.h"
#include "system_utils/environment.h"
#include "common_utils/backup/backup_helper.h"
#include "common_utils/backup/restore_helper.h"
#include "common_utils/lock_helper.h"
#include "common_utils/cpp14.h"
#include "common_utils/exception_utils.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_wrapper.h"
#include "jsonrpc/jsonrpc.h"
#include "notifications/exceptions.h"

#include <string>

namespace HubPlatform {
namespace Notifications {

const char* const NotificationsService::Name = "notifications";
namespace {
const auto CurrentApiVersion = "1";
const auto DefaultCloudRequestTimeout = std::chrono::seconds( 30 );
const auto DefaultReaskOnErrorDelay = std::chrono::minutes( 10 );
const auto SubscriptionDelayInterval = std::chrono::seconds( 5 );
}  // namespace

NotificationsService::NotificationsService()
: Services::Service( Name ) {}

void NotificationsService::setUp() {
    Services::Service::setUp();

    auto managerSettings = NotificationsManager::loadSettings();
    m_notificationsManager = std::make_unique< NotificationsManager >( std::move( managerSettings ), getDBClient() );

    join( GroupsNames::NOTIFICATIONS );
}

void NotificationsService::tearDown() {
    Services::Service::tearDown();
    m_notificationsManager.reset();
}

JsonObject NotificationsService::handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    try {
        return doHandleRequestFromBus( request, requestType );
    }
    catch( const std::exception& ex ) {
        LOGGER_ERROR( "Uncaught exception: failed to handle event from bus: " + cc::j( request ) +
                      ", error: " + CommonUtils::whatError( ex ) );
        Json response;
        setError( response, Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR, ex.what() );
        return response;
    }
}

JsonObject NotificationsService::doHandleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) {
    switch( requestType ) {
    case GLOBAL_LOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.tryLock( request );
        return lockHelper.getResponse();
    }
    case GLOBAL_UNLOCK: {
        auto& lockHelper = CommonUtils::LockHelper::global();
        lockHelper.unlock( request );
        return lockHelper.getResponse();
    }
    case BACKUP_CREATE: {
        CommonUtils::BackupHelper helper( getDBClient(), request );
        for( auto& tableName : Resources::DB::BackupTableNames ) {
            if( !helper.backupDatabase( tableName ) ) {
                return helper.getResponse();
            }
        }
        return helper.getResponse();
    }
    case HUB_NOTIFICATIONS_SMTP_SETTINGS_SET:
        return handleSMTPSettingsSet( request );
    case HUB_NOTIFICATIONS_SMTP_SEND:
        return handleSMTPSend( request );
    default:
        return Service::handleRequestFromBus( request, requestType );
    }
    return JSON_EMPTY_OBJ;
}

JsonObject NotificationsService::handleSMTPSettingsSet( const JsonObject& request ) {
    Json response;
    try {
        m_notificationsManager->updateSMTPSettings( request );
    }
    catch( const SettingsError& e ) {
        setError( response, Errors::WRONG_PARAMS, e.what() );
        return response;
    }
    catch( const std::exception& e ) {
        setError( response, Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR, e.what() );
        return response;
    }
    catch( ... ) {
        setError( response, Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR );
        return response;
    }

    return response;
}

JsonObject NotificationsService::handleSMTPSend( const JsonObject& request ) {
    Json response;
    try {
        namespace Fields = Resources::RPC::EmailFields;

        auto sender = json_get_str( request, Fields::Sender );
        if( sender.empty() ) {
            std::string error = "Field '" + std::string( Fields::Sender ) + "' is empty";
            LOGGER_ERROR( cc::error( "Invalid request to send email: " ) + cc::note( error ) );
            setError( response, Errors::WRONG_PARAMS, error );
            return response;
        }

        auto recipient = json_get_str( request, Fields::Recipient );
        if( recipient.empty() ) {
            std::string error = "Field '" + std::string( Fields::Recipient ) + "' is empty";
            LOGGER_ERROR( cc::error( "Invalid request to send email: " ) + cc::note( error ) );
            setError( response, Errors::WRONG_PARAMS, error );
            return response;
        }

        auto topic = json_get_str( request, Fields::Topic );
        if( topic.empty() ) {
            std::string error = "Field '" + std::string( Fields::Topic ) + "' is empty";
            LOGGER_ERROR( cc::error( "Invalid request to send email: " ) + cc::note( error ) );
            setError( response, Errors::WRONG_PARAMS, error );
            return response;
        }

        auto body = json_get_str( request, Fields::Body );
        if( body.empty() ) {
            std::string error = "Field '" + std::string( Fields::Body ) + "' is empty";
            LOGGER_ERROR( cc::error( "Invalid request to send email: " ) + cc::note( error ) );
            setError( response, Errors::WRONG_PARAMS, error );
            return response;
        }

        std::time_t date = static_cast< std::time_t >( json_get_int( request, Fields::Date, std::time( nullptr ) ) );

        m_notificationsManager->sendEmail( sender, recipient, topic, body, date );
    }
    catch( const std::exception& e ) {
        setError( response, Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR, e.what() );
        return response;
    }
    catch( ... ) {
        setError( response, Errors::INTERNAL_UNHANDLED_EXCEPTION_ERROR );
        return response;
    }

    return response;
}

JsonObject NotificationsService::getServiceInfo() {
    return JSON( "metrics" << m_notificationsManager->dumpMetrics() );
}

int NotificationsService::restoreBackup( const std::string& tempBackupFolder ) {
    CommonUtils::RestoreHelper helper( getDBClientSafe(), tempBackupFolder );
    for( auto& tableName : Resources::DB::BackupTableNames ) {
        if( !helper.restoreDatabase( tableName ) ) {
            return helper.getReturnCode();
        }
    }
    return helper.getReturnCode();
}

}  // namespace Notifications
}  // namespace HubPlatform
