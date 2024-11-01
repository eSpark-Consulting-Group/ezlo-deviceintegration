#ifndef MD5_H
#define MD5_H

#include <string>
#include <vector>
#include <websocketpp/common/md5.hpp>

class Md5 {
public:
    Md5();
    void append( std::ifstream& stream );
    void append( const std::string& str );
    void append( const std::vector< unsigned char >& data );
    void append( const unsigned char* buff, int size );
    std::vector< unsigned char > getBin();
    std::string get();
    std::string getHex();
    static std::string getHex( const std::string& name,
                               const std::string& lenStr,
                               const std::vector< unsigned char >& buff );
    static std::vector< unsigned char > getBin( const std::vector< unsigned char >& buff );
    static size_t getBinSize();

private:
    websocketpp::md5::md5_state_s md5State_;
};

#endif  // MD5_H
