#pragma once

#include <string>

class IDbClient;

namespace HubPlatform {
namespace Network {

class GSMModemActiveSimcardsTable {
public:
    GSMModemActiveSimcardsTable( IDbClient& dbClient );
    std::string getICCID( const std::string& modemIMEI );
    void storeSimcard( const std::string& modemIMEI, const std::string& simcardICCID );

private:
    IDbClient& m_dbClient;
};

}  // namespace Network
}  // namespace HubPlatform
