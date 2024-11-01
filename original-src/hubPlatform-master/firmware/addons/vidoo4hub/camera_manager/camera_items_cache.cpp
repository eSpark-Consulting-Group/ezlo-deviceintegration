#include "vidoo4hub/camera_manager/camera_items_cache.h"

#include "common_utils/cpp14.h"

namespace HubPlatform {
namespace Vidoo4Hub {

CameraItemsCache::Data::Data( CameraId cameraId, ItemType itemType )
: m_cameraId( std::move( cameraId ) )
, m_itemType( itemType ) {}

void CameraItemsCache::updateCache( const std::map< CameraId, VidooCamera >& camerasMap ) {
    m_cache.clear();
    for( auto& element : camerasMap ) {
        auto& camera = element.second;
        m_cache.emplace( camera.securityThreatItemId, Data{ camera.cameraId, ItemType::SecurityThreat } );
        m_cache.emplace( camera.motionItemId, Data{ camera.cameraId, ItemType::Motion } );
        m_cache.emplace( camera.hotzonesItemId, Data{ camera.cameraId, ItemType::Hotzones } );
        m_cache.emplace( camera.snapshotItemId, Data{ camera.cameraId, ItemType::Snapshot } );
        m_cache.emplace( camera.makeRecordingItemId, Data{ camera.cameraId, ItemType::MakeRecording } );
        m_cache.emplace( camera.stopRecordingItemId, Data{ camera.cameraId, ItemType::StopRecording } );
        m_cache.emplace( camera.streamId, Data{ camera.cameraId, ItemType::CameraStream } );
    }
}

const CameraItemsCache::Cache& CameraItemsCache::getCache() const {
    return m_cache;
}

std::unique_ptr< CameraItemsCache::Cache::value_type > CameraItemsCache::findItem( const ItemId& itemId ) const {
    auto it = m_cache.find( itemId );
    return ( it == m_cache.end() ) ? nullptr : std::make_unique< Cache::value_type >( *it );
}

size_t CameraItemsCache::size() const {
    return m_cache.size();
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
