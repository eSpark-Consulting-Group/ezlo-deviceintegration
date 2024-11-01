#include "notifications/remote_interface/notifications_remote_interface.h"
#include "notifications/database_settings/database_settings.h"
#include "service_interfaces/groups_names.h"
#include "service_interfaces/service.h"
#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"

namespace HubPlatform {
namespace Notifications {

RemoteInterface::RemoteInterface( Services::Service& hostService )
: m_hostService( hostService ) {}

JsonObject RemoteInterface::smtpSettingsSet( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::NOTIFICATIONS,
                                      params,
                                      MessageSubClasses::HUB_NOTIFICATIONS_SMTP_SETTINGS_SET,
                                      std::chrono::seconds( 60 ) );
}

JsonObject RemoteInterface::smtpSettingsGet() {
    return DatabaseSettings( m_hostService.getDBClient() ).getHashedSMTPSettings();
}

JsonObject RemoteInterface::sendEmail( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::NOTIFICATIONS,
                                      params,
                                      MessageSubClasses::HUB_NOTIFICATIONS_SMTP_SEND,
                                      std::chrono::seconds( 60 ) );
}

}  // namespace Notifications
}  // namespace HubPlatform
