#include "json_utils/json_path.h"
#include <utility>

void JsonPath::pushBack( Json key ) {
    m_keys.push_back( std::move( key ) );
}

void JsonPath::popBack() {
    if( !m_keys.empty() )
        m_keys.pop_back();
}

Json JsonPath::back() {
    if( m_keys.empty() )
        return Json{};
    return m_keys.back();
}

size_t JsonPath::size() const {
    return m_keys.size();
}

std::string JsonPath::toString() const {
    std::string result;
    auto formatKey = []( const Json& key ) -> std::string {
        if( key.is_number() )
            return "[" + key.dump() + "]";
        return key.dump();
    };
    for( const auto& key : m_keys ) {
        const char* delimiter = "/";
        result.append( delimiter + formatKey( key ) );
    }
    return result;
}

bool JsonPath::empty() const {
    return m_keys.empty();
}

const JsonPath::Keys& JsonPath::keys() const {
    return m_keys;
}

void JsonPath::append( const JsonPath& add ) {
    m_keys.insert( end( m_keys ), begin( add.m_keys ), end( add.m_keys ) );
}

JsonPath::Guard JsonPath::makeGuard( Json key ) {
    return JsonPath::Guard( *this, std::move( key ) );
}

JsonPath::JsonPath( std::initializer_list< Json > init ) {
    for( auto it : init )
        pushBack( std::move( it ) );
}

JsonPath::JsonPath( JsonPath::Keys init ) {
    for( auto& it : init )
        pushBack( std::move( it ) );
}

JsonPath::Guard::Guard( JsonPath::Guard&& r ) noexcept
: m_path( std::forward< decltype( r.m_path ) >( r.m_path ) ) {}

JsonPath::Guard::Guard( JsonPath& path, Json key )
: m_path( std::ref( path ) ) {
    m_path.get().pushBack( std::move( key ) );
}

JsonPath::Guard::~Guard() {
    if( !m_path.get().empty() )
        m_path.get().popBack();
}