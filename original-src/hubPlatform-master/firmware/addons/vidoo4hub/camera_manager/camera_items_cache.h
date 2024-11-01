#pragma once

#include "vidoo4hub/camera/vidoo_camera.h"
#include <map>
#include <unordered_map>
#include <memory>

namespace HubPlatform {
namespace Vidoo4Hub {

class CameraItemsCache {
public:
    using CameraId = std::string;
    using ItemId = std::string;
    enum class ItemType {
        Motion,
        SecurityThreat,
        Hotzones,
        Snapshot,
        MakeRecording,
        StopRecording,
        CameraStream,
    };

    struct Data {
        Data( CameraId cameraId, ItemType itemType );
        CameraId m_cameraId;
        ItemType m_itemType;
    };
    using Cache = std::unordered_map< ItemId, Data >;

    void updateCache( const std::map< CameraId, VidooCamera >& camerasMap );
    const Cache& getCache() const;
    std::unique_ptr< Cache::value_type > findItem( const ItemId& itemId ) const;
    size_t size() const;

private:
    Cache m_cache;
};

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
