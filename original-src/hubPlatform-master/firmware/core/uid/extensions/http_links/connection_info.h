#pragma once
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace HttpLinks {

class ConnectionInfo final {
public:
    using KeyValue = std::pair< std::string, std::string >;
    using Headers = std::vector< KeyValue >;
    using Params = std::vector< KeyValue >;

    void clientIp( std::string value );
    const std::string& clientIp() const;

public:
    ConnectionInfo() = default;
    ConnectionInfo( std::string linkId, Headers headers, Params params );
    void clear();

    const std::string& id() const;
    void id( std::string value );
    const std::string& linkId() const;
    void linkId( std::string value );
    time_t created() const;
    void created( time_t value );
    time_t closed() const;
    void closed( time_t value );
    bool isClosed() const;

    const Headers& headers() const;
    void headers( Headers value );
    const Params& params() const;
    void params( Params value );

    static void fillArray( Json& to, const std::string& key, const std::vector< KeyValue >& collection );
    static void fetchArray( const Json& from, const std::string& key, std::vector< KeyValue >& pairs );

    // to store in db
    Json toJsonDb() const;
    // For getConnectionInfo
    Json toJsonLua() const;

    void fromJsonDb( const Json& info );

private:
    std::string m_id;
    std::string m_linkId;
    time_t m_created{ 0 };
    time_t m_closed{ 0 };
    Headers m_headers;
    Params m_params;
    std::string m_clientIp;
};

}  // namespace HttpLinks
}  // namespace HubPlatform