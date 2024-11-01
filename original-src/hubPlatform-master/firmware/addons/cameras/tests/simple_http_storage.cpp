#include "cameras/tests/simple_http_storage.h"
#include "cameras/tests/http_file_sender.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {

FileSenderSharedPtr SimpleHttpStorage::createFileSender( const std::string& fileName ) {
    try {
        auto sender = std::make_unique< HttpFileSender >( m_baseUrl + "/" + fileName );
        return std::move( sender );
    }
    catch( std::exception& ex ) {
        return {};
    }
}

void SimpleHttpStorage::setProperties( const JsonObject& properties ) {
    if( !properties.is_object() ) {
        throw StorageException( "Properties is not a json object" );
    }
    auto urlIt = properties.find( "url" );
    if( urlIt == properties.end() || !urlIt->is_string() ) {
        throw StorageException( "\"url\" is not found or not a string" );
    }
    m_baseUrl = urlIt.value().get< std::string >();
}
}  // namespace HubPlatform
