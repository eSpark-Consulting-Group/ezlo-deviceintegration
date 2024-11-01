#pragma once

#include <string>
#include <functional>

#include "device_model/h/permission.h"

class Acl {
public:
    using requestUnbindUserClbk_t = std::function< void( const std::string& ) >;
    using pageViewUserRemoveClbk_t = std::function< void( const std::string& ) >;

    static void create( const std::string& id, const Resource::Resources resource, const JsonObject& params );
    static void remove( const std::string& id, requestUnbindUserClbk_t requestUnbindUserClbk );
    static void set( const std::string& id, const std::string& open2all, const std::string& close2all );
    static void get( const std::string& id, JsonObject& result );
    static void assign( const std::string& id,
                        const std::string& user,
                        const std::string& permission,
                        JsonObject& answer );
    static bool checkAssigned( const JsonObject& params,
                               JsonObject& answer,
                               pageViewUserRemoveClbk_t pageViewUserRemoveClbkr );
    static void unassign( const std::string& id,
                          const std::string& user,
                          requestUnbindUserClbk_t requestUnbindUserClbk );
    static void unassignAll( const JsonObject& params, pageViewUserRemoveClbk_t pageViewUserRemoveClbk );
    static bool checkPermission( const std::string& id,
                                 const JsonObject& params,
                                 const Resource::Resources resource,
                                 const int operation );
    static bool checkWithParent( const std::string& id,
                                 const JsonObject& params,
                                 Resource::Resources resource,
                                 const int operation,
                                 const std::string& dbName,
                                 JsonObject& answer );
    static bool check( const std::string& id,
                       const JsonObject& params,
                       Resource::Resources resource,
                       const int operation,
                       JsonObject& answer );
    static bool check( const std::string& id, const JsonObject& params, const int operation, JsonObject& answer );
    static bool check( const JsonObject& params,
                       const Resource::Resources resource,
                       const int operation,
                       JsonObject& answer );

private:
    static bool check_( const JsonObject& params,
                        const Resource::Resources resource,
                        const int operation,
                        JsonObject& answer );
    static bool checkLocked( const int operation, JsonObject& answer );
    static void unassignAll( const std::string& email,
                             const std::string& sender,
                             pageViewUserRemoveClbk_t pageViewUserRemoveClbk );
};
