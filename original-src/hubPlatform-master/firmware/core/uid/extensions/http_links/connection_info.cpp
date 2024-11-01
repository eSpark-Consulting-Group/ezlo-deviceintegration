#include "uid/extensions/http_links/connection_info.h"
#include "uid/extensions/http_links/resources.h"

namespace HubPlatform {
namespace HttpLinks {

ConnectionInfo::ConnectionInfo( std::string linkId, ConnectionInfo::Headers headers, ConnectionInfo::Params params )
: m_linkId( std::move( linkId ) )
, m_headers( std::move( headers ) )
, m_params( std::move( params ) ) {}

void ConnectionInfo::clear() {
    m_id.clear();
    m_linkId.clear();
    m_headers.clear();
    m_params.clear();
    m_created = 0;
    m_closed = 0;
}

const std::string& ConnectionInfo::id() const {
    return m_id;
}

void ConnectionInfo::id( std::string value ) {
    m_id = std::move( value );
}

const std::string& ConnectionInfo::linkId() const {
    return m_linkId;
}

void ConnectionInfo::linkId( std::string value ) {
    m_linkId = std::move( value );
}

time_t ConnectionInfo::created() const {
    return m_created;
}

void ConnectionInfo::created( time_t value ) {
    m_created = value;
}

time_t ConnectionInfo::closed() const {
    return m_closed;
}

void ConnectionInfo::closed( time_t value ) {
    m_closed = value;
}

bool ConnectionInfo::isClosed() const {
    return m_closed > 0;
}

const ConnectionInfo::Headers& ConnectionInfo::headers() const {
    return m_headers;
}

void ConnectionInfo::headers( ConnectionInfo::Headers value ) {
    m_headers = std::move( value );
}

const ConnectionInfo::Params& ConnectionInfo::params() const {
    return m_params;
}

void ConnectionInfo::params( ConnectionInfo::Params value ) {
    m_params = std::move( value );
}

void ConnectionInfo::fillArray( Json& to, const std::string& key, const std::vector< KeyValue >& collection ) {
    if( !collection.empty() ) {
        JsonArray& arr = to[ key ];
        arr = JSON_EMPTY_ARR;

        for( const auto& pair : collection ) {
            arr.push_back( { { pair.first, pair.second } } );
        }
    }
}

void ConnectionInfo::fetchArray( const Json& from, const std::string& key, std::vector< KeyValue >& pairs ) {
    Json jsonArray;
    if( json_get( from, key, jsonArray ) ) {
        for( const auto& obj : jsonArray ) {
            if( obj.is_object() ) {
                for( auto it = obj.begin(); it != obj.end(); it++ ) {
                    pairs.emplace_back( it.key().c_str(), json_get_str( it.value() ) );
                }
            }
        }
    }
}

Json ConnectionInfo::toJsonDb() const {
    using namespace HttpLinks::Resources;
    Json result = JSON( ConnectionDbFields::id << id() << ConnectionDbFields::linkId << linkId()
                                               << ConnectionDbFields::created << m_created << ConnectionDbFields::closed
                                               << m_closed << ConnectionDbFields::clientIp << m_clientIp );
    fillArray( result, ConnectionDbFields::headers, m_headers );
    fillArray( result, ConnectionDbFields::params, m_params );
    return result;
}

Json ConnectionInfo::toJsonLua() const {
    Json result = JSON( "id" << id() << "link_id" << linkId() << "status" << ( m_closed > 0 ? "closed" : "pending" )
                             << "client_ip" << m_clientIp << "created" << m_created );
    fillArray( result, "headers", m_headers );
    fillArray( result, "params", m_params );

    return result;
}

void ConnectionInfo::fromJsonDb( const Json& info ) {
    clear();
    namespace Fields = Resources::ConnectionDbFields;

    m_id = json_get_str( info, "_id" );
    m_linkId = json_get_str( info, Fields::linkId );
    m_created = json_get_int( info, Fields::created );
    m_closed = json_get_int( info, Fields::closed );

    fetchArray( info, Fields::headers, m_headers );
    fetchArray( info, Fields::params, m_params );

    m_clientIp = json_get_str( info, Fields::clientIp );
}

void ConnectionInfo::clientIp( std::string value ) {
    m_clientIp = std::move( value );
}

const std::string& ConnectionInfo::clientIp() const {
    return m_clientIp;
}

}  // namespace HttpLinks
}  // namespace HubPlatform