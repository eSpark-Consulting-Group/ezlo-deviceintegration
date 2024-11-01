#pragma once

#include <json_utils/json_fwd.h>
#include <string>
#include <map>
#include <stdexcept>

namespace HubPlatform {
namespace CommonHttp {

namespace Limits {

/** Max data length for send_data() call, actually can be anything,
 * doesn't use any buffers to send data provided by a user.
 */
constexpr const int SEND_DATA_MAX_DATA_LENGTH = 500000;
constexpr const int DEF_REDIRECTION = 3;
constexpr const int MAX_REDIRECTION = 15;
constexpr const int MIN_DATA_BUFFER = 32;
constexpr const int MAX_DATA_BUFFER_LINUX = 500000;
constexpr const int DEF_TIMEOUT_MS = 30 * 1000;
constexpr const int MAX_TIMEOUT_MS = 3600 * 1000 * 24;
constexpr const int DEF_CONNECT_TIMEOUT_MS = 10 * 1000;
constexpr const int MAX_CONNECT_TIMEOUT_MS = 600 * 1000;
constexpr const int PROGRESS_PERCENT_PORTION = 5;

}  // namespace Limits

class OptionsError : public std::runtime_error {
public:
    explicit OptionsError( const std::string& reason ) noexcept
    : std::runtime_error( "OptionsError: " + reason ) {}
};

struct Certificates {
    std::string ca;
    std::string privateKey;
    std::string publicKey;

    bool empty() const {
        return ca.empty() && privateKey.empty() && publicKey.empty();
    }
};

void to_json( Json& j, const Certificates& opts );
void from_json( const Json& j, Certificates& certificates );

enum class ERequestType {
    GET = 0,
    HEAD = 1,
    POST = 2,
    PUT = 3,
    DELETE = 4,
    CONNECT = 5,
    OPTIONS = 6,
    TRACE = 7,

    // DO NOT ADD ELEMENT AFTER THIS ONE
    END_OF_COMMANDS
};

namespace Mime {

enum class DataType { RAW, FILE };

struct Part {
    std::string name;
    std::string data;
    DataType dataType;
};

}  // namespace Mime

using Header = std::pair< std::string, std::string >;
using Headers = std::multimap< std::string, std::string >;
using MimeParts = std::vector< Mime::Part >;

/** According to https://confluence.mios.com/display/ELFD/Lua+HTTP v. 44 */
struct HttpOptions {
    enum { DOCS_VERSION = 44 };

public:
    // required fields
    std::string url;
    std::string handler;

    // optional fields
    Certificates certs;
    std::string type = "GET";
    std::string contentType;
    std::string user;
    std::string password;
    std::string receivedDataFilePath;
    std::string interface;

    std::string data;
    Json userData;

    Headers headers;
    MimeParts mimeParts;

    int contentLength = 0;
    int newDataBuffer = 0;
    int timeoutMs = CommonHttp::Limits::DEF_TIMEOUT_MS;
    int connectTimeoutMs = CommonHttp::Limits::DEF_CONNECT_TIMEOUT_MS;

    unsigned char redirection = CommonHttp::Limits::DEF_REDIRECTION;

    bool skipSecurity = false;
    bool keepAlive = false;
    bool repeatAllowed = true;
    bool last = true;
    bool newDataCache = true;
    bool failOnError = true;       //!< true: stop receiving body (default); false: ignore error and keep getting body
    bool verbose = false;          //!< Be verbose
    bool multipleHeaders = false;  //!< Use array/multimap of headers instead of map if true.

public:
    void validate();
    static const char* commandName( ERequestType command );
    static int version();
};

void to_json( Json& j, const HttpOptions& opts );
void from_json( const Json& json, HttpOptions& opts );

void to_json( Json& j, const Headers& opts, bool multipleHeaders );
void from_json( const Json& json, Headers& opts );

class ConnectionInfo final {
public:
    ConnectionInfo() = default;
    ConnectionInfo( std::string id, std::string url, Json userData, int maxDataLength );

    Json toJson() const;

private:
    std::string m_id;
    std::string m_url;
    Json m_userData;
    int m_maxDataLength = 0;
};

}  // namespace CommonHttp
}  // namespace HubPlatform
