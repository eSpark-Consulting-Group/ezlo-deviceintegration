#pragma once

#include <string>
#include <vector>

namespace db {

class DbId {
public:
    static std::vector< unsigned char > genOIDData();
    static std::string genOID();

private:
    static std::vector< unsigned char > getMacAddress( const std::string& interface );
    static int getRandomData();
    static void addIntToCharVector( int value, std::vector< unsigned char >& vector, int size = 4 );
    static std::string getHexString( const std::vector< unsigned char >& data, bool upperCase = false );
};

}  // namespace db
