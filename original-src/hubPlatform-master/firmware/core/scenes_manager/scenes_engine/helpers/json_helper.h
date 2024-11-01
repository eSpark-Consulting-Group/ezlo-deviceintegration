#pragma once

#include "json_utils/json_wrapper.h"
#include <string>
#include <list>
#include <set>

struct json_helper {
    static bool get_field_from_array( const std::string& field_name, const JsonObject& object, std::string& result );
    static bool get_field_from_array( const std::string& field_name,
                                      const JsonObject& object,
                                      JsonObject& result,
                                      const char* key = "value" );
    static bool get_int_from_fields_array( const std::string& field_name, const JsonObject& object, int& result );
    static bool get_scene_field_value( const std::string& name, const JsonObject& object, JsonObject& result );
    static bool get_scene_field_value( const std::string& name, const JsonObject& object, std::string& result );
};

namespace SceneEngine {
namespace Utils {

using EntryFuncProcConst = std::function< void( const JsonObject& elem ) >;
using ConditionFuncConst = std::function< bool( const JsonObject& arg ) >;
using ErrorFuncConst = std::function< void( const JsonObject& arg, const std::string& errorFieldName ) >;
using EntryFunProc = std::function< void( JsonObject& elem ) >;

struct JsonPathLeafType {
    using PathType = std::list< std::string >;
    PathType path;
    ConditionFuncConst finalTypeCond;
};

void iterateArray( const JsonArray& arr, EntryFuncProcConst func );
void entryProcessor( const std::string& name, const JsonObject& obj, EntryFuncProcConst func );
void entryProcessor( const std::string& name,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ConditionFuncConst condFunc );
void entryProcessor( const std::string& name,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ErrorFuncConst errorFunc );

void entryProcessor( const std::string& name,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ConditionFuncConst condFunc,
                     ErrorFuncConst errorFunc );
/*
 * brief : allow to recursive search in json by path
 * Final type of the element should be known
 */

void entryProcessor( const JsonPathLeafType& path, JsonObject& obj, EntryFunProc func );
void entryProcessor( const JsonPathLeafType& path, JsonObject& obj, EntryFunProc func, ErrorFuncConst errorFunc );
void entryProcessor( const JsonPathLeafType& path, const JsonObject& obj, EntryFuncProcConst func );
void entryProcessor( const JsonPathLeafType& path,
                     const JsonObject& obj,
                     EntryFuncProcConst func,
                     ErrorFuncConst errorFunc );
void entryProcessor( const std::string& name, JsonObject& obj, EntryFunProc func, ConditionFuncConst condFunc );

namespace JsonTypes {
bool isArray( const JsonObject& obj );
bool isString( const JsonObject& obj );

// TODO: make template with instances for argument and is_string()/is_object()/is_number() etc
bool isAnyOfFields( const JsonObject& itemFilterList, const std::string& itemTypeName );

}  // namespace JsonTypes

using FuncProcessor = std::function< void( JsonObject& field ) >;
using FuncProcessorConst = std::function< void( const JsonObject& field ) >;
bool findField( const std::string& fieldName, JsonArray& fields, FuncProcessor func );
bool findField( const std::string& fieldName, const JsonArray& fields, FuncProcessorConst func );

JsonArray::iterator findField( const std::string& name, JsonArray& fields );
JsonArray::const_iterator findField( const std::string& name, const JsonArray& fields );

void forEachField(
    const JsonArray& fields,
    std::function<
        void( const std::string& name, const std::string& type, const JsonObject& value, const std::string& scale ) >
        callback );

}  // namespace Utils
}  // namespace SceneEngine
