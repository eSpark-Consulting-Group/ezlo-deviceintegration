#include "luup/user_data.h"

#include "database_client/idbclient.h"

namespace HubPlatform {
namespace Luup {

UserData::UserData( IDbClient& dbClient )
: m_dbClient( dbClient )
, m_dataVersion( 1 ) {}

Json UserData::getData( uint64_t requestDataVersion, bool staticUserData ) const {
    std::lock_guard< std::mutex > lock( m_mtx );

    if( requestDataVersion >= m_dataVersion )
        return JSON_EMPTY_OBJ;

    auto result = JSON( "DataVersion" << m_dataVersion );

    return result;
}

UserData::DeviceId UserData::addDevice() {
    std::lock_guard< std::mutex > lock( m_mtx );

    m_dataVersion++;

    return "";
}

void UserData::updateDevice( DeviceId deviceId ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    m_dataVersion++;
}

void UserData::removeDevice( DeviceId deviceId ) {
    std::lock_guard< std::mutex > lock( m_mtx );

    m_dataVersion++;
}

}  // namespace Luup
}  // namespace HubPlatform
