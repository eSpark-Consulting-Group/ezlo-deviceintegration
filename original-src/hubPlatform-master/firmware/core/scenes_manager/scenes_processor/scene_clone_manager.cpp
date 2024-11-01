#include "scenes_manager/scenes_processor/scene_clone_manager.h"
#include "scenes_manager/scenes_processor/resources.h"
#include "scenes_manager/scenes_engine/model/model_resources.h"

#include "database_client/dbqueryparser.h"
#include "database_client/db_tables_names.h"
#include "base_utils/ezlodb.h"
#include "jsonrpc/jsonrpc.h"
#include <utility>
#include <string>

namespace HubPlatform {
namespace ScenesManager {

const auto ID = "_id";

ScenesCloneManager::ScenesCloneManager( IDbClient& client, JsonObject& scene, SceneEngine::SceneManager& sceneManager )
: m_dbClient( client )
, m_clonedScene( scene )
, m_manager( sceneManager ) {}

IDbClient& ScenesCloneManager::getDBClient() {
    return m_dbClient;
}

const IDbClient& ScenesCloneManager::getDBClient() const {
    return m_dbClient;
}

std::string ScenesCloneManager::createClonedName( const std::string& name ) {
    int i = 0;
    while( true ) {
        i++;
        std::string nextName = name;
        nextName += " - clone";
        nextName += "(";
        nextName += std::to_string( i );
        nextName += ")";

        auto hasScene = getDBClient().findOne( HaScenesDBName, JSON( "name" << nextName ), nullptr );
        if( hasScene.empty() ) {
            return nextName;
        }
    }

    LOGGER_ERROR( "Error in create cloned scene name" );
    return "";
}

void ScenesCloneManager::replaceAllReferencesByPlaceholders() {
    std::map< std::string, std::string > mapper;
    replaceAndStorePlaceholdersForConditionBlocks( EventBlockTypes::Exceptions, mapper );
    replaceIdForBlocks( EventBlockTypes::When, true );
    replaceIdForBlocks( ActionBlockTypes::Then );
    replaceIdForBlocks( ActionBlockTypes::Else );
    applyPlaceholdersForActionBlocks( ActionBlockTypes::Then, mapper );
    applyPlaceholdersForActionBlocks( ActionBlockTypes::Else, mapper );
    handleInnerSceneIdRelationsInActionBlocks( ActionBlockTypes::Then );
    handleInnerSceneIdRelationsInActionBlocks( ActionBlockTypes::Else );
}

void ScenesCloneManager::applyPlaceholdersForActionBlocks( const std::string& blockType,
                                                           std::map< std::string, std::string >& mapper ) {
    using namespace ActionFields::Fields;
    JsonArray blocks;
    JsonArrayBuilder thenBlocksWithPlaceholders;
    json_get_arr( m_clonedScene, blockType, blocks );

    for( auto& block : blocks ) {
        JsonObject control;
        json_get_obj( block, Control, control );
        if( !control.empty() ) {
            JsonObject exception;
            json_get_obj( control, Exception, exception );
            if( !exception.empty() ) {
                auto blockId = json_get_str( exception, BlockId );
                if( !blockId.empty() ) {
                    auto placeholder = mapper[ blockId ];
                    if( !placeholder.empty() ) {
                        exception[ BlockId ] = placeholder;
                    }
                }
                control[ Exception ] = exception;
            }
            block[ Control ] = control;
        }
        thenBlocksWithPlaceholders << block;
    }
    m_clonedScene[ blockType ] = thenBlocksWithPlaceholders;
}

void ScenesCloneManager::replaceAndStorePlaceholdersForConditionBlocks( const std::string& blockType,
                                                                        std::map< std::string, std::string >& mapper ) {
    JsonArrayBuilder exceptionsWithPlaceholders;
    JsonArray whenBlocks;
    json_get_arr( m_clonedScene, blockType, whenBlocks );

    const auto PlaceholderPrefix = "${block";
    int count = 0;
    for( auto& block : whenBlocks ) {
        auto id = json_get_str( block, ID );
        if( !id.empty() ) {
            count++;
            std::string placeholder = PlaceholderPrefix;
            placeholder += std::to_string( count );
            placeholder += "}";
            mapper[ id ] = placeholder;
            block[ ID ] = placeholder;
        }

        exceptionsWithPlaceholders << block;
    }

    m_clonedScene[ blockType ] = exceptionsWithPlaceholders;
}

void ScenesCloneManager::replaceIdForBlocks( const std::string& blockType, bool mapReference ) {
    JsonArrayBuilder updatedBlocks;
    JsonArray blocks;
    json_get_arr( m_clonedScene, blockType, blocks );

    std::string oldId;
    for( auto& block : blocks ) {
        oldId = json_get_str( block, ID );
        if( !oldId.empty() ) {
            block[ ID ] = Db::genOid();
            if( mapReference ) {
                mapReplacedReference( oldId, block[ ID ] );
            }
        }

        updatedBlocks << block;
    }

    m_clonedScene[ blockType ] = updatedBlocks;
}

bool ScenesCloneManager::run( const std::string& name ) {
    JsonObject answer = JSON_EMPTY_OBJ;

    auto originalSceneName = json_get_str( m_clonedScene, "name" );
    auto originalSceneId = json_get_str( m_clonedScene, ID );
    if( !name.empty() ) {
        m_clonedScene[ "name" ] = name;
    }
    else {
        auto clonedName = createClonedName( originalSceneName );
        m_clonedScene[ "name" ] = clonedName;
    }

    m_oldSceneId = originalSceneId;
    auto id = Db::genOid();
    for( std::size_t i = 0; m_manager.isSceneExist( id ); ++i ) {
        if( i >= 3 ) {
            setErrorWithField( answer, Errors::IMPOSSIBLE_TO_GENERATE_UNIQUE_ID, "", id );
            LOGGER_ERROR( "The unique id can't be generated" );
            return false;
        }
        id = Db::genOid();
    }

    m_newSceneId = id;

    JsonObject sceneID = Db::getIdObj( id );
    m_clonedScene[ db::ID_FIELD_NAME ] = id;

    replaceAllReferencesByPlaceholders();

    return true;
}

void ScenesCloneManager::mapReplacedReference( const std::string& oldReference, const std::string& newReference ) {
    m_innerBlockReferences[ oldReference ] = newReference;
}

void ScenesCloneManager::handleInnerSceneIdRelationsInActionBlocks( const std::string& blockType ) {
    using namespace ActionFields::Fields;
    JsonArray blocks;
    JsonArrayBuilder thenBlocksWithCloned;
    json_get_arr( m_clonedScene, blockType, blocks );

    if( blocks.empty() )
        return;

    for( auto& block : blocks ) {
        std::string methodName = getMethodName( block );
        if( methodName == "resetSceneLatches" | methodName == "resetLatch" ) {
            JsonArray fields;
            JsonArrayBuilder clonedFields;
            if( !json_get( block, "fields", fields ) )
                break;

            std::string sceneId;
            for( auto field : fields ) {
                findAndReplaceSceneIdReference( clonedFields, field );
                findAndReplaceBlockIdReference( clonedFields, field );
                block[ "fields" ] = clonedFields;
            }
        }
        thenBlocksWithCloned << block;
    }
    m_clonedScene[ blockType ] = thenBlocksWithCloned;
}

void ScenesCloneManager::findAndReplaceBlockIdReference( JsonArrayBuilder& clonedFiends, Json& field ) {
    if( json_get_str( field, "name" ) == "blockId" ) {
        auto blockId = json_get_str( field, "value" );
        auto newBlockId = m_innerBlockReferences[ blockId ];
        if( !newBlockId.empty() ) {
            field[ "value" ] = newBlockId;
        }
        clonedFiends << field;
    }
}

void ScenesCloneManager::findAndReplaceSceneIdReference( JsonArrayBuilder& clonedFiends, Json& field ) {
    if( json_get_str( field, "name" ) == "sceneId" ) {
        auto sceneId = json_get_str( field, "value" );
        if( sceneId == m_oldSceneId ) {
            field[ "value" ] = m_newSceneId;
        }
        clonedFiends << field;
    }
}

std::string ScenesCloneManager::getMethodName( const JsonObject& block ) {
    JsonObject blockOptions;
    if( !json_get( block, "blockOptions", blockOptions ) )
        return "";
    std::string methodName;
    JsonObject method;
    if( !json_get( blockOptions, "method", method ) )
        return "";
    methodName = json_get_str( method, "name" );
    return methodName;
}

}  // namespace ScenesManager
}  // namespace HubPlatform
