#include "scenes_manager/scenes_processor/meshbot_templates/templates_manager/templates_manager.h"
#include "scenes_manager/scenes_processor/meshbot_templates/utils/utils.h"
#include "scenes_manager/scenes_processor/meshbot_templates/exceptions.h"
#include "scenes_manager/scenes_processor/meshbot_templates/meshbot_builder/meshbot_builder.h"

#include "json_utils/json_utils.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "base_utils/ezlodb.h"
#include "common_utils/exception_utils.h"

#include <algorithm>

namespace HubPlatform {
namespace ScenesManager {
namespace MeshbotTemplates {

namespace TemplatesManagerUtils {

bool isTriggerValid( const Json& device, const Json& item, const Types::MeshbotTemplate& meshbotTemplate ) {
    auto result = Utils::findMeshbotTriggers( { device }, { item }, meshbotTemplate );
    return !result.empty();
}

std::map< std::string, const Json* > buildDevicesMap( const std::vector< Json >& devices ) {
    std::map< std::string, const Json* > map;
    for( auto& device : devices ) {
        map.emplace( Utils::getDeviceId( device ), &device );
    }
    return map;
}

std::map< std::string, const Json* > buildItemsMap( const std::vector< Json >& items ) {
    std::map< std::string, const Json* > map;
    for( auto& item : items ) {
        map.emplace( Utils::getItemId( item ), &item );
    }
    return map;
}

std::map< std::string, const Types::MeshbotTemplate* > buildTemplatesMap(
    const std::vector< Types::MeshbotTemplate >& templates ) {
    std::map< std::string, const Types::MeshbotTemplate* > map;
    for( auto& t : templates ) {
        map.emplace( t.uuid, &t );
    }
    return map;
}

}  // namespace TemplatesManagerUtils

TemplatesManager::TemplatesManager( IDbClient& dbClient, std::unique_ptr< HandlerInterface > handler )
: m_dbClient( dbClient )
, m_handler( std::move( handler ) ) {}

TemplatesManager::~TemplatesManager() = default;

void TemplatesManager::handleSyncMeshbotTemplates() {
    syncMeshbotTemplates();
}

void TemplatesManager::syncMeshbotTemplates() {
    LOGGER_INFO( cc::info( "Syncing meshbot template scenes..." ) );

    size_t counterNewScenes = 0;
    size_t counterUpdatedScenes = 0;
    size_t counterUserModifiedScenes = 0;

    const auto templates = m_handler->getMeshbotTemplates();
    const auto items = m_handler->getItems();
    const auto devices = m_handler->getDevices();

    MeshbotBuilder::Data builderData;
    builderData.devices = devices;
    builderData.items = items;
    builderData.tcdsGeneralDataVersion = m_handler->getTcdsGeneralDataVersion();
    builderData.tcdsMeshbotsDataVersion = m_handler->getTcdsMeshbotsDataVersion();
    builderData.controllerSerial = m_handler->getControllerSerial();

    MeshbotBuilder meshbotBuilder( std::move( builderData ) );

    const auto templatesMap = TemplatesManagerUtils::buildTemplatesMap( templates );
    const auto devicesMap = TemplatesManagerUtils::buildDevicesMap( devices );
    const auto itemsMap = TemplatesManagerUtils::buildItemsMap( items );

    auto previousBindings = Utils::readAllTemplateBindings( m_dbClient );
    std::map< BindingKey, Types::TemplateBinding* > previousBindingsMap;
    std::set< BindingKey > ignoreBindingsSet;
    std::set< std::string > removedScenes;

    for( auto& binding : previousBindings ) {
        auto bindingKey = BindingKey( binding.templateUUID, binding.triggerDeviceId, binding.triggerItemId );
        previousBindingsMap.emplace( bindingKey, &binding );

        if( m_handler->isSceneModified( binding.sceneId ) ) {
            ignoreBindingsSet.emplace( bindingKey );
            ++counterUserModifiedScenes;
            continue;
        }

        auto templateIt = templatesMap.find( binding.templateUUID );
        if( templateIt == templatesMap.cend() ) {
            LOGGER_DEBUG( "Remove deprecated binding with sceneId" + cc::note( binding.sceneId ) );
            removeBinding( binding );
            removedScenes.emplace( binding.sceneId );
            continue;
        }

        auto& meshbotTemplate = *( templateIt->second );

        if( meshbotTemplate.version != binding.templateVersion ) {
            LOGGER_DEBUG( "Remove binding with outdated version, sceneId" + cc::note( binding.sceneId ) );
            removeBinding( binding );
            removedScenes.emplace( binding.sceneId );
            continue;
        }

        auto deviceIt = devicesMap.find( binding.triggerDeviceId );
        if( deviceIt == devicesMap.cend() ) {
            LOGGER_DEBUG( "Remove binding with missing device, sceneId" + cc::note( binding.sceneId ) );
            removeBinding( binding );
            removedScenes.emplace( binding.sceneId );
            continue;
        }

        auto itemIt = itemsMap.find( binding.triggerItemId );
        if( itemIt == itemsMap.cend() ) {
            LOGGER_DEBUG( "Remove binding with missing item, sceneId" + cc::note( binding.sceneId ) );
            removeBinding( binding );
            removedScenes.emplace( binding.sceneId );
            continue;
        }

        if( !TemplatesManagerUtils::isTriggerValid( *( deviceIt->second ), *( itemIt->second ), meshbotTemplate ) ) {
            LOGGER_DEBUG( "Remove binding with outdated triggers, sceneId" + cc::note( binding.sceneId ) );
            removeBinding( binding );
            removedScenes.emplace( binding.sceneId );
            continue;
        }

        ignoreBindingsSet.emplace( bindingKey );
    }

    for( auto& meshbotTemplate : templates ) {
        auto triggers = Utils::findMeshbotTriggers( devices, items, meshbotTemplate );

        for( auto& trigger : triggers ) {
            Types::TemplateBinding binding;
            BindingKey bindingKey( meshbotTemplate.uuid, trigger.deviceId, trigger.itemId );

            if( ignoreBindingsSet.count( bindingKey ) ) {
                continue;
            }

            auto previousBindingIt = previousBindingsMap.find( bindingKey );
            if( previousBindingIt != previousBindingsMap.cend() ) {
                binding = *( previousBindingIt->second );
                binding.templateVersion = meshbotTemplate.version;
                ++counterUpdatedScenes;
            }
            else {
                binding.bindingId = Db::genOid();
                binding.templateUUID = meshbotTemplate.uuid;
                binding.templateVersion = meshbotTemplate.version;
                binding.triggerDeviceId = trigger.deviceId;
                binding.triggerItemId = trigger.itemId;
                binding.sceneId = Db::genOid();
                ++counterNewScenes;
            }

            if( !createScene( meshbotBuilder, binding.sceneId, meshbotTemplate, trigger ) ) {
                continue;
            }

            Utils::storeTemplateBinding( m_dbClient, binding );
            removedScenes.erase( binding.sceneId );
        }
    }

    size_t counterProcessedScenes =
        counterNewScenes + counterUpdatedScenes + counterUserModifiedScenes + removedScenes.size();
    LOGGER_INFO( cc::info( "Meshbot template scenes updated. Processed total: " ) +
                 cc::num10( counterProcessedScenes ) + cc::info( ", updated: " ) + cc::num10( counterUpdatedScenes ) +
                 cc::info( ", new: " ) + cc::num10( counterNewScenes ) + cc::info( ", user_modified: " ) +
                 cc::num10( counterUserModifiedScenes ) + cc::info( ", deleted: " ) +
                 cc::num10( removedScenes.size() ) );
}

TemplatesManager::Error TemplatesManager::handleMigrateMeshbotTemplates( const int generalDataVersion,
                                                                         const int generalDataFileVersion,
                                                                         const int meshbotsDataVersion,
                                                                         const int meshbotsFileVersion ) {
    LOGGER_INFO( cc::info( "Migrating meshbot template scenes..." ) );
    size_t scenesNum = 0;

    const auto templates = m_handler->getMeshbotTemplates( meshbotsDataVersion, meshbotsFileVersion );
    const auto items = m_handler->getItems();
    const auto devices = m_handler->getDevices();

    MeshbotBuilder::Data builderData;
    builderData.devices = devices;
    builderData.items = items;
    builderData.tcdsGeneralDataVersion = generalDataVersion;
    builderData.tcdsMeshbotsDataVersion = meshbotsDataVersion;
    builderData.controllerSerial = m_handler->getControllerSerial();

    MeshbotBuilder meshbotBuilder( std::move( builderData ) );

    std::string errorString;

    const auto previousBindings = Utils::readAllTemplateBindings( m_dbClient );
    std::vector< Types::TemplateBinding > newBindings;

    for( auto& meshbotTemplate : templates ) {
        auto triggers = Utils::findMeshbotTriggers( devices, items, meshbotTemplate );

        for( auto& trigger : triggers ) {
            Types::TemplateBinding binding;

            binding.bindingId = Db::genOid();
            binding.templateUUID = meshbotTemplate.uuid;
            binding.templateVersion = meshbotTemplate.version;
            binding.triggerDeviceId = trigger.deviceId;
            binding.triggerItemId = trigger.itemId;
            binding.sceneId = Db::genOid();

            if( !createScene( meshbotBuilder, binding.sceneId, meshbotTemplate, trigger ) ) {
                errorString = "Unable to create scene for template UUID: " + binding.templateUUID +
                              ", device id: " + binding.triggerDeviceId + ", item id: " + binding.triggerItemId;
                break;
            }
            Utils::storeTemplateBinding( m_dbClient, binding );
            newBindings.emplace_back( binding );
            ++scenesNum;
        }

        if( !errorString.empty() )
            break;
    }

    if( !errorString.empty() ) {
        LOGGER_ERROR( cc::error( errorString ) );
        for( auto& binding : newBindings ) {
            removeBinding( binding );
        }
        return errorString;
    }

    m_handler->switchMeshbotsVersion( meshbotsDataVersion, meshbotsFileVersion );
    m_handler->switchGeneralDataVersion( generalDataVersion, generalDataFileVersion );

    for( auto& binding : previousBindings ) {
        removeBinding( binding );
    }

    LOGGER_INFO( cc::info( "Migration of meshbot template scenes finished, scenes: " ) + cc::num10( scenesNum ) );

    return {};
}

void TemplatesManager::handleDeviceUpdated( const std::string& deviceId ) {
    LOGGER_INFO( cc::info( "Handling device update, device id: " ) + cc::note( deviceId ) );
    Types::TemplateBinding query;
    query.triggerDeviceId = deviceId;
    auto bindings = Utils::queryBindings( m_dbClient, query );
    updateBindings( bindings );
}

void TemplatesManager::handleItemUpdated( const std::string& itemId ) {
    LOGGER_INFO( cc::info( "Handling item update, item id: " ) + cc::note( itemId ) );
    Types::TemplateBinding query;
    query.triggerItemId = itemId;
    auto bindings = Utils::queryBindings( m_dbClient, query );
    updateBindings( bindings );
}

void TemplatesManager::updateBindings( const std::vector< Types::TemplateBinding >& bindings ) {
    const auto templates = m_handler->getMeshbotTemplates();
    const auto items = m_handler->getItems();
    const auto devices = m_handler->getDevices();

    MeshbotBuilder::Data builderData;
    builderData.devices = devices;
    builderData.items = items;
    builderData.tcdsGeneralDataVersion = m_handler->getTcdsGeneralDataVersion();
    builderData.tcdsMeshbotsDataVersion = m_handler->getTcdsMeshbotsDataVersion();
    builderData.controllerSerial = m_handler->getControllerSerial();

    MeshbotBuilder meshbotBuilder( std::move( builderData ) );

    const auto templatesMap = TemplatesManagerUtils::buildTemplatesMap( templates );
    const auto devicesMap = TemplatesManagerUtils::buildDevicesMap( devices );
    const auto itemsMap = TemplatesManagerUtils::buildItemsMap( items );

    bool syncRequired = false;

    for( auto& binding : bindings ) {
        auto templateIt = templatesMap.find( binding.templateUUID );
        auto deviceIt = devicesMap.find( binding.triggerDeviceId );
        auto itemIt = itemsMap.find( binding.triggerItemId );

        if( templateIt == templatesMap.cend() ) {
            LOGGER_WARN( cc::warn( "Data out-of-sync: template already removed for sceneId: " ) +
                         cc::note( binding.sceneId ) );
            syncRequired = true;
            continue;
        }

        if( deviceIt == devicesMap.cend() ) {
            LOGGER_WARN( cc::warn( "Data out-of-sync: device already removed for sceneId: " ) +
                         cc::note( binding.sceneId ) );
            syncRequired = true;
            continue;
        }

        if( itemIt == itemsMap.cend() ) {
            LOGGER_WARN( cc::warn( "Data out-of-sync: item already removed for sceneId: " ) +
                         cc::note( binding.sceneId ) );
            syncRequired = true;
            continue;
        }

        auto& meshbotTemplate = *( templateIt->second );
        auto& device = *( deviceIt->second );
        auto& item = *( itemIt->second );

        removeBinding( binding );

        if( TemplatesManagerUtils::isTriggerValid( device, item, meshbotTemplate ) ) {
            Types::MeshbotTrigger trigger;
            trigger.deviceId = Utils::getDeviceId( device );
            trigger.device = device;
            trigger.itemId = Utils::getItemId( item );
            trigger.item = item;

            createScene( meshbotBuilder, binding.sceneId, meshbotTemplate, trigger );
            Utils::storeTemplateBinding( m_dbClient, binding );
        }
    }

    if( syncRequired ) {
        syncMeshbotTemplates();
    }
}

bool TemplatesManager::createScene( const MeshbotBuilder& meshbotBuilder,
                                    const std::string& sceneId,
                                    const Types::MeshbotTemplate& meshbotTemplate,
                                    const Types::MeshbotTrigger& trigger ) {
    LOGGER_TRACE( cc::trace( "Creating scene from template, scene id: " ) + cc::note( sceneId ) );
    Json scene;
    try {
        scene = meshbotBuilder.buildMeshbot(
            meshbotTemplate.localMeshbot, sceneId, trigger.deviceId, trigger.itemId, meshbotTemplate.version );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to parse meshbot template with uuid: " ) + cc::note( meshbotTemplate.uuid ) +
                      cc::error( ", error: " ) + cc::note( CommonUtils::whatError( e ) ) );
        return false;
    }

    try {
        m_handler->createScene( scene );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to create scene from meshbot template with uuid: " ) +
                      cc::note( meshbotTemplate.uuid ) + cc::error( ", error: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
        return false;
    }

    return true;
}

void TemplatesManager::removeBinding( const Types::TemplateBinding& binding ) {
    try {
        LOGGER_TRACE( cc::trace( "Remove binding for scene id: " ) + cc::note( binding.sceneId ) );
        m_handler->deleteScene( binding.sceneId );
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "Unable to delete scene for meshbot template with uuid: " ) +
                      cc::note( binding.templateUUID ) + cc::error( ", error: " ) +
                      cc::note( CommonUtils::whatError( e ) ) );
    }
    Utils::deleteTemplateBinding( m_dbClient, binding.bindingId );
};

}  // namespace MeshbotTemplates
}  // namespace ScenesManager
}  // namespace HubPlatform
