#pragma once
#include "cameras/storages/file_sender.h"
#include <fstream>

namespace HubPlatform {

class LocalFileSender : public FileSender {
public:
    explicit LocalFileSender( const std::string& fileName );

    bool finalizeFile() override;
    bool sendData( uint8_t* data, size_t size ) override;
    FileRemoverSharedPtr getRemover() override;

private:
    std::ofstream m_output;
    std::string m_filePath;
};
}  // namespace HubPlatform
