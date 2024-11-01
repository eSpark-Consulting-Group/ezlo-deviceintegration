#pragma once
#include "cameras/defs.h"
#include "system_utils/filesystem.h"

#include <string>
#include <stdexcept>
#include <atomic>

namespace HubPlatform {

class IFileRemover {
public:
    virtual ~IFileRemover() = default;
};

class FolderRemover {
public:
    explicit FolderRemover( std::string folder );
    ~FolderRemover();

private:
    const std::string m_folderPath;
};

class FileRemover : public IFileRemover {
public:
    FileRemover( std::string filePath, std::atomic_llong& storageSize );
    ~FileRemover() override;

private:
    std::string m_filePath;
    std::atomic_llong& m_storageSize;
    static std::atomic_int m_counter;
};

class EmptyFileRemover : public IFileRemover {
public:
};

}  // namespace HubPlatform
