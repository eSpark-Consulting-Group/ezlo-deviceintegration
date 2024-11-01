#include "json_helper.h"
#include "scenes_manager/scenes_engine/exceptions.h"
#include "json_utils/json_utils.h"

bool json_helper::get_field_from_array( const std::string& field_name,
                                        const JsonObject& object,
                                        JsonObject& result,
                                        const char* key ) {
    JsonArray fields;

    if( !key || !*key )
        key = "value";

    if( !json_get_arr( object, "fields", fields ) )
        return false;

    for( const auto& field : fields ) {
        std::string name;

        if( !json_get( field, "name", name ) )
            return false;

        if( name == field_name )
            return json_get( field, key, result ) && !result.empty();
    }

    return false;
}

bool json_helper::get_int_from_fields_array( const std::string& field_name, const JsonObject& object, int& result ) {
    JsonObject value;
    if( get_field_from_array( field_name, object, value ) && value.is_number() && json_get( value, result ) ) {
        return true;
    }

    return false;
}

bool json_helper::get_field_from_array( const std::string& field_name, const JsonObject& object, std::string& result ) {
    JsonObject value;

    if( get_field_from_array( field_name, object, value ) && value.is_string() ) {
        result = json_get_str( value );
        return true;
    }

    return false;
}

bool json_helper::get_scene_field_value( const std::string& name, const JsonObject& object, JsonObject& result ) {
    JsonObject block_options;

    if( !json_get_obj( object, "blockOptions", block_options ) )
        return false;

    JsonObject trigger_object;
    if( !json_get_obj( block_options, "trigger_object", trigger_object ) )
        return false;

    std::string field_value;

    if( json_get( trigger_object, name, field_value ) ) {
        get_field_from_array( field_value, object, result );
        return true;
    }
    return false;
}

bool json_helper::get_scene_field_value( const std::string& name, const JsonObject& object, std::string& result ) {
    JsonObject toRet;
    if( get_scene_field_value( name, object, toRet ) && toRet.is_string() ) {
        result = json_get_str( toRet );
        return true;
    }
    return false;
}

namespace SceneEngine {
namespace Utils {

void iterateArray( const JsonArray& arr, EntryFuncProcConst func ) {
    if( !arr.is_array() ) {
        return;
    }

    for( const auto& elem : arr ) {
        func( elem );
    }
}

void entryProcessor( const std::string& name, const JsonObject& obj, EntryFuncProcConst func ) {
    JsonObject::const_iterator it = obj.find( name );
    if( it != obj.cend() ) {
        func( *it );
    }
}

void entryProcessor( const std::string& name,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ConditionFuncConst condFunc ) {
    JsonObject::const_iterator it = obj.find( name );
    if( it != obj.cend() && condFunc( *it ) ) {
        func( *it );
    }
}

void entryProcessor( const std::string& name,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ErrorFuncConst errorFunc ) {
    JsonObject::const_iterator it = obj.find( name );
    if( it != obj.cend() ) {
        func( *it );
    }
    else {
        errorFunc( obj, name );
    }
}

void entryProcessor( const std::string& name, JsonObject& obj, EntryFunProc func, ConditionFuncConst condFunc ) {
    JsonObject::iterator it = obj.find( name );
    if( it != obj.end() ) {
        func( *it );
    }
}

void entryProcessor( const std::string& name,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ConditionFuncConst condFunc,
                     ErrorFuncConst errorFunc ) {
    JsonObject::const_iterator it = obj.find( name );
    if( it != obj.cend() && condFunc( *it ) ) {
        func( *it );
    }
    else {
        errorFunc( obj, name );
    }
}

void entryProcessor( const JsonPathLeafType& path, JsonObject& obj, EntryFunProc func ) {
    JsonObject& currentObj = obj;

    if( path.path.empty() ) {
        return;
    }

    if( path.path.size() == 1 ) {
        const auto entryName = path.path.front();
        JsonObject::iterator jsonIt = currentObj.find( entryName );
        if( jsonIt != currentObj.end() && path.finalTypeCond( *jsonIt ) ) {
            func( *jsonIt );
        }
        return;
    }

    JsonObject::iterator jsonIt = currentObj.find( path.path.front() );
    if( jsonIt == currentObj.end() || !( *jsonIt ).is_object() ) {
        return;
    }
    else {
        JsonPathLeafType::PathType newPath;
        JsonPathLeafType::PathType::const_iterator next = std::next( path.path.cbegin() );
        std::copy( next, path.path.cend(), std::back_inserter( newPath ) );
        JsonPathLeafType p{ newPath, path.finalTypeCond };
        entryProcessor( p, *jsonIt, func );
    }
}

void entryProcessor( const JsonPathLeafType& path, JsonObject& obj, EntryFunProc func, ErrorFuncConst errorFunc ) {
    JsonObject& currentObj = obj;

    if( path.path.empty() ) {
        return;
    }

    if( path.path.size() == 1 ) {
        const auto entryName = path.path.front();
        JsonObject::iterator jsonIt = currentObj.find( entryName );
        if( jsonIt != currentObj.end() && path.finalTypeCond( *jsonIt ) ) {
            func( *jsonIt );
        }
        else {
            errorFunc( currentObj, entryName );
        }
        return;
    }

    const auto entryName = path.path.front();
    JsonObject::iterator jsonIt = currentObj.find( entryName );
    if( jsonIt == currentObj.end() || !( *jsonIt ).is_object() ) {
        errorFunc( currentObj, entryName );
        return;
    }
    else {
        JsonPathLeafType::PathType newPath;
        JsonPathLeafType::PathType::const_iterator next = std::next( path.path.cbegin() );
        std::copy( next, path.path.cend(), std::back_inserter( newPath ) );
        JsonPathLeafType p{ newPath, path.finalTypeCond };
        entryProcessor( p, *jsonIt, func, errorFunc );
    }
}

void entryProcessor( const JsonPathLeafType& jsonFinder, const JsonObject& obj, EntryFuncProcConst func ) {
    if( jsonFinder.path.empty() ) {
        return;
    }

    JsonObject currentObject = obj;
    JsonPathLeafType::PathType::const_iterator it = jsonFinder.path.cbegin();
    JsonPathLeafType::PathType::const_iterator e = std::prev( jsonFinder.path.cend() );
    for( it; it != e; ++it ) {
        JsonObject::const_iterator jsonIt = currentObject.find( *it );
        if( jsonIt == currentObject.cend() || !( *jsonIt ).is_object() ) {
            return;
        }
        else {
            currentObject = ( *jsonIt );
        }
    }
    const auto finalNode = *it;
    JsonObject::const_iterator jsonIt = currentObject.find( finalNode );
    if( jsonIt != currentObject.cend() && jsonFinder.finalTypeCond( *jsonIt ) ) {
        func( *jsonIt );
    }
}

void entryProcessor( const JsonPathLeafType& jsonFinder,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ErrorFuncConst errorFunc ) {
    if( jsonFinder.path.empty() ) {
        return;
    }

    JsonObject currentObject = obj;
    JsonPathLeafType::PathType::const_iterator it = jsonFinder.path.cbegin();
    JsonPathLeafType::PathType::const_iterator e = std::prev( jsonFinder.path.cend() );
    for( ; it != e; ++it ) {
        JsonObject::const_iterator jsonIt = currentObject.find( *it );
        if( jsonIt == currentObject.cend() || !( *jsonIt ).is_object() ) {
            errorFunc( *it, currentObject );
            return;
        }
        else {
            currentObject = ( *jsonIt );
        }
    }
    const auto finalNode = *it;
    JsonObject::const_iterator jsonIt = currentObject.find( finalNode );
    if( jsonIt != currentObject.cend() && jsonFinder.finalTypeCond( *jsonIt ) ) {
        func( *jsonIt );
    }
    else {
        errorFunc( finalNode, currentObject );
    }
}

namespace JsonTypes {
bool isArray( const JsonObject& obj ) {
    return obj.is_array();
}

bool isString( const JsonObject& obj ) {
    return obj.is_string();
}

bool isAnyOfFields( const JsonObject& itemFilterList, const std::string& itemTypeName ) {
    if( itemFilterList.is_array() ) {
        bool ret = std::any_of(
            itemFilterList.begin(), itemFilterList.end(), [&itemTypeName]( const JsonObject& obj ) -> bool {
                if( JsonTypes::isString( obj ) ) {
                    const auto name = obj.get< std::string >();
                    return name == itemTypeName;
                }
                return false;
            } );
        return ret;
    }
    return false;
}

}  // namespace JsonTypes

bool findField( const std::string& name, JsonArray& fields, FuncProcessor func ) {
    if( !fields.is_array() ) {
        throw SceneException( Errors::WRONG_FIELD );
    }

    for( auto& field : fields ) {
        JsonObject::iterator fieldNameIt = field.find( "name" );
        if( fieldNameIt != field.end() && fieldNameIt->is_string() ) {
            const auto fieldName = fieldNameIt->get< std::string >();
            if( fieldName == name ) {
                func( field );
                return true;
            }
        }
    }
    return false;
}

bool findField( const std::string& name, const JsonArray& fields, FuncProcessorConst func ) {
    if( !fields.is_array() ) {
        throw SceneException( Errors::WRONG_FIELD );
    }
    for( const auto& field : fields ) {
        JsonObject::const_iterator fieldNameIt = field.find( "name" );
        if( fieldNameIt != field.cend() && fieldNameIt->is_string() ) {
            const auto fieldName = fieldNameIt->get< std::string >();
            if( fieldName == name ) {
                func( field );
                return true;
            }
        }
    }
    return false;
}

JsonArray::iterator findField( const std::string& name, JsonArray& fields ) {
    if( !fields.is_array() ) {
        return fields.end();
    }
    for( JsonArray::iterator field = fields.begin(); field != fields.end(); ++field ) {
        JsonObject::const_iterator fieldNameIt = field->find( "name" );
        if( fieldNameIt != field->cend() && fieldNameIt->is_string() ) {
            const auto fieldName = fieldNameIt->get< std::string >();
            if( fieldName == name ) {
                return field;
            }
        }
    }
    return fields.end();
}

JsonArray::const_iterator findField( const std::string& name, const JsonArray& fields ) {
    if( !fields.is_array() ) {
        return fields.end();
    }
    for( JsonArray::const_iterator field = fields.begin(); field != fields.end(); ++field ) {
        JsonObject::const_iterator fieldNameIt = field->find( "name" );
        if( fieldNameIt != field->cend() && fieldNameIt->is_string() ) {
            const auto fieldName = fieldNameIt->get< std::string >();
            if( fieldName == name ) {
                return field;
            }
        }
    }
    return fields.end();
}

void forEachField(
    const JsonArray& fields,
    std::function<
        void( const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) >
        callback ) {
    if( !fields.is_array() ) {
        throw SceneException( Errors::WRONG_PARAMS );
    }
    for( const auto& field : fields ) {
        JsonObject::const_iterator it = field.find( "name" );
        if( it == field.cend() || !it->is_string() ) {
            throw SceneException( Errors::WRONG_PARAMS );
        }
        auto name = it->get< std::string >();

        it = field.find( "type" );
        if( it == field.cend() || !it->is_string() ) {
            throw SceneException( Errors::WRONG_PARAMS );
        }
        auto type = it->get< std::string >();

        std::string scale;
        it = field.find( "scale" );
        if( it != field.cend() ) {
            scale = it->get< std::string >();
        }

        it = field.find( "value" );
        if( it == field.cend() ) {
            throw SceneException( Errors::WRONG_PARAMS );
        }

        callback( name, type, *it, scale );
    }
}

}  // namespace Utils
}  // namespace SceneEngine
