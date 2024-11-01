#ifndef PERMISSION_H
#define PERMISSION_H

#include <string>

#include "bus/enum2str.h"
#include "jsonrpc/message.h"

class Resource {
public:
    enum Resources { UNKNOWN = 0, EZLO, USERS, UI, SCENES, DEVICES };

    static std::string get( const Resources& resourceCode );
    static int getCode( const std::string& resource );

private:
    static const EnumToStrMap& getResourcesMap();
};

class Permission {
public:
    enum Permissions {
        UNKNOWN = 0,
        CLOSE,
        CREATE,
        CLONE,
        VIEW,
        EXECUTE,
        EDIT,
        DELETE,
        SUPER,
        EXPERT,
    };

    static std::string getSender( const JsonObject& params );
    static std::string get( const JsonObject& params, const Resource::Resources resource );
    static int getCode( const std::string& permission );
    static int getCode( const JsonObject& params, const Resource::Resources resource );
    static std::string get( const Permissions& permissionCode );
    static std::string getDefaultOpen2All();
    static std::string getDefaultClose2All();
    static void update( JsonObject& userPermission, const JsonObject& permission );
    static void updatePermission( JsonObject& set_permission, std::string set_profile );
    static void setNoMore( JsonObject& userPermission, const JsonObject& permission );
    static const JsonObject& getExpertOwnerDefault();
    static const JsonObject& getExpertDefault();
    static const JsonObject& getOwnerDefault();
    static const JsonObject& getAdminDefault();
    static const JsonObject& getUserDefault();
    static const JsonObject& getGuestDefault();
    static bool check( const JsonObject& params, const Resource::Resources resource, const int operation );
    static bool check( const JsonObject& params,
                       const Resource::Resources resource,
                       const int operation,
                       JsonObject& answer );
    static bool check( rpc::Message& message, const Resource::Resources resource, const int operation );
    /// Abstract user might have no rights at all. While sharing device on him we grant him these minimal rights
    /// so that he could perform basic actions. They are lower than admin's ones, i.e. he cannot EDIT devices.
    static const JsonArray& getSharedUserMinimalRights();

private:
    static const EnumToStrMap& getPermissionsMap();
};

#endif  // PERMISSION_H
