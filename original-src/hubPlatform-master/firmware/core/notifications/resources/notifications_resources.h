#pragma once

#include <vector>
#include <string>

namespace HubPlatform {
namespace Notifications {
namespace Resources {
namespace DB {

extern const std::vector< std::string > BackupTableNames;

namespace Settings {
extern const char* const SMTPSettingsId;

namespace FieldsSMTP {
extern const char* const SMTPServerUrl;
extern const char* const Encryption;
extern const char* const VerifyServerCertificate;
extern const char* const VerifyServerHostname;
extern const char* const CertificatesVersion;
extern const char* const ServerCACertificate;
extern const char* const ClientCertificate;
extern const char* const ClientCertificateKey;
extern const char* const ClientUsername;
extern const char* const ClientPassword;
}  // namespace FieldsSMTP

}  // namespace Settings

}  // namespace DB

namespace RPC {
namespace EmailFields {
extern const char* const Sender;
extern const char* const Recipient;
extern const char* const Topic;
extern const char* const Body;
extern const char* const Date;
}  // namespace EmailFields
}  // namespace RPC

namespace ConfigKeys {
extern const char* const NotificationsSmtpSendRetryLimit;
extern const char* const NotificationsSmtpSendPendingLimit;
extern const char* const NotificationsSmtpSendInterval;
extern const char* const NotificationsSmtpSendTimeout;
}  // namespace ConfigKeys

}  // namespace Resources
}  // namespace Notifications
}  // namespace HubPlatform
