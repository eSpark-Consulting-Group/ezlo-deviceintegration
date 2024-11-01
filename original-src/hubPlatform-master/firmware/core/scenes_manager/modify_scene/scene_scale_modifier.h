#pragma once
#include "scenes_manager/scenes_engine/value_source_key.h"
#include "json_utils/json_path.h"
#include <string>
#include <functional>

namespace SceneEngine {

class SceneScaleModifier {
public:
    using ItemSearchKey = SceneEngine::ValueSourceKey;
    struct ItemFound {
        ItemSearchKey key;
        JsonPath path;
        Json valueBlock;
        JsonPath valueBlockRelativePath;
    };

    using ItemFoundHandler = std::function< void( const ItemFound& item ) >;
    using ItemCheckConditionHandler = std::function< bool( const Json& itemArrayValue ) >;

public:
    SceneScaleModifier();
    SceneScaleModifier( Json scene, std::string defaultScale );

    void setScene( Json scene );
    void setDefaultScale( std::string defaultScale );
    void clear();

    void onItemFound( ItemFoundHandler handler );
    void onCheckItemCondition( ItemCheckConditionHandler checkItem );

    bool updateItemValue( const JsonPath& path, const Json& valueBlock );
    const Json& getScene() const;

    bool process();
    unsigned modificationCount() const;

    const std::string& defaultScale() const;
    static bool isNumericItem( const std::string& itemValueType );

private:
    void init();
    Json processScene( Json& scene, JsonPath& path );
    Json processItem( const Json& scene, JsonPath& path, const std::string& command, Json itemFields );

    void incModificationCount();

private:
    static bool isConditionBlock( const std::string& command );
    static bool isLogicOperatorBlock( const std::string& command );
    void processConditionBlock( const std::string& command, Json& scene, JsonPath& path );
    static bool isBlock( const std::string& key );
    static Json arrayToObject( const Json& array, const std::string& key );

private:
    Json& iterateSceneKeys( Json& scene, JsonPath& path );
    static std::string fetchBlockMethodName( const Json& scene );
    JsonPath& iterateBlockFieldsArray( Json& fields, JsonPath& path, const std::string& command );
    void iterateBlocksInFields( Json& scene, JsonPath& path, Json& fieldsValueArr );
    void iterateCompoundBlockFields( Json& scene, JsonPath& path, Json& fields );
    void emitItemFound( const ItemFound& itemFound,
                        const JsonPath& path,
                        const Json& valueBlock,
                        const Json& valueBlockIndex );
    bool emitItemCheckCondition( const Json& itemArrayValue );
    void processItemByValueName( const JsonPath& path,
                                 const Json& itemObject,
                                 const std::string& itemId,
                                 const std::string& valueFieldName );

private:
    Json m_scene;
    ItemFoundHandler m_itemFoundHandler{ nullptr };
    ItemCheckConditionHandler m_itemCheckCondition{ nullptr };
    unsigned m_modificationCount{ 0 };
    std::string m_defaultScale{ "celsius" };
};

}  // namespace SceneEngine