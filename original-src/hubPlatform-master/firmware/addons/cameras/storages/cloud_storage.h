#pragma once
#include "cameras/storages/storage.h"
#include "cameras/storages/cloud_file_sender.h"
#include "cameras/defs.h"
#include <atomic>

namespace HubPlatform {

class CloudStorage : public ILocalStorage {
public:
    explicit CloudStorage( const std::string& folderPath );
    CloudStorage( const CloudStorage& ) = delete;
    CloudStorage( CloudStorage&& ) = delete;

    void setProperties( const JsonObject& properties ) override;
    FileSenderSharedPtr createFileSender( const std::string& fileName ) override;
    void clearStorage() override;

    void setSaveFileCallback( SaveFileCallback saveFile );
    std::string getPath() const;
    void setMetaData( const Json& metaData );

protected:
    const std::string m_folderPath;
    SaveFileCallback m_saveFile;
    CloudStorageContextPtr m_cloudStorageContext;
};
}  // namespace HubPlatform
