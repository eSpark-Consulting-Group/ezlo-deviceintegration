#pragma once

#include "2gig/peristence_client.h"

#include <fstream>
#include <memory>

namespace HubPlatform {
namespace TwoGig {

class PersistenceClientImpl : public ::TwoGig::PersistenceClient {
public:
    static std::unique_ptr< ::TwoGig::PersistenceClient > createForReading( const std::string& filePath );
    static std::unique_ptr< ::TwoGig::PersistenceClient > createForWriting( const std::string& filePath );

    ~PersistenceClientImpl();

    void writeBytes( const uint8_t* bytes, size_t count ) override;
    void readBytes( uint8_t* buffer, size_t count ) override;

private:
    static std::unique_ptr< ::TwoGig::PersistenceClient > create( const std::string& filePath,
                                                                  std::ios_base::openmode mode );

    PersistenceClientImpl();

private:
    std::filebuf m_file;
};

}  // namespace TwoGig
}  // namespace HubPlatform
