#pragma once

#include "2gig/peristence_client.h"

#include <sstream>
#include <vector>

namespace HubPlatform {
namespace TwoGig {

struct InMemoryPersistenceClient : public ::TwoGig::PersistenceClient {
    void writeBytes( const uint8_t* bytes, size_t count ) override {
        m_buffer.sputn( reinterpret_cast< const char* >( bytes ), count );
    }

    void readBytes( uint8_t* buffer, size_t count ) override {
        m_buffer.sgetn( reinterpret_cast< char* >( buffer ), count );
    }

    std::vector< uint8_t > getData() const {
        auto data = m_buffer.str();

        return std::vector< uint8_t >( data.begin(), data.end() );
    }

    void setData( const std::vector< uint8_t >& bytes ) {
        std::string data( bytes.begin(), bytes.end() );

        m_buffer.str( data );
    }

private:
    std::stringbuf m_buffer;
};

}  // namespace TwoGig
}  // namespace HubPlatform