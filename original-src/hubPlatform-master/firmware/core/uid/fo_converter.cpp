
#include "uid/fo_converter.h"
#include "uid/broadcast.h"

#include "device_model/h/permission.h"
#include "log_utils/ezlo_log.h"

#include "base_utils/ezlodb.h"
#include "database_client/db_tables_names.h"

JsonObject Fo::getUpdatedFields( const JsonObject& item, const JsonObject& fields )  // 2do: move to FO utils
{
    JsonObject updatedFields;
    for( auto it = fields.cbegin(); it != fields.cend(); ++it )
        if( item[ it.key() ] != it.value() )
            updatedFields[ it.key() ] = it.value();
    return updatedFields;
}

void Fo::foUpdateFields( const std::string& dbName,
                         const JsonObject& query,
                         const JsonObject& fields,
                         const MessageSubClasses msgSubclass ) {
    const JsonObject fieldsToReturn = Db::getFieldsToReturn( fields );
    auto objects = Db::findAll( dbName, query, &fieldsToReturn );
    if( !objects.empty() ) {
        Db::update( dbName, query, JSON( "$set" << fields ), false, true );
        for( auto& object : objects ) {
            const JsonObject updatedFields = getUpdatedFields( object, fields );
            if( !updatedFields.empty() )
                Broadcast::sendToUis( msgSubclass, JSON( Db::getIdObj( Db::getId( object ) ) << updatedFields ) );
        }
    }
}

std::vector< JsonObject > Fo::getAvailableDeviceIds( const JsonObject& query, const std::string& restrictedUser ) {
    std::vector< JsonObject > availableDeviceIds;

    for( auto& d : getDevices( query, restrictedUser, "_id" ) ) {
        if( d.is_object() && d.count( "_id" ) ) {
            availableDeviceIds.push_back( std::move( d[ "_id" ] ) );
        }
    }

    return availableDeviceIds;
}

std::vector< JsonObject > Fo::getDevices( JsonObject query,
                                          const std::string& restricted_user,
                                          const std::string& includeSpec ) {
    if( !restricted_user.empty() ) {
        query[ "assigns.user" ] = restricted_user;
        query[ "assigns.permission" ] = JSON( "$in" << Permission::getSharedUserMinimalRights() );
    }
    JsonObject projections =
        ( !includeSpec.empty() ) ? JSON( includeSpec << true ) : JSON( "assigns" << false << "private" << false );

    return Db::findAll( HaDevicesDBName, query, &projections );
}

std::vector< JsonObject > Fo::getItems( JsonObject query, const std::string& restrictedUser ) {
    if( !restrictedUser.empty() ) {
        query[ "deviceId" ] = { { "$in", getAvailableDeviceIds( query, restrictedUser ) } };
    }

    auto allItems = Db::findAll( HaItemsDBName, query );
    for( auto& item : allItems ) {
        const int instanceId = json_get_int( item[ "private" ], "instanceId" );

        item[ "instanceId" ] = instanceId;

        if( instanceId > 0 ) {
            const auto label = json_get_str( item, "label" );
            if( label.find( "[[" ) != std::string::npos && label.find( "]]" ) != std::string::npos ) {
                item[ "label" ] = label + " " + std::to_string( instanceId );
            }
        }

        item.erase( "private" );
    }

    return allItems;
}

std::vector< JsonObject > Fo::getItemsByDeviceId( const std::string& deviceId, const std::string& restrictedUser ) {
    return getItems( JSON( "deviceId" << deviceId ), restrictedUser );
}

void Fo::setPageId( const std::string& deviceId, const std::string& pageId ) {
    if( !deviceId.empty() ) {
        const JsonObject fields = ( pageId.empty() ? JSON( "pageId" << nullptr ) : JSON( "pageId" << pageId ) );
        foUpdateFields( HaDevicesDBName, Db::getIdObj( deviceId ), fields, HUB_DEVICE_UPDATED );
        foUpdateFields( HaItemsDBName, JSON( "deviceId" << deviceId ), fields, HUB_ITEM_UPDATED );
    }
}

void Fo::setParentId( const std::string& dataitemId, const std::string& pageId ) {
    if( !dataitemId.empty() )
        Db::update(
            HaDataItemsDBName,
            Db::getIdObj( dataitemId ),
            JSON( "$set" << ( pageId.empty() ? JSON( "parent_id" << nullptr ) : JSON( "parent_id" << pageId ) ) ) );
}

JsonObject Fo::getButtonMethod( const std::string& label, const std::string& description, const std::string& method ) {
    return JSON( "type"
                 << "button"
                 << "label" << label << "description" << description << "method" << method );
}

bool Instance::getAdd() {
    return add_;
}

void Instance::updateItem( JsonObject& item ) {
    json_add_str( item, "label", label_ );
    json_add_str( item, "type", type_ );
    json_add_bool( item, "show", show_ );
    json_add_bool( item, "hasGetter", hasGetter_ );
    json_add_bool( item, "hasSetter", hasSetter_ );
    if( !enum_.empty() )
        item[ "enum" ] = enum_;
}

void Instance::set( bool show,
                    bool showAsRule,
                    bool hasSetter,
                    bool hasGetter,
                    const std::string& label,
                    const std::string& valueType,
                    const JsonObject& valuesEnum ) {
    if( show ) {
        show_ = true;
        label_ = label;
    }
    if( show || showAsRule )
        add_ = true;
    if( hasSetter )
        hasSetter_ = true;
    if( hasGetter )
        hasGetter_ = true;
    if( !valueType.empty() )
        type_ = valueType;
    if( !valuesEnum.empty() )
        enum_ = valuesEnum;
}

std::string Instance::getPageId( const std::string& /*id*/ ) {
    return "";
}

void Fo::renameDevice( const std::string& deviceId, const std::string& name ) {
    foUpdateFields( HaDevicesDBName, Db::getIdObj( deviceId ), JSON( "name" << name ), HUB_DEVICE_UPDATED );
}

bool Fo::hasDeviceWithName( const std::string& name, const std::string& id ) {
    return Db::has( HaDevicesDBName, JSON( "_id" << JSON( "$ne" << id ) << "name" << name ) );
}

bool Fo::hasItem( const std::string& id ) {
    return Db::has( HaItemsDBName, JSON( "_id" << JSON( "$eq" << id ) ) );
}

void Fo::updateItemFields( const std::string& itemId, const JsonObject& fields ) {
    foUpdateFields( HaItemsDBName, Db::getIdObj( itemId ), fields, HUB_ITEM_UPDATED );
}

std::vector< std::string > Fo::getOnUnhiddenItems() {
    std::vector< std::string > itemsList;
    std::string itemId;
    const JsonObject fieldstoReturn = JSON_EMPTY_OBJ;
    for( auto& item : Db::findAll( HaItemsDBName,
                                   JSON( "isOn" << true << "show" << true << "$or"
                                                << JSON_ARRAY( JSON( "isOnHidden" << false ) << JSON(
                                                                   "isOnHidden" << JSON( "$exists" << false ) ) ) ),
                                   &fieldstoReturn ) ) {
        if( json_get( item, "_id", itemId ) )
            itemsList.push_back( itemId );
    }
    return itemsList;
}
