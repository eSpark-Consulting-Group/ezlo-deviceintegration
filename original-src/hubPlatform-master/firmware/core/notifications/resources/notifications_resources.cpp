#include "notifications/resources/notifications_resources.h"

#include "database_client/db_tables_names.h"
#include "database_client/dbqueryparser.h"

namespace HubPlatform {
namespace Notifications {
namespace Resources {
namespace DB {

const std::vector< std::string > BackupTableNames = { HaNotificationsSettingsDbName };

namespace Settings {
const char* const SMTPSettingsId = "smtp";

namespace FieldsSMTP {
const char* const SMTPServerUrl = "smtpServerUrl";
const char* const Encryption = "encryption";
const char* const VerifyServerCertificate = "verifyServerCertificate";
const char* const VerifyServerHostname = "verifyServerHostname";
const char* const CertificatesVersion = "certificatesVersion";
const char* const ServerCACertificate = "serverCACertificate";
const char* const ClientCertificate = "clientCertificate";
const char* const ClientCertificateKey = "clientCertificateKey";
const char* const ClientUsername = "clientUsername";
const char* const ClientPassword = "clientPassword";
}  // namespace FieldsSMTP

}  // namespace Settings

}  // namespace DB

namespace RPC {
namespace EmailFields {
const char* const Sender = "sender";
const char* const Recipient = "recipient";
const char* const Topic = "topic";
const char* const Body = "body";
const char* const Date = "date";
}  // namespace EmailFields
}  // namespace RPC

namespace ConfigKeys {
const char* const NotificationsSmtpSendRetryLimit = "notifications/smtpSendRetryLimit";
const char* const NotificationsSmtpSendPendingLimit = "notifications/smtpSendPendingLimit";
const char* const NotificationsSmtpSendInterval = "notifications/smtpSendInterval";
const char* const NotificationsSmtpSendTimeout = "notifications/smtpSendTimeout";
}  // namespace ConfigKeys

}  // namespace Resources
}  // namespace Notifications
}  // namespace HubPlatform
