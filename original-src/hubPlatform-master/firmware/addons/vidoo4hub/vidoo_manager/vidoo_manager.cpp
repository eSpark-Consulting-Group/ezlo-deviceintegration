#include "vidoo4hub/vidoo_manager/vidoo_manager.h"

#include "vidoo4hub/vidoo_manager/vidoo_manager_resources.h"
#include "vidoo4hub/vidoo_manager/vidoo_manager_utils.h"
#include "vidoo4hub/dictionary_manager/dictionary_manager_interface.h"
#include "vidoo4hub/resources/vidoo4hub_resources.h"
#include "vidoo4hub/camera/camera_utils.h"
#include "vidoo4hub/vidoo4hub_exception.h"

#include "cored/model/resources/cored_resources.h"

#include <map>

namespace HubPlatform {
namespace Vidoo4Hub {

VidooManager::VidooManager( IVidooManagerResources& resources, IDictionaryManager& dictionaryManager )
: m_resources( resources )
, m_dictionaryManager( dictionaryManager ) {}

bool VidooManager::updateObjectList( const std::vector< Json >& objectList ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;

    auto objectsItemId = m_resources.getDetectableItemId( DetectableTypes::Type::Object );

    if( objectList.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "Received empty object list: clear dictionary" ) );
        m_dictionaryManager.clearDictionary( objectsItemId );
        return true;
    }

    auto oldDictionary = m_dictionaryManager.readDictionary( objectsItemId );

    auto newDictionary = Json();
    for( auto& objectJson : objectList ) {
        auto objectId = json_get_str( objectJson, MessageFields::objectId );
        auto objectName = json_get_str( objectJson, MessageFields::objectName );
        newDictionary[ objectId ] = JSON( DetectableObjectBaseFields::Name << objectName );
    }

    if( json_contains( newDictionary, "" ) )
        throw Vidoo4HubError( "Failed to update object list: empty object id" );

    return VidooManagerUtils::updateDetectableItemsDictionary(
        m_dictionaryManager, objectsItemId, std::move( oldDictionary ), std::move( newDictionary ) );
}

bool VidooManager::updatePeopleList( const std::vector< Json >& peopleList ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;

    auto personsItemId = m_resources.getDetectableItemId( DetectableTypes::Type::Person );

    if( peopleList.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "Received empty people list: clear dictionary" ) );
        m_dictionaryManager.clearDictionary( personsItemId );
        return true;
    }

    auto oldDictionary = m_dictionaryManager.readDictionary( personsItemId );

    auto newDictionary = Json();
    for( auto& personJson : peopleList ) {
        auto personId = json_get_str( personJson, MessageFields::personId );
        auto personName = json_get_str( personJson, MessageFields::personName );
        newDictionary[ personId ] = JSON( DetectableObjectBaseFields::Name << personName );
    }

    if( json_contains( newDictionary, "" ) )
        throw Vidoo4HubError( "Failed to update people list: empty person id" );

    return VidooManagerUtils::updateDetectableItemsDictionary(
        m_dictionaryManager, personsItemId, std::move( oldDictionary ), std::move( newDictionary ) );
}

bool VidooManager::updateQRCodeList( const std::vector< Json >& qrCodeList ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;

    auto qrCodesItemId = m_resources.getDetectableItemId( DetectableTypes::Type::QRCode );

    if( qrCodeList.empty() ) {
        LOGGER_DEBUG_RAW( cc::debug( "Received empty QR codes list: clear dictionary" ) );
        m_dictionaryManager.clearDictionary( qrCodesItemId );
        return true;
    }

    auto oldDictionary = m_dictionaryManager.readDictionary( qrCodesItemId );

    auto newDictionary = Json();
    for( auto& qrCodeJson : qrCodeList ) {
        auto qrCodeId = json_get_str( qrCodeJson, MessageFields::qrCodeId );
        auto qrCodeName = json_get_str( qrCodeJson, MessageFields::qrCodeName );
        auto qrCodeValue = json_get_str( qrCodeJson, MessageFields::qrCodeValue );
        newDictionary[ qrCodeId ] =
            JSON( DetectableObjectBaseFields::Name << qrCodeName << DetectableObjectBaseFields::Value << qrCodeValue );
    }

    if( json_contains( newDictionary, "" ) )
        throw Vidoo4HubError( "Failed to update QR codes list: empty qrcode id" );

    return VidooManagerUtils::updateDetectableItemsDictionary(
        m_dictionaryManager, qrCodesItemId, std::move( oldDictionary ), std::move( newDictionary ) );
}

bool VidooManager::updateHotzoneList( const std::vector< Json >& hotzoneList ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;

    bool syncRequired = false;

    using HotzoneItemId = std::string;
    using HotzoneDictionary = Json;
    const auto cameras = m_resources.getCameraList();

    std::map< HotzoneItemId, HotzoneDictionary > dictionaryMap;

    for( auto& hotzone : hotzoneList ) {
        auto cameraId = json_get_str( hotzone, MessageFields::cameraId );
        auto hotzoneId = json_get_str( hotzone, MessageFields::hotzoneId );
        auto hotzoneName = json_get_str( hotzone, MessageFields::hotzoneName );

        auto cameraIter = cameras.find( cameraId );
        if( cameraIter == cameras.cend() ) {
            LOGGER_WARN_RAW( cc::warn( "Unknown cameraId in hotzone list: " ) + cc::note( cameraId ) );
            continue;
        }

        if( hotzoneId.empty() )
            throw Vidoo4HubError( "Failed to update hotzone list: empty hotzone id" );

        auto hotzoneJson = CameraUtils::getDefaultHotzoneJson();
        json_add_str( hotzoneJson, CameraHotzoneValueFields::Name, hotzoneName );

        auto& camera = cameraIter->second;
        dictionaryMap[ camera.hotzonesItemId ][ hotzoneId ] = std::move( hotzoneJson );
    }

    for( auto& dictionary : dictionaryMap )
        dictionary.second[ Resources::DefaultValues::defaultHotzoneId ] = CameraUtils::getDefaultHotzoneJson();

    for( auto& camera : cameras ) {
        auto& hotzoneItemId = camera.second.hotzonesItemId;
        auto dictionaryId = dictionaryMap.find( hotzoneItemId );

        if( dictionaryId == dictionaryMap.cend() ) {
            LOGGER_DEBUG_RAW( cc::debug( "Cleaning hotzones for camera id: " ) + cc::note( camera.second.cameraId ) );
            m_dictionaryManager.updateDictionary( hotzoneItemId, CameraUtils::getDefaultHotzoneDictionaryJson() );
            syncRequired = true;
            continue;
        }

        auto oldDictionary = m_dictionaryManager.readDictionary( hotzoneItemId );
        auto& newDictionary = dictionaryId->second;

        auto sortedKeys =
            VidooManagerUtils::ThreeWaySort::sort( VidooManagerUtils::getDictionaryKeys( oldDictionary ),
                                                   VidooManagerUtils::getDictionaryKeys( newDictionary ) );
        bool updateRequired = ( !sortedKeys.missingKeys.empty() ) || ( !sortedKeys.newKeys.empty() );

        Json dictionary;

        for( auto& key : sortedKeys.commonKeys ) {
            auto value = oldDictionary.at( key );
            updateRequired |=
                VidooManagerUtils::updateJsonValue( value, newDictionary.at( key ), CameraHotzoneValueFields::Name );
            dictionary[ key ] = std::move( value );
        }

        for( auto& key : sortedKeys.newKeys ) {
            dictionary[ key ] = newDictionary.at( key );
        }

        if( updateRequired ) {
            m_dictionaryManager.updateDictionary( hotzoneItemId, dictionary );
            syncRequired = true;
        }
    }

    return syncRequired;
}

void VidooManager::motion( const std::string& cameraId,
                           const std::string& hotzoneId,
                           const DetectedObjects& detectedObjects ) {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;
    namespace HotzoneFields = HubPlatform::CoreD::Model::Resources::CameraHotzoneValueFields;

    auto updateHotzoneObjects = []( JsonObject& hotzoneObjects, const DetectedObjects& detectedObjects ) {
        std::set< std::string > detectedHotzoneTypes;

        bool hotzoneUpdated = false;
        for( auto& detectedObject : detectedObjects ) {
            auto& type = detectedObject.first;
            auto& ids = detectedObject.second;
            auto& hotzoneTypeStr = DetectableTypes::getHotzoneTypeName( type );

            auto hotzoneObjectsIt = hotzoneObjects.find( hotzoneTypeStr );
            const bool hotzoneObjectTypeFound = ( hotzoneObjectsIt == hotzoneObjects.cend() );

            if( hotzoneObjectTypeFound && ids.empty() ) {
                hotzoneObjects.erase( hotzoneObjectsIt );
                hotzoneUpdated = true;
                continue;
            }

            if( ids.empty() )
                continue;

            detectedHotzoneTypes.emplace( hotzoneTypeStr );

            if( hotzoneObjectTypeFound ) {
                hotzoneObjects[ hotzoneTypeStr ] = ids;
                hotzoneUpdated = true;
                continue;
            }

            std::set< std::string > existingIds;
            for( const std::string& id : *hotzoneObjectsIt )
                existingIds.emplace( id );

            if( existingIds != ids ) {
                *hotzoneObjectsIt = ids;
                hotzoneUpdated = true;
            }
        }

        // remove missing
        for( auto it = hotzoneObjects.cbegin(); it != hotzoneObjects.cend(); ) {
            const auto& type = it.key();
            if( detectedHotzoneTypes.count( type ) ) {
                ++it;
                continue;
            }
            hotzoneObjects.erase( it++ );
            hotzoneUpdated = true;
        }

        return hotzoneUpdated;
    };

    const auto currentTime = VidooManagerUtils::getTimeISO8601UTC();

    const auto cameras = m_resources.getCameraList();
    const auto& camera = cameras.at( cameraId );

    auto hotzonesDictionary = m_dictionaryManager.readDictionary( camera.hotzonesItemId );

    auto hotzoneIter = hotzonesDictionary.find( hotzoneId );
    if( hotzoneIter == hotzonesDictionary.end() ) {
        LOGGER_WARN_RAW( cc::warn( "Unknown hotzone id: " + hotzoneId + " for camera id: " + cameraId ) );
        return;
    }
    auto& hotzoneObjects = hotzoneIter->at( HotzoneFields::Detected );

    if( updateHotzoneObjects( hotzoneObjects, detectedObjects ) )
        m_dictionaryManager.updateDictionary( camera.hotzonesItemId, hotzonesDictionary );

    if( detectedObjects.empty() )
        return;

    auto updateObjectsDictionary = [&]( Json& dictionary,
                                        const std::string& hotzoneItemId,
                                        const std::string& hotzoneId,
                                        const std::set< std::string > detectedIds ) {
        bool updated = false;
        for( auto& objectId : detectedIds ) {
            auto it = dictionary.find( objectId );
            if( it == dictionary.end() ) {
                LOGGER_WARN_RAW( cc::warn( "Unknown detectable object/person/qrcode id: " + objectId ) );
                continue;
            }

            Json lastHotzone;
            lastHotzone[ DetectableObjectBaseFields::Date ] = currentTime;
            lastHotzone[ DetectableObjectBaseFields::ItemId ] = hotzoneItemId;
            lastHotzone[ DetectableObjectBaseFields::Id ] = hotzoneId;

            dictionary.at( objectId )[ DetectableObjectBaseFields::LastHotzone ] = std::move( lastHotzone );
            updated = true;
        }
        return updated;
    };

    for( auto& detectedObject : detectedObjects ) {
        const auto& type = detectedObject.first;
        const auto& ids = detectedObject.second;
        const auto& detectableItemId = m_resources.getDetectableItemId( type );

        auto objectsDictionary = m_dictionaryManager.readDictionary( detectableItemId );
        if( updateObjectsDictionary( objectsDictionary, camera.hotzonesItemId, hotzoneId, ids ) ) {
            m_dictionaryManager.updateDictionary( detectableItemId, objectsDictionary );
        }
    }
}

void VidooManager::syncUpdatedHotzones() {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;

    using HotzoneItemId = std::string;
    using Dictionary = Json;
    using HotzonesMap = std::map< HotzoneItemId, Dictionary >;

    auto syncDetectableObjects = []( Json& DetectableDictionary, const HotzonesMap& hotzonesMap ) {
        bool updated = false;
        for( auto& element : DetectableDictionary ) {
            auto lastHotzoneIt = element.find( std::string( DetectableObjectBaseFields::LastHotzone ) );
            if( lastHotzoneIt == element.cend() )
                continue;
            auto& lastHotzone = *lastHotzoneIt;
            auto hotzoneItemId = json_get_str( lastHotzone, DetectableObjectBaseFields::ItemId );

            auto cameraHotzonesIt = hotzonesMap.find( hotzoneItemId );
            if( cameraHotzonesIt == hotzonesMap.cend() ) {
                element.erase( lastHotzoneIt );
                updated = true;
                continue;
            }

            auto& cameraHotzones = cameraHotzonesIt->second;
            auto& lastHotzoneId = lastHotzone.at( DetectableObjectBaseFields::Id );
            if( cameraHotzones.find( lastHotzoneId ) == cameraHotzones.cend() ) {
                lastHotzoneId = std::string( DefaultValues::defaultHotzoneId );
                updated = true;
            }
        }
        return updated;
    };

    const auto cameras = m_resources.getCameraList();

    HotzonesMap hotzonesMap;
    for( auto& cameraIt : cameras ) {
        auto& camera = cameraIt.second;
        auto hotzones = m_dictionaryManager.readDictionary( camera.hotzonesItemId );
        hotzonesMap.emplace( camera.hotzonesItemId, std::move( hotzones ) );
    }

    for( auto& type : DetectableTypes::getKnownTypes() ) {
        const auto itemId = m_resources.getDetectableItemId( type );
        auto itemDictionary = m_dictionaryManager.readDictionary( itemId );
        if( syncDetectableObjects( itemDictionary, hotzonesMap ) ) {
            m_dictionaryManager.updateDictionary( itemId, itemDictionary );
        }
    }
}

void VidooManager::syncUpdatedDetectableObjects() {
    using namespace HubPlatform::CoreD::Model::Resources;
    using namespace Vidoo4Hub::Resources;

    auto updateHotzoneObjectList = []( JsonArray& hotzoneObjects, const Json& knownObjects ) {
        bool updated = false;

        Json::array_t syncedObjects;
        syncedObjects.reserve( hotzoneObjects.size() );

        for( const std::string& id : hotzoneObjects ) {
            if( knownObjects.find( id ) != knownObjects.cend() ) {
                syncedObjects.emplace_back( id );
            }
            else {
                updated = true;
            }
        }

        if( updated ) {
            hotzoneObjects = std::move( syncedObjects );
        }
        return updated;
    };

    std::map< DetectableTypes::Type, JsonObject > knownObjectsMap;
    for( auto& type : DetectableTypes::getKnownTypes() ) {
        const auto itemId = m_resources.getDetectableItemId( type );
        knownObjectsMap.emplace( type, m_dictionaryManager.readDictionary( itemId ) );
    }

    const auto cameras = m_resources.getCameraList();

    for( auto& cameraIt : cameras ) {
        auto& camera = cameraIt.second;
        auto cameraHotzones = m_dictionaryManager.readDictionary( camera.hotzonesItemId );

        bool cameraHotzonesUpdated = false;
        for( auto& hotzone : cameraHotzones ) {
            auto& hotzoneObjectsMap = hotzone.at( CameraHotzoneValueFields::Detected );

            for( auto& type : DetectableTypes::getKnownTypes() ) {
                const auto& hotzoneObjectType = DetectableTypes::getHotzoneTypeName( type );
                auto it = hotzoneObjectsMap.find( hotzoneObjectType );
                if( it == hotzoneObjectsMap.cend() )  // nothing to sync
                    continue;
                auto& hotzoneObjects = it.value();
                cameraHotzonesUpdated |= updateHotzoneObjectList( hotzoneObjects, knownObjectsMap.at( type ) );
            }
        }

        if( cameraHotzonesUpdated ) {
            LOGGER_DEBUG_RAW( cc::debug( "Syncing hotzones detected objects for camera: " ) +
                              cc::note( camera.cameraId ) );
            m_dictionaryManager.updateDictionary( camera.hotzonesItemId, cameraHotzones );
        }
    }
}

}  // namespace Vidoo4Hub
}  // namespace HubPlatform
