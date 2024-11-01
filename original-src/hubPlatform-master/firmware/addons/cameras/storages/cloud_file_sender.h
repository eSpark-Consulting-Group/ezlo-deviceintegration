#pragma once
#include "cameras/storages/file_sender.h"
#include "cameras/defs.h"
#include <string>
#include <sstream>
#include <fstream>
#include <functional>
#include <atomic>

namespace HubPlatform {

struct CloudStorageContext {
    Json metaData;
};

using CloudStorageContextPtr = std::shared_ptr< CloudStorageContext >;

class CloudFileSender : public FileSender {
public:
    CloudFileSender( const FilePath& filePath,
                     FileRemoverSharedPtr remover,
                     CloudStorageContextPtr cloudStorageContext,
                     SaveFileCallback saveFile );
    CloudFileSender( const CloudFileSender& ) = delete;
    CloudFileSender( CloudFileSender&& ) = delete;
    ~CloudFileSender() override;
    bool finalizeFile() override;
    bool sendData( uint8_t* data, size_t size ) override;
    void setDynamicMetaData( Json metaData ) override;
    FileRemoverSharedPtr getRemover() final;

private:
    static Json mergeMetaData( const Json& userMetaData, const Json& dynamicMetaData );
    static Json createError( const std::string& errorMessage );

    FilePath m_filePath;
    std::ofstream m_output;
    SaveFileCallback m_saveFile;
    const CloudStorageContextPtr m_storageContext;
    FileRemoverSharedPtr m_remover;
    Json m_dynamicMetaData;
    bool m_isFinalized = false;
};
}  // namespace HubPlatform
