#include "2gig_persistence_client.h"

namespace HubPlatform {
namespace TwoGig {

std::unique_ptr< ::TwoGig::PersistenceClient > PersistenceClientImpl::createForReading( const std::string& filePath ) {
    return create( filePath, std::ios_base::binary | std::ios_base::in );
}

std::unique_ptr< ::TwoGig::PersistenceClient > PersistenceClientImpl::createForWriting( const std::string& filePath ) {
    return create( filePath, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc );
}

std::unique_ptr< ::TwoGig::PersistenceClient > PersistenceClientImpl::create( const std::string& filePath,
                                                                              std::ios_base::openmode mode ) {
    std::unique_ptr< PersistenceClientImpl > client( new PersistenceClientImpl );

    client->m_file.open( filePath, mode );

    if( client->m_file.is_open() )
        return std::move( client );

    return {};
}

PersistenceClientImpl::PersistenceClientImpl() = default;

PersistenceClientImpl::~PersistenceClientImpl() {
    m_file.close();
}

void PersistenceClientImpl::writeBytes( const uint8_t* bytes, size_t count ) {
    m_file.sputn( reinterpret_cast< const char* >( bytes ), count );
}

void PersistenceClientImpl::readBytes( uint8_t* buffer, size_t count ) {
    m_file.sgetn( reinterpret_cast< char* >( buffer ), count );
}

}  // namespace TwoGig
}  // namespace HubPlatform