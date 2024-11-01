#pragma once
#include "local_notification_downloader.h"
#include "types/meshbot_template_record.h"
#include "database_client/idbclient.h"
#include <string>

namespace HubPlatform {

namespace CloudDevices {
namespace LocalNotification {

class DataSegmentsDeserializer {
public:
    explicit DataSegmentsDeserializer( IDbClient& dbClient );
    virtual void fromJson( const JsonObject& data ) = 0;
    virtual JsonObject toJson() = 0;
    virtual void addRecordToDb( const int currentVersion, const int fileVersion ) = 0;

protected:
    IDbClient& m_dbClient;
};

class LocationDeserializer : public DataSegmentsDeserializer {
public:
    LocationDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    int m_oemId{};
    std::string m_locationUuid;
    int m_accountId{};
    JsonArray m_permissions;
};

class UserDeserializer : public DataSegmentsDeserializer {
public:
    UserDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    std::string encryptData( const std::string& data );
    std::string decryptData( const std::string& data );

private:
    int m_userId{};
    std::string m_userUuid;
    int m_permissionRole{};
    std::string m_email;
    int m_emailValidated{};
    std::string m_arn;
    std::string m_firstName;
    std::string m_lastName;
    std::string m_userName;
    JsonArray m_notificationCategories;
    JsonArray m_permissions;
};

class PartnerDeserializer : public DataSegmentsDeserializer {
public:
    PartnerDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    JsonObject m_data;
};

class SettingsDeserializer : public DataSegmentsDeserializer {
public:
    SettingsDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    JsonObject m_data;
};

class ProtectDeserializer : public DataSegmentsDeserializer {
public:
    ProtectDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    std::string m_provider;
};

class RegisteredDevicesDeserializer : public DataSegmentsDeserializer {
public:
    RegisteredDevicesDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    std::string m_uuid;
    std::string m_name;
    JsonArray m_capabilities;
    JsonArray m_outboundConnectivity;
};

class MeshbotUuidDeserializer : public DataSegmentsDeserializer {
public:
    MeshbotUuidDeserializer( IDbClient& dbClient );
    void fromJson( const JsonObject& data ) override;
    JsonObject toJson() override;
    void addRecordToDb( const int currentVersion, const int fileVersion ) override;

private:
    std::string getUuid();

private:
    MeshbotTemplateRecord m_record;
};

}  // namespace LocalNotification
}  // namespace CloudDevices
}  // namespace HubPlatform
