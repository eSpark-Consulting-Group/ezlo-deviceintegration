#include "scene_scale_modifier.h"
#include "resources.h"

#include "log_utils/ezlo_log.h"
#include "json_utils/json_utils.h"
#include "json_utils/json_path.h"
#include <utility>

namespace SceneEngine {

SceneScaleModifier::SceneScaleModifier() {
    init();
}

SceneScaleModifier::SceneScaleModifier( Json scene, std::string defaultScale )
: m_scene( std::move( scene ) )
, m_defaultScale( std::move( defaultScale ) ) {
    init();
}

void SceneScaleModifier::init() {
    onCheckItemCondition(
        []( const Json& itemArrayValue ) -> bool { return isNumericItem( itemArrayValue[ Fields::TypeField ] ); } );
}

void SceneScaleModifier::onItemFound( ItemFoundHandler handler ) {
    m_itemFoundHandler = std::move( handler );
}

const Json& SceneScaleModifier::getScene() const {
    return m_scene;
}

bool SceneScaleModifier::process() {
    m_modificationCount = 0;

    JsonPath path;
    processScene( m_scene, path );
    return true;
}

/** Converts [{name, value, ...}] to {name: {value: ..., ...}} */
Json SceneScaleModifier::arrayToObject( const Json& array, const std::string& key ) {
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

Json SceneScaleModifier::processScene( Json& scene, JsonPath& path ) {
    if( scene.count( Fields::IdField ) ) {
        LOGGER_TRACE( cc::trace( "# Process scene: " ) + json_get_str( scene, Fields::IdField ) +
                      cc::trace( ", path " ) + path.toString() );
    }

    if( scene.is_object() ) {
        Json found;

        auto command = fetchBlockMethodName( scene );
        // process block
        if( !command.empty() ) {
            LOGGER_DEBUG_RAW( "blockOptions / method / name item found: " + command );

            // if compound block
            bool diveInFields = isLogicOperatorBlock( command );
            if( diveInFields && scene.count( Fields::FieldsField ) ) {
                Json& fields = scene[ Fields::FieldsField ];

                auto g = path.makeGuard( Fields::FieldsField );
                // iterate compound block fields
                iterateCompoundBlockFields( scene, path, fields );

                // iterate fields array of a block
                iterateBlockFieldsArray( fields, path, command );
            }
            else if( isConditionBlock( command ) ) {
                // final block found, process final block
                processConditionBlock( command, scene, path );
            }
        }
        scene = iterateSceneKeys( scene, path );
    }
    else {
        LOGGER_ERROR( "Json passed is not an object" );
    }
    return scene;
}

void SceneScaleModifier::iterateCompoundBlockFields( Json& scene, JsonPath& path, Json& fields ) {
    for( size_t idx = 0; fields.is_array() && idx < fields.size(); idx++ ) {
        Json& record = fields[ idx ];
        auto g = path.makeGuard( idx );

        if( record.is_object() && record.count( Fields::TypeField ) && record[ Fields::TypeField ] == "blocks" &&
            record.count( Fields::ValueField ) && record[ Fields::ValueField ].is_array() ) {
            // iterate blocks in fields / value
            Json& fieldsValueArr = record[ Fields::ValueField ];
            auto gv = path.makeGuard( Fields::ValueField );
            iterateBlocksInFields( scene, path, fieldsValueArr );
        }
    }
}

void SceneScaleModifier::iterateBlocksInFields( Json& scene, JsonPath& path, Json& fieldsValueArr ) {
    for( auto valueIdx = 0; valueIdx != fieldsValueArr.size(); valueIdx++ ) {
        auto g = path.makeGuard( valueIdx );
        Json& block = fieldsValueArr[ valueIdx ];

        Json result = processScene( block, path );

        // Update source
        try {
            incModificationCount();
        }
        catch( const std::exception& err ) {
            LOGGER_ERROR( std::string{ "Error in scene[ \"fields\" ][ \"value\" ][ valueIdx ]: " } + err.what() );
            throw err;
        }
    }
}

JsonPath& SceneScaleModifier::iterateBlockFieldsArray( Json& fields, JsonPath& path, const std::string& command ) {
    for( auto fieldIt = fields.begin(); fieldIt != fields.end(); fieldIt++ ) {
        auto dist = std::distance( fields.begin(), fieldIt );
        auto g = path.makeGuard( dist );
        processScene( fieldIt.value(), path );
    }
    return path;
}

std::string SceneScaleModifier::fetchBlockMethodName( const Json& scene ) {
    using namespace Fields;
    Json found;
    if( json_get( scene, std::vector< Json >{ BlockOptions, Method, NameField }, found ) ) {
        return found.get< std::string >();
    }
    return std::string{};
}

Json& SceneScaleModifier::iterateSceneKeys( Json& scene, JsonPath& path ) {
    for( auto keyIt = scene.begin(); keyIt != scene.end(); keyIt++ ) {
        std::string key = keyIt.key();
        Json value = keyIt.value();

        if( isBlock( key ) && value.is_array() && !value.empty() ) {
            auto g = path.makeGuard( key );

            Json& blockArray = value;
            for( auto blockIt = blockArray.begin(); blockIt != blockArray.end(); blockIt++ ) {
                auto dist = std::distance( blockArray.begin(), blockIt );
                auto gd = path.makeGuard( dist );

                processScene( blockIt.value(), path );
            }
        }
    }
    return scene;
}

bool SceneScaleModifier::isBlock( const std::string& key ) {
    return ( key == "when" || key == "then" );
}

void SceneScaleModifier::processConditionBlock( const std::string& command,
                                                Json& scene,
                                                JsonPath& path ) {  // process "fields" of a command
    if( scene.count( Fields::FieldsField ) ) {
        auto g = path.makeGuard( Fields::FieldsField );
        processItem( scene, path, command, scene[ Fields::FieldsField ] );
    }
}

bool SceneScaleModifier::isLogicOperatorBlock( const std::string& command ) {
    using namespace MethodNames;
    return command == OrOperator || command == AndOperator || command == NotOperator || command == XorOperator ||
           command == FunctionOperator;
}

bool SceneScaleModifier::isConditionBlock( const std::string& command ) {
    using namespace MethodNames;
    return command == CompareNumbers || command == SetItemValue || command == IsItemState ||
           command == CompareNumberRange;
}

Json SceneScaleModifier::processItem( const Json& scene, JsonPath& path, const std::string& command, Json itemFields ) {
    LOGGER_DEBUG( cc::j( itemFields ) );
    if( !itemFields.is_array() )
        return nullptr;

    Json itemObject = arrayToObject( itemFields, Fields::NameField );
    auto itemSectionIt = itemObject.find( Fields::ItemField );
    if( itemSectionIt == itemObject.cend() )
        return nullptr;  // not an item

    std::string itemId = ( *itemSectionIt )[ Fields::ValueField ];
    if( itemObject.count( Fields::ValueField ) ) {
        processItemByValueName( path, itemObject, itemId, Fields::ValueField );
    }
    else if( itemObject.count( Fields::StartValueField ) && itemObject.count( Fields::EndValueField ) ) {
        LOGGER_DEBUG( cc::debug( "process method without 'value' field, but with 'startValue', 'endValue' fields" ) );
        processItemByValueName( path, itemObject, itemId, Fields::StartValueField );
        processItemByValueName( path, itemObject, itemId, Fields::EndValueField );
    }

    return itemFields;
}

void SceneScaleModifier::processItemByValueName( const JsonPath& path,
                                                 const Json& itemObject,
                                                 const std::string& itemId,
                                                 const std::string& valueFieldName ) {
    Json valueObject = itemObject[ valueFieldName ];
    if( valueObject.count( Fields::ArrayIndex ) ) {
        json_remove( valueObject, Fields::ArrayIndex );
    }
    std::string itemValueType = valueObject[ Fields::TypeField ];

    if( emitItemCheckCondition( valueObject ) ) {
        ItemSearchKey searchKey( "", itemId );
        ItemFound itemFound{ searchKey };
        const Json& valueBlockIndex = itemObject[ valueFieldName ][ Fields::ArrayIndex ];
        emitItemFound( itemFound, path, valueObject, valueBlockIndex );
    }
}

bool SceneScaleModifier::isNumericItem( const std::string& itemValueType ) {
    return itemValueType == Types::IntType || itemValueType == Types::FloatType;
}

void SceneScaleModifier::incModificationCount() {
    m_modificationCount++;
}

unsigned SceneScaleModifier::modificationCount() const {
    return m_modificationCount;
}

void SceneScaleModifier::emitItemFound( const ItemFound& itemFound,
                                        const JsonPath& path,
                                        const Json& valueBlock,
                                        const Json& valueBlockIndex ) {
    LOGGER_TRACE( "# Path " + path.toString() +
                  cc::trace( ", itemId: " )
                      .append( itemFound.key.getSourceId() )
                      .append( cc::trace( ", controllerId: " ).append( itemFound.key.getControllerId() ) ) );
    if( !valueBlock.empty() ) {
        LOGGER_TRACE( "valueBlock: " + cc::j( valueBlock ) + ", index: " + valueBlockIndex.dump() );
    }

    if( m_itemFoundHandler ) {
        ItemFound event( itemFound );
        event.path = path;
        event.valueBlock = valueBlock;
        if( !valueBlockIndex.empty() ) {
            event.valueBlockRelativePath.pushBack( valueBlockIndex );
        }

        m_itemFoundHandler( event );
    }
}

bool SceneScaleModifier::updateItemValue( const JsonPath& path, const Json& valueBlock ) {
    const auto& keys = path.keys();
    Json* current = &m_scene;

    try {
        for( const auto& key : keys ) {
            if( key.is_string() ) {
                Json& found = current->at( key.get< std::string >() );
                current = &found;
            }
            else if( key.is_number() ) {
                Json& found = current->at( key.get< int >() );
                current = &found;
            }
        }

        if( current ) {
            *current = valueBlock;
            return true;
        }
    }
    catch( std::exception& ex ) {
        LOGGER_ERROR( cc::error( "SceneScaleModifier exception: " ) + ex.what() );
    }
    return false;
}

void SceneScaleModifier::onCheckItemCondition( SceneScaleModifier::ItemCheckConditionHandler checkItem ) {
    m_itemCheckCondition = std::move( checkItem );
}

bool SceneScaleModifier::emitItemCheckCondition( const Json& itemArrayValue ) {
    if( m_itemCheckCondition != nullptr )
        return m_itemCheckCondition( itemArrayValue );
    return isNumericItem( itemArrayValue[ Fields::TypeField ] );
}

const std::string& SceneScaleModifier::defaultScale() const {
    return m_defaultScale;
}

void SceneScaleModifier::setScene( Json scene ) {
    m_scene = std::move( scene );
}

void SceneScaleModifier::setDefaultScale( std::string defaultScale ) {
    m_defaultScale = std::move( defaultScale );
}

void SceneScaleModifier::clear() {
    m_scene.clear();
    m_defaultScale.clear();
    this->m_modificationCount = 0;
}

}  // namespace SceneEngine
