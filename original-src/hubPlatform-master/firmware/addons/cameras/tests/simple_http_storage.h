#pragma once
#include "cameras/storages/storage.h"
#include "common_utils/cpp14.h"

namespace HubPlatform {

class SimpleHttpStorage : public Storage {
public:
    FileSenderSharedPtr createFileSender( const std::string& fileName ) override;
    void setProperties( const JsonObject& properties ) override;

private:
    std::string m_baseUrl;
};
}  // namespace HubPlatform
