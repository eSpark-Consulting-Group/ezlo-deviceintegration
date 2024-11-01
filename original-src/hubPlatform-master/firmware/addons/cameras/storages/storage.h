#pragma once
#include "cameras/defs.h"
#include "cameras/storages/file_sender.h"
#include "json_utils/json_fwd.h"
#include "config_reader/config_reader.h"
#include <string>
#include <stdexcept>
#include <atomic>

#include <system_utils/filesystem.h>

namespace HubPlatform {

class StorageException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Storage {
    friend class StorageManager;

public:
    using Bytes = std::atomic_llong;
    ///
    /// \brief createFileSender Creates initialized sender, ready for uploading
    /// \param fileName
    /// \return can return nullptr if error
    ///
    virtual FileSenderSharedPtr createFileSender( const std::string& fileName ) = 0;

    virtual void setProperties( const JsonObject& ) = 0;
    virtual void clearStorage();

    static Bytes& getStorageVolume();
    virtual int64_t getMaxStorageVolume();

    virtual ~Storage() = default;
};

class ILocalStorage : public Storage {
public:
    int64_t getMaxStorageVolume() override;
    static void setMaxStorageVolume( int maxStorageSizeMb );

private:
    static Bytes MaxStorageSize;
};

}  // namespace HubPlatform
