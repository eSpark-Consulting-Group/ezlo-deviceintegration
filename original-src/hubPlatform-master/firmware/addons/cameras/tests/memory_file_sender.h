#pragma once
#include "cameras/storages/file_sender.h"
#include <string>
#include <sstream>

namespace HubPlatform {

class MemoryFileSender : public FileSender {
public:
    explicit MemoryFileSender( std::string fileName );
    MemoryFileSender( const MemoryFileSender& ) = delete;

    bool finalizeFile() override;
    bool sendData( uint8_t* data, size_t size ) override;

    std::stringstream& getOutputStream();
    const std::stringstream& getOutputStream() const;

private:
    std::string m_fileName;
    std::stringstream m_output;
};
}  // namespace HubPlatform
