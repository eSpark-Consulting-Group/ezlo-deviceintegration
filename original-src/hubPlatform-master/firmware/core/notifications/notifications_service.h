#pragma once

#include "notifications/notifications_manager/notifications_manager.h"
#include "service_interfaces/service.h"

#include <memory>

namespace HubPlatform {
namespace Notifications {

class NotificationsService final : public Services::Service {
public:
    static const char* const Name;

public:
    NotificationsService();

    int restoreBackup( const std::string& tempBackupFolder ) override;

private:
    void setUp() override;
    void tearDown() override;

    JsonObject handleRequestFromBus( const JsonObject& request, MessageSubClasses requestType ) override;

private:
    JsonObject doHandleRequestFromBus( const JsonObject& request, MessageSubClasses requestType );

    virtual JsonObject getServiceInfo() override;

    JsonObject handleSMTPSettingsSet( const JsonObject& request );
    JsonObject handleSMTPSend( const JsonObject& request );

private:
    std::unique_ptr< NotificationsManager > m_notificationsManager;
};

}  // namespace Notifications
}  // namespace HubPlatform
