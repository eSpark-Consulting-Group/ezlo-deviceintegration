#include "migration_tool/migration/v1_28/scale_settings_migration.h"

#include "system_utils/environment.h"
#include "database_client/idbclient.h"
#include "database_client/dbqueryparser.h"
#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace MigrationTool {
namespace v1_28 {

namespace Fields {
const char* ValueField = "value";
const char* ScaleField = "scale";
const char* TypeField = "type";
const char* IdField = "_id";
const char* FieldsField = "fields";
const char* ItemField = "item";
const char* NameField = "name";
const char* ArrayIndex = "__array_index";

const char* BlockOptions = "blockOptions";
const char* Method = "method";
const char* WhenBlock = "when";
const char* ThenBlock = "then";
}  // namespace Fields

namespace Types {
const char* IntType = "int";
const char* FloatType = "float";
}  // namespace Types

namespace MethodNames {
const char* IsItemState = "isItemState";
const char* CompareNumbers = "compareNumbers";
const char* SetItemValue = "setItemValue";
const char* And = "and";
const char* Not = "not";
const char* Or = "or";
}  // namespace MethodNames

namespace {

/** Converts [{name, value, ...}] to {name: {value: ..., ...}}
 * */
Json arrayToObject( const Json& array, const std::string& key ) {
    if( !array.is_array() || key.empty() )
        return nullptr;
    Json result = Json::object();
    int index = 0;
    for( auto it = array.begin(); it != array.end(); it++, index++ ) {
        const auto& next = it.value();
        if( !next.is_object() )
            continue;
        if( 0 == next.count( key ) )
            continue;

        Json left = Json::object();
        for( auto oit = next.begin(); oit != next.end(); oit++ ) {
            left[ oit.key() ] = oit.value();
        }
        left[ Fields::ArrayIndex ] = index;

        result[ next[ key ].get< std::string >() ] = std::move( left );
    }
    return result;
}

}  // namespace

Migration::Migration( IDbClient& dbClient )
: m_dbClient( dbClient ) {}

namespace {

const char* itemsDb = "db.firmware.uid.items";
const char* scenesDb = "db.firmware.scenes_manager.rules";

}  // namespace

bool Migration::migrate() {
    auto scenes = m_dbClient.findAll( scenesDb, JSON_EMPTY_OBJ, nullptr );

    for( auto scene : scenes ) {
        auto copyScene = scene;
        try {
            auto updatedScene = start( scene );

            if( auto numBlocks = getItemBlocksUpdated() ) {
                LOGGER_INFO( cc::trace( "Scene is to be updated: " ) + json_get_str( updatedScene, Fields::IdField ) +
                             cc::trace( ", changed item values: " ) + cc::num10( numBlocks ) );
                LOGGER_TRACE( cc::j( updatedScene ) );
                m_updatedScenes++;
                saveScene( updatedScene );
            }
            else {
                LOGGER_INFO( "Scene to be left unchanged" );
            }
        }
        catch( const std::exception& err ) {
            LOGGER_ERROR( std::string{ "Exception has caught: " } + err.what() );
            LOGGER_ERROR( "Migration has failed" );
            return false;
        }
    }

    return true;
}

Json Migration::start( Json& scene ) {
    resetItemBlocksUpdated();

    return processScene( scene );
}

std::string Migration::fetchBlockMethodName( const Json& scene ) {
    using namespace Fields;
    Json found;
    if( json_get( scene, std::vector< Json >{ BlockOptions, Method, NameField }, found ) ) {
        return found.get< std::string >();
    }
    return std::string{};
}

Json Migration::processScene( Json& scene, const std::string& keyPath ) {
    if( scene.is_object() ) {
        Json found;

        auto command = fetchBlockMethodName( scene );
        // find root of blockOption
        if( !command.empty() ) {
            LOGGER_DEBUG_RAW( "blockOptions / method / name item found: " + command );

            bool diveInFields = isLogicOperatorBlock( command );
            if( diveInFields && scene.count( Fields::FieldsField ) ) {
                LOGGER_DEBUG_RAW( "Dive in fields for command " + command );
                Json& fields = scene[ Fields::FieldsField ];
                fields = iterateCompoundBlockFields( scene, fields, keyPath );

                iterateBlockFieldsArray( fields, command, keyPath );
            }
            else if( isConditionBlock( command ) ) {
                // process "fields" of a command
                processCommandFields( scene, command );
            }
        }
        scene = iterateSceneKeys( scene, keyPath );
    }
    else {
        LOGGER_ERROR( "error: scene is not an object" );
    }
    return scene;
}

void Migration::iterateBlockFieldsArray( Json& fields, const std::string& command, const std::string& keyPath ) {
    for( auto fieldIt = fields.begin(); fieldIt != fields.end(); fieldIt++ ) {
        auto dist = std::distance( fields.begin(), fieldIt );
        std::string newKeyPath =
            std::string{ keyPath + "/ " }.append( command ).append( " / " + std::to_string( dist ) );
        Json sub = processScene( fieldIt.value(), newKeyPath );

        *fieldIt = sub;
    }
}

Json& Migration::iterateCompoundBlockFields( Json& scene, Json& fields, const std::string& keyPath ) {
    for( size_t idx = 0; fields.is_array() && idx < fields.size(); idx++ ) {
        Json& record = fields[ idx ];

        if( record.is_object() && record.count( Fields::TypeField ) && record[ Fields::TypeField ] == "blocks" &&
            record.count( Fields::ValueField ) && record[ Fields::ValueField ].is_array() ) {
            // iterate blocks in fields / value
            Json& fieldsValueArr = record[ Fields::ValueField ];
            iterateBlocksInFields( scene, keyPath, idx, fieldsValueArr );
        }
    }
    return fields;
}

void Migration::iterateBlocksInFields( Json& scene, const std::string& keyPath, size_t idx, Json& fieldsValueArr ) {
    for( auto valueIdx = 0; valueIdx != fieldsValueArr.size(); valueIdx++ ) {
        Json& block = fieldsValueArr[ valueIdx ];

        std::string newKeyPath = keyPath + " / fields / value / " + std::to_string( valueIdx );
        Json result = processScene( block, newKeyPath );

        // Update source
        try {
            scene[ Fields::FieldsField ][ idx ][ Fields::ValueField ][ valueIdx ] = result;
        }
        catch( const std::exception& err ) {
            LOGGER_ERROR( std::string{ "Error in scene[ \"fields\" ][ \"value\" ][ valueIdx ]: " } + err.what() );
            throw err;
        }
    }
}

Json& Migration::iterateSceneKeys( Json& scene, const std::string& keyPath ) {
    for( auto keyIt = scene.begin(); keyIt != scene.end(); keyIt++ ) {
        std::string key = keyIt.key();
        Json& value = keyIt.value();

        if( isBlock( key ) && value.is_array() && !value.empty() ) {
            LOGGER_DEBUG_RAW( "Key: " + key );
            for( auto blockIt = value.begin(); blockIt != value.end(); blockIt++ ) {
                auto dist = std::distance( value.begin(), blockIt );
                std::string newKeyPath =
                    std::string( keyPath + "/ " ).append( key ).append( " / " ).append( std::to_string( dist ) );
                Json sub = processScene( blockIt.value(), newKeyPath );
                Log::print( Log::DEBUG, "Block '%s'", key.c_str() );

                scene[ key ][ dist ] = sub;
            }
        }
    }
    return scene;
}

bool Migration::isBlock( const std::string& key ) {
    return ( key == Fields::WhenBlock || key == Fields::ThenBlock );
}

void Migration::processCommandFields( Json& scene, const std::string& command ) {
    if( scene.count( Fields::FieldsField ) ) {
        Json itemWithScale = processItem( scene, command, scene[ Fields::FieldsField ] );

        scene[ Fields::FieldsField ] = itemWithScale;
    }
}

bool Migration::isLogicOperatorBlock( const std::string& command ) {
    using namespace MethodNames;
    return command == Or || command == And || command == Not;
}

bool Migration::isConditionBlock( const std::string& command ) {
    using namespace MethodNames;
    return command == CompareNumbers || command == SetItemValue || command == IsItemState;
}

Json Migration::processItem( const Json& scene, const std::string& command, Json itemFields ) {
    if( !itemFields.is_array() )
        return nullptr;

    Json itemObject = arrayToObject( itemFields, Fields::NameField );

    std::string itemId = itemObject[ Fields::ItemField ][ Fields::ValueField ];
    Json& itemArrayValue = itemObject[ Fields::ValueField ];
    std::string itemValueType = itemArrayValue[ Fields::TypeField ];
    Json itemValue = itemArrayValue[ Fields::ValueField ];

    if( itemValueType == Types::IntType || itemValueType == Types::FloatType ) {
        // possibly it's a temperature
        auto coreItem = findCoreItem( itemId );
        auto valueType = json_get_str( coreItem, Fields::TypeField );
        if( valueType == "temperature" ) {
            LOGGER_DEBUG_RAW( "Core item found: " + json_get_str( coreItem, Fields::IdField ) );

            if( itemArrayValue.count( Fields::ScaleField ) == 0 ) {
                int toModify = itemObject[ Fields::ValueField ][ Fields::ArrayIndex ];

                incItemBlocksUpdated();
                itemFields[ toModify ][ Fields::ScaleField ] = m_defaultScale;
            }
        }
    }

    return itemFields;
}

Json Migration::findCoreItem( const std::string& itemId ) {
    return m_dbClient.findOne( itemsDb, Json{ { Fields::IdField, itemId } }, nullptr );
}

unsigned Migration::getScenesUpdatedCount() const {
    return m_updatedScenes;
}

void Migration::saveScene( const Json& updatedScene ) {
    m_dbClient.update( scenesDb,
                       JSON( db::ID_FIELD_NAME << updatedScene[ db::ID_FIELD_NAME ] ),
                       JSON( "$set" << updatedScene ),
                       false,
                       false );
}

void Migration::resetItemBlocksUpdated() {
    m_itemBlocksUpdated = 0;
}

void Migration::incItemBlocksUpdated() {
    m_itemBlocksUpdated++;
}

int Migration::getItemBlocksUpdated() const {
    return m_itemBlocksUpdated;
}

}  // namespace v1_28
}  // namespace MigrationTool
}  // namespace HubPlatform
