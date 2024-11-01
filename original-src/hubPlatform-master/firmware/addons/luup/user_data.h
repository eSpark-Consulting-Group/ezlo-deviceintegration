#pragma once

#include "json_utils/json_utils.h"

#include <string>
#include <mutex>

class IDbClient;

namespace HubPlatform {

namespace UT {
class UserDataTestSuite;
}

namespace Luup {

/* @brief manages Vera's user data structure */
class UserData final {
    friend UT::UserDataTestSuite;

public:
    using DeviceId = std::string;

    UserData( IDbClient& dbClient );

    UserData( const UserData& rhs ) = delete;
    UserData& operator=( const UserData& rhs ) = delete;

    Json getData( uint64_t requestDataVersion = 0, bool staticUserData = true ) const;

    /* @brief add/update/remove a device, throws an exception if fails */
    DeviceId addDevice( /* C++ types... */ );
    void updateDevice( DeviceId deviceId /*, C++ types... */ );
    void removeDevice( DeviceId deviceId );

private:
    const char* m_userDataDbTableName = "luup.userData";
    const char* m_devicesEntryName = "devices";

    IDbClient& m_dbClient;

    /* each time changes to user data happens data version gets increased, using this
     * we can provide UIs by user data only if some changes happened to it */
    uint64_t m_dataVersion;

    mutable std::mutex m_mtx;
};

}  // namespace Luup
}  // namespace HubPlatform
