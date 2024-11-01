#pragma once
#include "cameras/storages/storage.h"

namespace HubPlatform {

class LocalStorage : public ILocalStorage {
public:
    LocalStorage() = default;
    explicit LocalStorage( std::string folderPath );
    LocalStorage( const LocalStorage& ) = delete;
    LocalStorage( LocalStorage&& ) = delete;

    void setProperties( const JsonObject& properties ) override;
    FileSenderSharedPtr createFileSender( const std::string& fileName ) override;

    std::string getPath() const;

protected:
    std::string m_path;
};
}  // namespace HubPlatform
