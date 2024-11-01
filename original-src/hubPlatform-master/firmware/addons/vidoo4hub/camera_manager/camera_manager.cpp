#include "vidoo4hub/camera_manager/camera_manager.h"
#include "vidoo4hub/core_manager/core_manager_interface.h"
#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/vidoo4hub_exception.h"

#include "common_utils/cpp14.h"
#include "log_utils/ezlo_log.h"
#include "tiny/tiny_consolecolors.h"

#include <algorithm>

namespace HubPlatform {
namespace Vidoo4Hub {
namespace {
const char* const streamItemName = "camera_stream";
const char* const streamName = "camera_stream";
}  // namespace

CameraManager::CameraManager( Services::Service& service,
                              ICoreManager& coreManager,
                              IStreamManager& streamManager,
                              std::string vidooDeviceId,
                              std::string coreDGatewayId )
: m_service( service )
, m_coreManager( coreManager )
, m_streamManager( streamManager )
, m_vidooDeviceId( std::move( vidooDeviceId ) )
, m_gatewayId( std::move( coreDGatewayId ) ) {
    reloadCameras();
}

void CameraManager::reloadCameras() {
    m_cameraList = CameraUtils::readAllCamerasFromDB( m_service.getDBClient() );
    m_itemsCache.updateCache( m_cameraList );
}

void CameraManager::addCamera( std::string cameraId,
                               std::string cameraSubcategory,
                               std::string cameraName,
                               std::string rtspURL ) {
    using namespace CameraUtils;
    if( cameraId.empty() )
        throw Vidoo4HubError( "Failed to add camera: empty camera id" );
    if( cameraSubcategory.empty() )
        throw Vidoo4HubError( "Failed to add camera: empty camera subcategory" );
    if( !CameraUtils::isKnownCameraSubcategory( cameraSubcategory ) )
        throw Vidoo4HubError( "Failed to add camera: unknown camera subcategory: " + cameraSubcategory );
    if( cameraName.empty() )
        throw Vidoo4HubError( "Failed to add camera: empty camera name" );
    if( rtspURL.empty() )
        throw Vidoo4HubError( "Failed to add camera: empty rtsp url" );

    VidooCamera camera;
    camera.cameraId = cameraId;
    camera.cameraSubcategory = cameraSubcategory;
    camera.cameraName = cameraName;
    camera.rtspURL = rtspURL;

    try {
        auto& core = m_coreManager.core();

        camera.cameraDeviceId = createCameraDevice( core, m_gatewayId, cameraSubcategory, cameraName );
        camera.sensorDeviceId = createMotionSensorDevice( core, m_gatewayId, camera.cameraDeviceId, cameraName );
        camera.securityThreatItemId = createSecurityThreatItem( core, camera.sensorDeviceId );
        camera.motionItemId = createMotionItem( core, camera.sensorDeviceId );
        core.setDeviceArmed( camera.sensorDeviceId, true );

        camera.hotzonesItemId = createHotzonesItem( core, camera.cameraDeviceId );

        camera.snapshotItemId = CameraUtils::createSnapshotItem( core, camera.cameraDeviceId );
        camera.makeRecordingItemId = CameraUtils::createMakeRecordingItem( core, camera.cameraDeviceId );
        camera.stopRecordingItemId = CameraUtils::createStopRecordingItem( core, camera.cameraDeviceId );

        camera.streamId = CameraUtils::createStreamItem( core, camera.cameraDeviceId, streamItemName );
        m_streamManager.addCameraStream( camera.streamId, streamName, camera.rtspURL );
        m_streamManager.createLiveRTSPStream( camera.streamId );

        CameraUtils::storeCameraToDB( m_service.getDBClient(), camera );

        m_cameraList.emplace( camera.cameraId, camera );
        Log::info( cc::info( "Camera with id: " + cameraId + " added" ) );
        m_itemsCache.updateCache( m_cameraList );
    }
    catch( const std::exception& e ) {
        Log::warn( cc::warn( "Failed to create camera: " + std::string( e.what() ) ) );
        deleteCameraResources( camera );
        CameraUtils::deleteCameraFromDB( m_service.getDBClient(), camera.cameraId );
        m_cameraList.erase( camera.cameraId );
    }
}

void CameraManager::updateCamera( std::string cameraId,
                                  std::string cameraSubcategory,
                                  std::string cameraName,
                                  std::string rtspURL ) {
    using namespace CameraUtils;

    if( cameraId.empty() )
        throw Vidoo4HubError( "Failed to update camera: empty camera id" );
    if( cameraSubcategory.empty() )
        throw Vidoo4HubError( "Failed to update camera: empty camera subcategory" );
    if( !CameraUtils::isKnownCameraSubcategory( cameraSubcategory ) )
        throw Vidoo4HubError( "Failed to update camera: unknown camera subcategory: " + cameraSubcategory );
    if( cameraName.empty() )
        throw Vidoo4HubError( "Failed to update camera: empty camera name" );
    if( rtspURL.empty() )
        throw Vidoo4HubError( "Failed to update camera: empty rtsp url" );

    auto cameraIt = m_cameraList.find( cameraId );
    if( cameraIt == m_cameraList.cend() )
        throw Vidoo4HubError( "Failed to update camera: unknown id: " + cameraId );
    auto& camera = cameraIt->second;

    if( cameraSubcategory != camera.cameraSubcategory ) {
        deleteCamera( camera.cameraId );
        addCamera( camera.cameraId, camera.cameraSubcategory, camera.cameraName, camera.rtspURL );
    }
    else {
        auto& core = m_coreManager.core();

        if( cameraName != camera.cameraName ) {
            camera.cameraName = cameraName;
            core.updateDeviceName( camera.cameraDeviceId, camera.cameraName );
            auto motionSensorName = createMotionSensorName( camera.cameraName );
            core.updateDeviceName( camera.sensorDeviceId, motionSensorName );
            CameraUtils::storeCameraToDB( m_service.getDBClient(), camera );
        }
        if( rtspURL != camera.rtspURL ) {
            camera.rtspURL = rtspURL;
            m_streamManager.deleteLiveRTSPStream( camera.streamId );
            m_streamManager.removeCameraStream( camera.streamId );

            m_streamManager.addCameraStream( camera.streamId, streamName, camera.rtspURL );
            m_streamManager.createLiveRTSPStream( camera.streamId );
            CameraUtils::storeCameraToDB( m_service.getDBClient(), camera );
        }
    }
}

void CameraManager::deleteCamera( const std::string& cameraId ) {
    Log::debug( cc::debug( "Try to delete camera with id: " + cameraId ) );
    auto it = m_cameraList.find( cameraId );
    if( it == m_cameraList.cend() ) {
        Log::warn( cc::warn( "Unable to delete nonexistent camera with id: " + cameraId ) );
        return;
    }
    VidooCamera camera = it->second;
    deleteCameraResources( camera );

    CameraUtils::deleteCameraFromDB( m_service.getDBClient(), camera.cameraId );
    m_cameraList.erase( camera.cameraId );
    Log::info( cc::info( "Camera with id: " + cameraId + " deleted" ) );
    m_itemsCache.updateCache( m_cameraList );
}

void CameraManager::deleteCameraResources( const VidooCamera& camera ) {
    Log::debug( cc::debug( "Deleting resources for camera with id: " + camera.cameraId ) );
    auto& core = m_coreManager.core();

    auto removeItem = [&]( const std::string& id ) {
        if( !id.empty() )
            CameraUtils::removeItem( core, id );
    };

    if( !camera.streamId.empty() ) {
        m_streamManager.deleteLiveRTSPStream( camera.streamId );
        m_streamManager.removeCameraStream( camera.streamId );
        CameraUtils::removeItem( core, camera.streamId );
    }

    removeItem( camera.stopRecordingItemId );
    removeItem( camera.makeRecordingItemId );
    removeItem( camera.snapshotItemId );
    removeItem( camera.hotzonesItemId );
    removeItem( camera.motionItemId );
    removeItem( camera.securityThreatItemId );

    if( !camera.sensorDeviceId.empty() ) {
        CameraUtils::removeDevice( core, camera.sensorDeviceId );
    }

    if( !camera.cameraDeviceId.empty() ) {
        CameraUtils::removeDevice( core, camera.cameraDeviceId );
    }

    if( !camera.cameraId.empty() ) {
        CameraUtils::deleteCameraFromDB( m_service.getDBClient(), camera.cameraId );
    }
}

void CameraManager::updateCameraList( const std::vector< Json >& cameraJsonList ) {
    std::map< CameraId, VidooCamera > cameraList;

    for( auto& cameraJson : cameraJsonList ) {
        auto camera = CameraUtils::cameraFromJson( cameraJson );
        auto id = camera.cameraId;

        if( camera.cameraId.empty() ) {
            throw Vidoo4HubError( "Invalid camera list: empty camera id" );
        }
        if( camera.cameraSubcategory.empty() ) {
            throw Vidoo4HubError( "Invalid camera list: empty camera subcategory" );
        }
        if( camera.cameraName.empty() ) {
            throw Vidoo4HubError( "Invalid camera list: empty camera name for camera (id " + id + ")" );
        }
        if( camera.rtspURL.empty() ) {
            throw Vidoo4HubError( "Invalid camera list: empty rtsp url for camera (id " + id + ")" );
        }

        cameraList.emplace( id, std::move( camera ) );
    }

    std::map< CameraId, VidooCamera* > camerasToAdd;
    std::set< std::string > camerasToDelete;

    for( auto& element : m_cameraList ) {
        auto& camera = element.second;
        if( cameraList.find( camera.cameraId ) == cameraList.cend() )
            camerasToDelete.emplace( camera.cameraId );
    }

    for( auto& camera : cameraList ) {
        auto& receivedCamera = camera.second;

        auto it = m_cameraList.find( receivedCamera.cameraId );
        if( it == m_cameraList.cend() ) {
            addCamera( receivedCamera.cameraId,
                       receivedCamera.cameraSubcategory,
                       receivedCamera.cameraName,
                       receivedCamera.rtspURL );
        }
        else {
            updateCamera( receivedCamera.cameraId,
                          receivedCamera.cameraSubcategory,
                          receivedCamera.cameraName,
                          receivedCamera.rtspURL );
        }
    }

    for( auto& cameraId : camerasToDelete )
        deleteCamera( cameraId );

    m_itemsCache.updateCache( m_cameraList );
}

void CameraManager::setMotionStarted( const std::string& cameraId ) {
    auto it = m_cameraList.find( cameraId );
    if( it == m_cameraList.cend() ) {
        Log::warn( cc::warn( "Unable to set motion started: unknown camera id: " + cameraId ) );
        return;
    }

    auto& core = m_coreManager.core();
    auto securityThreatItemId = it->second.securityThreatItemId;
    CameraUtils::setMotionStarted( core, securityThreatItemId );

    auto motionItemId = it->second.motionItemId;
    CameraUtils::setMotionStarted( core, motionItemId );
}

void CameraManager::setMotionEnded( const std::string& cameraId ) {
    auto it = m_cameraList.find( cameraId );
    if( it == m_cameraList.cend() ) {
        Log::warn( cc::warn( "Unable to set motion ended: unknown camera id: " + cameraId ) );
        return;
    }

    auto& core = m_coreManager.core();
    auto securityThreatItemId = it->second.securityThreatItemId;
    CameraUtils::setMotionEnded( core, securityThreatItemId );

    auto motionItemId = it->second.motionItemId;
    CameraUtils::setMotionEnded( core, motionItemId );
}

void CameraManager::handleStreamStarted( const std::string& streamId, const std::string& rtspURL ) {
    using ItemType = CameraItemsCache::ItemType;
    auto cacheData = m_itemsCache.findItem( streamId );
    if( !cacheData ) {
        LOGGER_TRACE_RAW( cc::trace( "Camera stream not found: " + streamId ) );
        return;
    }

    auto& itemType = cacheData->second.m_itemType;
    if( itemType != ItemType::CameraStream )
        throw Vidoo4HubError( "Stream id is not a streaming item" );

    CameraUtils::setStreamStarted( m_coreManager.core(), streamId, rtspURL );
}

void CameraManager::handleStreamStopped( const std::string& streamId ) {
    using ItemType = CameraItemsCache::ItemType;
    auto cacheData = m_itemsCache.findItem( streamId );
    if( !cacheData ) {
        LOGGER_TRACE_RAW( cc::trace( "Camera stream not found: " + streamId ) );
        return;
    }

    auto& itemType = cacheData->second.m_itemType;
    if( itemType != ItemType::CameraStream )
        throw Vidoo4HubError( "Stream id is not a streaming item" );

    CameraUtils::setStreamStopped( m_coreManager.core(), streamId );
}

const std::map< CameraManager::CameraId, VidooCamera >& CameraManager::getCameras() {
    return m_cameraList;
}

const CameraItemsCache& CameraManager::getCameraItemsCache() {
    return m_itemsCache;
}

void CameraManager::cleanItemValues() {
    LOGGER_DEBUG_RAW( cc::debug( "Cleaning cameras item values" ) );
    for( auto& element : m_cameraList ) {
        auto& camera = element.second;
        CameraUtils::cleanCameraItemValues( m_coreManager.core(), camera );
    }
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
