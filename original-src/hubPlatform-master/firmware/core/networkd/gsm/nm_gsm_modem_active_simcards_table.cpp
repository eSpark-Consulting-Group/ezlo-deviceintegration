#include "networkd/gsm/nm_gsm_modem_active_simcards_table.h"

#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace Network {

GSMModemActiveSimcardsTable::GSMModemActiveSimcardsTable( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

std::string GSMModemActiveSimcardsTable::getICCID( const std::string& modemIMEI ) {
    if( modemIMEI.empty() )
        return {};

    auto query = JSON( db::ID_FIELD_NAME << modemIMEI );
    auto result = m_dbClient.findOne( NetworkGSMModemActiveSimCardsDBName, query, nullptr );
    return json_get_str( result, "iccid" );
}

void GSMModemActiveSimcardsTable::storeSimcard( const std::string& modemIMEI, const std::string& simcardICCID ) {
    auto j = JSON( db::ID_FIELD_NAME << modemIMEI << "iccid" << simcardICCID );
    m_dbClient.update( NetworkGSMModemActiveSimCardsDBName, JSON_EMPTY_OBJ, j, true, false );
}

}  // namespace Network
}  // namespace HubPlatform
