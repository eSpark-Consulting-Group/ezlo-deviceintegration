#pragma once
#include <string>
#include <set>
#include <iosfwd>

namespace HubPlatform {
namespace CryptoUtils {

class Digests final {
public:
    using DigestNames = std::set< std::string >;

public:
    Digests();
    static void initAllDigests();
    static Digests& getInstance();

    static bool calculateDigest( const std::string& digestName, const std::string& message, std::string& digest );
    static bool calculateDigest( const std::string& digestName, std::istream& stream, std::string& digest );

    bool appendDigest( const std::string& name );
    const DigestNames& supportedDigests();

public:
    static std::string md5( const std::string& );
    static std::string md5( std::istream& stream );
    static std::string sha1( const std::string& );
    static std::string sha1( std::istream& stream );
    static std::string sha256( const std::string& );
    static std::string sha256( std::istream& stream );

private:
    std::set< std::string > m_supportedDigests;
};

std::string toHex( const unsigned char* text, size_t size );
std::string toHex( const std::string& digest );

}  // namespace CryptoUtils

}  // namespace HubPlatform