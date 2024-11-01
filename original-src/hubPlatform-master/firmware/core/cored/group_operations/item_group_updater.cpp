#include "item_group.h"
#include "item_group_updater.h"
#include "group_operations_utils.h"
#include "resources.h"
#include "scenes_manager/scenes_engine/helpers/json_helper.h"
#include "json_utils/json_path.h"
#include "jsonrpc/jsonrpc.h"
#include <algorithm>

namespace HubPlatform {
namespace CoreD {
using namespace Resources;

JsonObject ItemGroupUpdater::getItemGroupUpdateData( const JsonObject& dbItem, const Json& params ) {
    using namespace Resources;
    auto savedName = json_get_str( dbItem, ItemGroup::Name );
    auto updateName = json_get_str( params, ItemGroup::Name );

    auto savedPersistent = json_get_bool( dbItem, ItemGroup::Persistent );
    auto updatePersistent = json_get_bool( params, ItemGroup::Persistent );

    bool hasUpdatedPersistent = json_get( params, ItemGroup::Persistent, updatePersistent );
    Json updatedJson = params;

    if( !hasUpdatedPersistent ) {
        updatedJson[ ItemGroup::Persistent ] = savedPersistent;
    }

    return updatedJson;
}

JsonObject ItemGroupUpdater::getItemGroupUpdateBroadcastData( const JsonObject& updatedData,
                                                              const JsonObject& savedData ) {
    using namespace Resources;
    ItemGroupInfo fromDb, fromUser;

    fromDb.fromJson( savedData );
    fromUser.fromJson( updatedData );

    JsonArray updatedItemNames;
    bool isItemNamesUpdated =
        !GroupOperationsUtils::areStringArraysEqual( fromDb.getItemNames(), fromUser.getItemNames() );
    bool isEnumUpdated = !GroupOperationsUtils::areStringArraysEqual( fromDb.getEnum(), fromUser.getEnum() );
    Json updatedJson = JSON_EMPTY_OBJ;

    auto isExist = [&updatedData]( const std::string name ) {
        JsonObject groupMemeber;
        return json_get( updatedData, name, groupMemeber );
    };

    if( isExist( ItemGroup::ItemNames ) && isItemNamesUpdated ) {
        updatedJson[ ItemGroup::ItemNames ] = fromUser.getItemNames();
    }

    if( isExist( ItemGroup::Enum ) && isEnumUpdated ) {
        updatedJson[ ItemGroup::Enum ] = fromUser.getEnum();
    }

    if( isExist( ItemGroup::HasGetter ) && fromDb.getHasGetter() != fromUser.getHasGetter() ) {
        updatedJson[ ItemGroup::HasGetter ] = fromUser.getHasGetter();
    }

    if( isExist( ItemGroup::HasSetter ) && fromDb.getHasSetter() != fromUser.getHasSetter() ) {
        updatedJson[ ItemGroup::HasSetter ] = fromUser.getHasSetter();
    }

    if( isExist( ItemGroup::ValueType ) && fromDb.getValueType() != fromUser.getValueType() ) {
        updatedJson[ ItemGroup::ValueType ] = fromUser.getValueType().toString();
    }

    if( isExist( ItemGroup::ValueTypeFamily ) && fromDb.getValueTypeFamily() != fromUser.getValueTypeFamily() ) {
        updatedJson[ ItemGroup::ValueTypeFamily ] = fromUser.getValueTypeFamily();
    }

    if( fromDb.getName() != fromUser.getName() ) {
        updatedJson[ ItemGroup::Name ] = fromUser.getName();
        updatedJson[ ItemGroup::Sync ] = true;
    }
    else {
        if( !updatedJson.empty() ) {
            updatedJson[ ItemGroup::Sync ] = false;
        }
    }

    return updatedJson;
}
}  // namespace CoreD
}  // namespace HubPlatform
