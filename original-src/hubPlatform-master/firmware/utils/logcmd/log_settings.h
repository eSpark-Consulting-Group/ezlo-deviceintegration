#pragma once

#include <cstddef>

namespace HubPlatform {
namespace LogCmd {

struct LogSettings {
    enum class RotationPolicy { Off, EachRotation, StoreLocally };
    RotationPolicy rotationPolicy = RotationPolicy::EachRotation;
    unsigned int rotationInterval = 12;
    size_t archivesTotalSizeLimit = 1 * 1024 * 1024;
    size_t fileSizeLimit = 2 * 1024 * 1024;
    size_t pendingLogsSizeLimit = 200 * 1024 * 1024;

public:
    void setArchivesTotalSizeLimit( size_t sizeLimit );
    void setSliceFileSizeLimit( size_t sizeLimit );
    void setSlicePendingSizeLimit( size_t sizeLimit );
};

void loadLogSettings( LogSettings& logSettings );

inline size_t mbToBytes( size_t value ) {
    return value * 1024 * 1024;
}
inline size_t kbToBytes( size_t value ) {
    return value * 1024;
}

}  // namespace LogCmd
}  // namespace HubPlatform
