#pragma once

#include <string>
#include <memory>
#include "json_utils/json_fwd.h"

namespace HubPlatform {
namespace LogCmd {

class LogStorageImpl;

class LogStorage {
public:
    using StoragePimpl = std::unique_ptr< LogStorageImpl >;
    LogStorage();
    LogStorage( StoragePimpl storagePimpl );
    ~LogStorage();
    Json upload( const std::string& filePath, const Json& storageMetadata );

private:
    StoragePimpl m_storagePimpl;
};

}  // namespace LogCmd
}  // namespace HubPlatform
