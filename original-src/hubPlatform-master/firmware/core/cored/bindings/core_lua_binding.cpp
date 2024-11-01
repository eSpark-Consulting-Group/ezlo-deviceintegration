#include "binding_interfaces/lua/lua_binding_interface.h"
#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/exceptions.h"
#include "cored/model/values/value.h"
#include "cored/model/configuration/configuration.h"
#include "cored/model/exceptions.h"
#include "cored/model/cored_device_setting.h"
#include "cored/resources/core_resources.h"
#include "cored/model/cored_item_checker.h"

#include "uid/remote_interface/ui_remote_interface.h"

#include "service_interfaces/service.h"
#include "service_interfaces/script_client.h"
#include "database_client/idbclient.h"
#include "json_utils/json_fwd.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"
#include "base_utils/timeutils.h"
#include "jsonrpc/error_codes.h"

#include "common_utils/cpp14.h"

#include "bus/base_bus.h"

#include <stdexcept>
#include <vector>
#include <utility>
#include <string>
#include <numeric>

#define CHECK_LUA_INPUT_PARAMETERS( number ) CHECK_INPUT_PARAMETERS( ( std::string{ "core." } + __func__ ), number )
#define CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( number ) \
    CHECK_INPUT_OPTIONAL_PARAMETERS( ( std::string{ "core." } + __func__ ), number )

#define GET_LUA_INPUT_PARAMETERS( type, idx ) GET_INPUT_PARAMETERS( type, idx, ( std::string{ "core." } + __func__ ) )

namespace HubPlatform {
namespace {

const KeyMap ItemDbToLuaMap = { { "_id", "id" },
                                { "type", "value_type" },
                                { "elementsMaxNumber2", "elements_max_number_2" } };

const KeyMap RoomDbToLuaMap = { { "_id", "id" }, { "name", "name" } };

CoreDRemoteInterface::ValueLoader makeValueLoader( const Json& source ) {
    return [&source]( CoreD::Model::ValueType valueType ) {
        return CoreD::Model::ValueFactory::createItemValue( valueType, source );
    };
}

CoreDRemoteInterface::ValueLoader makeEmptyValueLoader() {
    return []( CoreD::Model::ValueType valueType ) { return CoreD::Model::ValueUPtr{}; };
}

class CoreLuaBindingError : public std::runtime_error {
public:
    explicit CoreLuaBindingError( const std::string& errDescription )
    : runtime_error( "CoreLuaBinding: " + errDescription ) {}
};

std::unique_ptr< CoreD::Model::Device > pullDevice( const std::string& gatewayId, lua_State* luaState, int idx = -1 ) {
    luaL_checktype( luaState, idx, LUA_TTABLE );

    auto name = lookupLuaTable< NativeString >( luaState, "name", idx, true );
    auto type = lookupLuaTable< NativeString >( luaState, "type", idx, true );
    auto deviceTypeId = lookupLuaTable< NativeString >( luaState, "device_type_id", idx, true );
    auto category = lookupLuaTable< NativeString >( luaState, "category", idx, true );
    auto batteryPowered = lookupLuaTable< NativeBool >( luaState, "battery_powered", idx, true );

    auto info = lookupLuaTable< NativeStringStringTable >( luaState, "info", idx );
    auto firmware = lookupLuaTable< Json >( luaState, "firmware", idx );
    auto subcategory = lookupLuaTable< NativeString >( luaState, "subcategory", idx );
    auto reachable = lookupLuaTable< NativeBool >( luaState, "reachable", idx );
    auto parentDeviceId = lookupLuaTable< NativeString >( luaState, "parent_device_id", idx );
    auto roomId = lookupLuaTable< NativeString >( luaState, "room_id", idx );
    auto security = lookupLuaTable< NativeString >( luaState, "security", idx );
    auto status = lookupLuaTable< NativeString >( luaState, "status", idx );
    auto ready = lookupLuaTable< NativeBool >( luaState, "ready", idx );
    auto persistent = lookupLuaTable< NativeBool >( luaState, "persistent", idx );
    auto houseModesOptions = lookupLuaTable< Json >( luaState, "house_modes_options", idx );
    auto parentRoom = lookupLuaTable< NativeBool >( luaState, "parent_room", idx );

    auto device = CoreD::Model::Device{ gatewayId,        name.value(),          type.value(), deviceTypeId.value(),
                                        category.value(), batteryPowered.value() };

    if( info ) {
        std::vector< DeviceAdditionalInfo > result;
        for( const auto& additionalInfo : info.value() ) {
            result.emplace_back( additionalInfo.first, additionalInfo.second );
        }
        device.setAdditionalInfo( std::move( result ) );
    }

    if( firmware ) {
        std::vector< FirmwareInfo > result;
        for( const auto& firmwareInfo : firmware.value() ) {
            auto id = json_get_str( firmwareInfo, "id" );
            auto version = json_get_str( firmwareInfo, "version" );

            result.push_back( { id, version } );
        }
        device.setFirmware( std::move( result ) );
    }

    if( houseModesOptions ) {
        Json options = houseModesOptions.value();
        auto renameSnakeCaseField =
            []( Json& options, const std::string& sourceField, const std::string& targetField ) {
                if( sourceField.empty() || targetField.empty() || sourceField == targetField )
                    return;

                auto fieldIt = options.find( sourceField );
                if( options.end() != fieldIt ) {
                    options[ targetField ] = *fieldIt;
                    options.erase( fieldIt );
                }
            };

        using namespace CoreD::Model::Resources::DeviceFields;
        renameSnakeCaseField( options, "alarms_off", HouseModesOptions::AlarmsOffFieldName );
        renameSnakeCaseField( options, "cameras_off", HouseModesOptions::CamerasOffFieldName );

        device.setHouseModesOptions( options );
    }

    if( reachable )
        device.setReachable( reachable.value() );
    if( parentDeviceId )
        device.setParentDeviceId( parentDeviceId.value() );
    if( subcategory )
        device.setSubcategory( subcategory.value() );
    if( roomId )
        device.setRoomId( roomId.value() );
    if( parentRoom ) {
        if( parentDeviceId )
            device.setParentRoom( parentRoom.value() );
        else
            throw CoreLuaBindingError{ "Failed to modify parent_room field: allowed only for child devices" };
    }
    if( security ) {
        if( !device.setSecurity( security.value() ) ) {
            return nullptr;
        }
    }
    if( status && !device.setStatus( status.value() ) ) {
        return nullptr;
    }
    if( ready )
        device.setReady( ready.value() );
    if( persistent )
        device.setPersistent( persistent.value() );

    return std::make_unique< CoreD::Model::Device >( std::move( device ) );
}

CoreD::Model::Item pullItem( lua_State* luaState, int idx = -1 ) {
    using namespace CoreD::Model;

    luaL_checktype( luaState, idx, LUA_TTABLE );

    auto name = lookupLuaTable< NativeString >( luaState, "name", idx, true );
    auto valueType = lookupLuaTable< NativeString >( luaState, "value_type", idx, true );
    auto hasGetter = lookupLuaTable< NativeBool >( luaState, "has_getter", idx, true );
    auto hasSetter = lookupLuaTable< NativeBool >( luaState, "has_setter", idx, true );
    auto value = lookupLuaTable< Json >( luaState, "value", idx, true );
    auto deviceID = lookupLuaTable< NativeString >( luaState, "device_id", idx, true );

    const auto itemValueType = ValueType::fromString( valueType.value() );
    if( itemValueType == ValueType::Enum::NoValue ) {
        throw CoreLuaBindingError{ "core_lua_binding::pullItem - invalid item value type" };
    }

    auto item =
        Item{ deviceID.value(),  name.value(),      itemValueType,
              hasGetter.value(), hasSetter.value(), ValueFactory::createItemValue( itemValueType, value.value() ) };

    if( auto info = lookupLuaTable< Json >( luaState, "info", idx ) ) {
        item.setInfo( info.value() );
    }

    if( auto show = lookupLuaTable< NativeBool >( luaState, "show", idx ) ) {
        item.setShow( show.value() );
    }

    if( auto valueMin = lookupLuaTable< Json >( luaState, "value_min", idx ) ) {
        auto itemValue = ValueFactory::createItemValue( itemValueType, valueMin.value() );
        item.setLowerLimitValue( std::move( itemValue ) );
    }

    if( auto valueMax = lookupLuaTable< Json >( luaState, "value_max", idx ) ) {
        auto itemValue = ValueFactory::createItemValue( itemValueType, valueMax.value() );
        item.setUpperLimitValue( std::move( itemValue ) );
    }

    auto enumeration = lookupLuaTable< Json >( luaState, "enum", idx );
    if( enumeration && !enumeration.value().empty() ) {
        item.setEnum( enumeration.value() );
    }

    size_t containerLevel = 0;
    for( auto type = &itemValueType; type && type->isContainer(); type = type->getSubType().get(), ++containerLevel ) {
        const auto key = containerLevel ? std::string( "elements_max_number_" ) + std::to_string( containerLevel + 1 ) :
                                          std::string( "elements_max_number" );

        if( auto elementsMaxNumber = lookupLuaTable< NativeInteger >( luaState, key, idx ) ) {
            item.setElementsMaxNumber( elementsMaxNumber.value(), containerLevel );
        }
    }

    auto oneWeekDayCost = lookupLuaTable< NativeInteger >( luaState, "one_week_day_cost", idx );
    auto oneShiftedWeekDayCost = lookupLuaTable< NativeInteger >( luaState, "one_shifted_week_day_cost", idx );
    if( oneWeekDayCost || oneShiftedWeekDayCost ) {
        item.setWeekDayCost(
            static_cast< Item::WeekDayCost >( oneWeekDayCost ? oneWeekDayCost.value() : 0 ),
            static_cast< Item::WeekDayCost >( oneShiftedWeekDayCost ? oneShiftedWeekDayCost.value() : 0 ) );
    }

    auto userCodeRestriction = lookupLuaTable< NativeString >( luaState, "user_code_restriction", idx );
    if( userCodeRestriction ) {
        item.setUserCodeRestriction( userCodeRestriction.value() );
    }

    auto userCodeModes = lookupLuaTable< Json >( luaState, "user_code_modes", idx );
    if( userCodeModes ) {
        item.setUserCodeModes( userCodeModes.value() );
    }

    auto stringRestriction = lookupLuaTable< NativeString >( luaState, "string_restriction", idx );
    if( stringRestriction ) {
        item.setStringRestriction( stringRestriction.value() );
    }

    auto valueIndent = lookupLuaTable< Json >( luaState, "value_indent", idx );
    if( valueIndent ) {
        Json vi = valueIndent.value();
        std::string itemId;
        float minIndent;
        if( !json_get( vi, "item", itemId ) || itemId.empty() ) {
            throw LuaError{ "wrong 'item' field within 'value_indent'" };
        }
        if( !json_get( vi, "min_indent", minIndent ) ) {
            throw LuaError{ "wrong 'min_indent' field within 'value_indent'" };
        }
        item.setValueIndent( std::make_unique< Item::ValueIndent >( itemId, minIndent ) );
    }

    return item;
}

CoreD::Model::DeviceSetting pullSetting( lua_State* luaState, int idx = -1 ) {
    const auto joSetting = pullJson( luaState, idx, snakeToCamelCase );

    return CoreD::Model::DeviceSetting::fromJson( joSetting );
}

ScriptClient createScriptClient( lua_State* luaState ) {
    auto& service = dynamic_cast< Services::Service& >( getHostService( luaState ) );
    return ScriptClient( GroupsNames::CORED, service );
}

namespace Core = CoreDRemoteInterface;

std::string busLongErrorMessage() {
    return json_get_str( Errors::get( Errors::BUS_MESSAGE_TOO_LONG ), "message" );
}

bool isBusLongDataError( const JsonObject& response ) {
    const auto& error = response[ "error" ];
    if( !error.empty() && json_get_str( error, "data" ) == "bus.message.too.long" )
        return true;
    return false;
}

Core::CoreDInterface::GatewayID getGatewayIdByDeviceId( Core::CoreDInterface& coreDRemoteInterface,
                                                        const Core::CoreDInterface::DeviceID& deviceId ) {
    using namespace CoreD::Model::Resources;
    const auto device = coreDRemoteInterface.getSingleDevice( deviceId );
    return json_get_str( device, DeviceFields::GatewayIdFieldName );
}

Core::CoreDInterface::GatewayID getGatewayIdByItemId( Core::CoreDInterface& coreDRemoteInterface,
                                                      const Core::CoreDInterface::ItemID& itemId ) {
    using namespace CoreD::Model::Resources;
    const auto deviceId = json_get_str( coreDRemoteInterface.getItem( itemId ), ItemFields::DeviceIdFieldName );
    return getGatewayIdByDeviceId( coreDRemoteInterface, deviceId );
}

Core::CoreDInterface::GatewayID getGatewayIdBySettingId( Core::CoreDInterface& coreDRemoteInterface,
                                                         const Core::CoreDInterface::DeviceSettingId& settingId ) {
    using namespace CoreD::Model::Resources;
    const auto deviceId =
        json_get_str( coreDRemoteInterface.getDeviceSetting( settingId ), DeviceSettingFields::DeviceId );
    return getGatewayIdByDeviceId( coreDRemoteInterface, deviceId );
}

Core::CoreDInterface::GatewayID getGatewayIdByGatewaySettingId(
    Core::CoreDInterface& coreDRemoteInterface,
    const Core::CoreDInterface::GatewaySettingId& settingId ) {
    using namespace CoreD::Model::Resources;
    return json_get_str( coreDRemoteInterface.getGatewaySetting( settingId ), GatewaySettingFields::GatewayId );
}

bool isGatewayAndPluginGatewaySame( Core::CoreDInterface& coreDRemoteInterface,
                                    const Core::CoreDInterface::GatewayID& gatewayId,
                                    const std::string& pluginId ) {
    using namespace CoreD::Model::Resources;
    const auto pluginGatewayId =
        json_get_str( coreDRemoteInterface.getGatewayByPluginId( pluginId ), GatewayFields::IdFieldName );
    LOGGER_TRACE( "gatewayId: " + gatewayId + ", pluginGatewayId: " + pluginGatewayId );
    return gatewayId == pluginGatewayId;
}

bool isDeviceAndPluginGatewaysSame( Core::CoreDInterface& coreDRemoteInterface,
                                    const Core::CoreDInterface::DeviceID& deviceId,
                                    const std::string& pluginId ) {
    const auto gatewayId = getGatewayIdByDeviceId( coreDRemoteInterface, deviceId );
    return isGatewayAndPluginGatewaySame( coreDRemoteInterface, gatewayId, pluginId );
}

bool isItemAndPluginGatewaysSame( Core::CoreDInterface& coreDRemoteInterface,
                                  const Core::CoreDInterface::ItemID& itemId,
                                  const std::string& pluginId ) {
    const auto gatewayId = getGatewayIdByItemId( coreDRemoteInterface, itemId );
    return isGatewayAndPluginGatewaySame( coreDRemoteInterface, gatewayId, pluginId );
}

bool isSettingAndPluginGatewaysSame( Core::CoreDInterface& coreDRemoteInterface,
                                     const Core::CoreDInterface::DeviceSettingId& settingId,
                                     const std::string& pluginId ) {
    const auto gatewayId = getGatewayIdBySettingId( coreDRemoteInterface, settingId );
    return isGatewayAndPluginGatewaySame( coreDRemoteInterface, gatewayId, pluginId );
}

bool isGatewaySettingAndPluginGatewaysSame( Core::CoreDInterface& coreDRemoteInterface,
                                            const Core::CoreDInterface::GatewaySettingId& settingId,
                                            const std::string& pluginId ) {
    const auto gatewayId = getGatewayIdByGatewaySettingId( coreDRemoteInterface, settingId );
    return isGatewayAndPluginGatewaySame( coreDRemoteInterface, gatewayId, pluginId );
}

bool checkGatewaySetting( Core::CoreDInterface& coreDRemoteInterface,
                          const Core::CoreDInterface::GatewaySettingId& settingId ) {
    return !coreDRemoteInterface.getGatewaySetting( settingId ).empty();
}

std::string getDictionaryElementType( CoreDRemoteInterface::CoreDRemoteInterface& coreDRemoteInterface,
                                      const CoreDRemoteInterface::CoreDInterface::ItemID& itemId ) {
    using namespace CoreD::Model::Resources;
    auto item = coreDRemoteInterface.getItem( itemId );
    LOGGER_TRACE( cc::j( item ) );
    auto type = json_get_str( item, ItemFields::TypeFieldName );
    auto separator = type.find( '.' );
    return type.substr( separator );
}

/* arguments: no
 * return: gatewayS (table) or nil if failed to get gateways
 *          { [0] = { gateway }, [1] = {},... [n - 1] = {}  } }
 *
 *          gateway = { ... }
 * @note there's no 'addGateway' lua binding, 'cause gateways are going to be added only from c++ code */
int getGateways( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto gateways = coreDRemoteInterface.getGateways();

        if( gateways.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            pushJsonAsLuaObj(
                luaState, gateways, createMappingKeyFormatter( DefaultDbToLuaKeyMap, camelToSnakeCase ), 1 );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get gateways : %s", e.what() );
    }

    return 1;
}

/* arguments: no
 * return: gateway(table) of a running plugin or nil if failed to get gateways */
int getGateway( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        auto gateway = coreDRemoteInterface.getGatewayByPluginId( getPluginId( luaState ) );

        if( gateway.is_object() && !gateway.empty() ) {
            pushJsonAsLuaObj( luaState, gateway, createMappingKeyFormatter( DefaultDbToLuaKeyMap, camelToSnakeCase ) );
        }
        else {
            lua_pushnil( luaState );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get gateway : %s", e.what() );
    }

    return 1;
}

template < typename T >
void fillJsonFieldFromAnotherCase( Json& destination,
                                   const std::string& destinationFieldName,
                                   const Json& source,
                                   const std::string& sourceFieldName ) {
    T field;
    if( json_get( source, sourceFieldName, field ) )
        destination[ destinationFieldName ] = field;
}

/* arguments: fields (table)
 *  optional  [ "manual_device_adding" ] = string,
 *  optional  [ "ready" ] = bool */
int updateGateway( lua_State* luaState ) {
    using namespace CoreD::Model::Resources;

    CHECK_LUA_INPUT_PARAMETERS( 1 );

    const auto fieldsRaw = GET_LUA_INPUT_PARAMETERS( Json, 1 );

    Json fields;
    fillJsonFieldFromAnotherCase< std::string >(
        fields, GatewayFields::ManualDeviceAddingFieldName, fieldsRaw, "manual_device_adding" );
    fillJsonFieldFromAnotherCase< bool >( fields, GatewayFields::ReadyFieldName, fieldsRaw, "ready" );
    fillJsonFieldFromAnotherCase< JsonObject >( fields, GatewayFields::InfoFieldName, fieldsRaw, "info" );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        auto gateway = coreDRemoteInterface.getGatewayByPluginId( getPluginId( luaState ) );

        if( gateway.is_object() && !gateway.empty() ) {
            const auto gatewayId = json_get_str( gateway, GatewayFields::IdFieldName );

            coreDRemoteInterface.updateGateway( gatewayId, fields );
        }
        else {
            return luaL_error( luaState, "unable to get current gateway" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to modify gateway : %s", e.what() );
    }

    return 0;
}

Json processGatewaySettingJson( lua_State* luaState, const Json& gatewaySetting ) {
    using namespace CoreD::Model::Resources;
    Json setting = gatewaySetting;

    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

    jsonCheckString( setting, "id" );

    /* add/evaluate a gatewayId based on a calling plugin */
    const auto gateway = coreDRemoteInterface.getGatewayByPluginId( getPluginId( luaState ) );
    setting[ GatewaySettingFields::GatewayId ] = json_get_str( gateway, GatewayFields::IdFieldName );
    setting[ GatewaySettingFields::Id ] = getPluginId( luaState ) + "." + json_get_str( setting, "id" );

    return setting;
}

CoreD::Model::GatewaySetting pullGatewaySetting( lua_State* luaState, int idx = -1 ) {
    auto gatewaySetting = pullJson( luaState, idx, snakeToCamelCase );
    gatewaySetting = processGatewaySettingJson( luaState, gatewaySetting );

    return CoreD::Model::GatewaySetting::fromJson( gatewaySetting );
}

/* arguments: fields (table)
 *  mandatory  [ "id" ] = string,
 *  mandatory  [ "status" ] = string
 *  mandatory  [ "value_type" ] = string
 *  mandatory  [ "value" ] = depends on a 'value_type'
 *  */
int addGatewaySetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );
        const auto setting = pullGatewaySetting( luaState );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        const auto result = coreDRemoteInterface.addGatewaySetting( setting );
        if( result.second ) {
            lua_pushlstring( luaState, result.first.c_str(), result.first.size() );
        }
        else {
            lua_pushnil( luaState );
        }

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to add a gateway setting : %s", e.what() );
    }
}

/* arguments:
 * mandatory setting_id = string,
 */
int removeGatewaySetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );
        const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( !checkGatewaySetting( coreDRemoteInterface, settingId ) ) {
            throw CoreD::Model::CoreDModelError( "setting " + settingId + " doesn't exist" );
        }
        if( !isGatewaySettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            throw CoreD::Model::CoreDModelError( "setting isn't owned by the running plugin, pluginId: " +
                                                 getPluginId( luaState ) + ", settingId: " + settingId );
        }

        coreDRemoteInterface.removeGatewaySetting( settingId );

        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove a gateway setting : %s", e.what() );
    }
}

/* arguments:
 * mandatory setting_id = string,
 * mandatory value = depends on a setting type,
 */
int setGatewaySettingValue( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 )
        const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto value = GET_LUA_INPUT_PARAMETERS( Json, 2 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( !checkGatewaySetting( coreDRemoteInterface, settingId ) ) {
            throw CoreD::Model::CoreDModelError( "setting " + settingId + " doesn't exist" );
        }

        if( !isGatewaySettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            throw CoreD::Model::CoreDModelError( "setting isn't owned by the running plugin, pluginId: " +
                                                 getPluginId( luaState ) + ", settingId: " + settingId );
        }

        coreDRemoteInterface.updateGatewaySettingValue( settingId, makeValueLoader( value ) );
    }
    catch( const std::exception& error ) {
        return luaL_error( luaState, "fail to set value for a gateway setting : %s", error.what() );
    }
    return 0;
}

/* arguments:
 * mandatory setting_id = string,
 * mandatory status = string,
 */
int setGatewaySettingStatus( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 )
        const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto statusStr = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( !checkGatewaySetting( coreDRemoteInterface, settingId ) ) {
            throw CoreD::Model::CoreDModelError( "setting " + settingId + " doesn't exist" );
        }

        if( !isGatewaySettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            throw CoreD::Model::CoreDModelError( "setting isn't owned by the running plugin, pluginId: " +
                                                 getPluginId( luaState ) + ", settingId: " + settingId );
        }

        const auto status = CoreD::Model::SynchronizationStatus::fromString( statusStr );
        coreDRemoteInterface.updateGatewaySettingStatus( settingId, status );
    }
    catch( const std::exception& error ) {
        return luaL_error( luaState, "fail to set status for a gateway setting : %s", error.what() );
    }
    return 0;
}

/* arguments:
 * mandatory gateway_id = string,
 */
int getGatewaySettingIdsByGatewayId( lua_State* luaState ) {
    using namespace CoreD::Model::Resources;
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );
        /* TODO: should we allow "read" access to gateways setting for every plugin? */
        const auto gatewayId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto settings = coreDRemoteInterface.getGatewaySettings( gatewayId );
        if( settings.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            JsonArray ids;

            for( const auto& setting : settings ) {
                ids.push_back( json_get_str( setting, GatewaySettingFields::Id ) );
            }

            pushJsonAsLuaObj( luaState, ids );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get gateway settings : %s", e.what() );
    }
    return 1;
}

/* arguments:
 * mandatory setting_id = string,
 */
int getGatewaySetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );
        const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        auto setting = coreDRemoteInterface.getGatewaySetting( settingId );
        if( setting.empty() )
            lua_pushnil( luaState );
        else
            pushJsonAsLuaObj( luaState, setting, createMappingKeyFormatter( DefaultDbToLuaKeyMap, camelToSnakeCase ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get a gateway setting : %s", e.what() );
    }

    return 1;
}

std::string fetchGatewayId( lua_State* luaState ) {
    using namespace CoreD::Model::Resources;
    CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

    const std::string pluginId = getPluginId( luaState );
    Json gateway = coreDRemoteInterface.getGatewayByPluginId( pluginId );
    std::string gatewayId;
    const std::string idFieldName = GatewayFields::IdFieldName;
    if( gateway.is_object() && gateway.count( idFieldName ) > 0 ) {
        gatewayId = json_get_str( gateway, idFieldName );
    }
    else {
        LOGGER_ERROR( "Could not fetch plugin's gateway, pluginId: " + pluginId );
        throw std::runtime_error( "Could not fetch plugin's gateway, pluginId: " + pluginId );
    }
    return gatewayId;
}

/* arguments: device (table)
 *             [ "name" ] = string,
 *             [ "type" ] = string,
 *             [ "device_type_id" ] = string,
 *             [ "category " ] = string,
 *             [ "battery_powered" ] = bool,
 *  optional   [ "subcategory" ] = string,
 *  optional   [ "room_id" ] = string,
 *  optional   [ "info" ] = { [ "some_key" ] = string, [ "some_key" ] = string, ... }
 *  optional   [ "firmware" ] = { { [ "id" ] = string, [ "version" ] = string }, ... }
 *  optional   [ "reachable" ] = bool
 *  optional   [ "parent_device_id" ] = string
 *  optional   [ "persistent" ] = bool
 *  optional   [ "parent_room" ] = bool
 *  optional   [ "house_modes_options" ] = { disarmed: string[], alarms_off: string[] }
 *      disarmed: optional
 *      alarms_off: optional
 *      cameras_off: optional
 * return: device id (string) or nil if failed to add a device */
int addDevice( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const std::string pluginId = getPluginId( luaState );
        std::string gatewayId = fetchGatewayId( luaState );

        auto device = pullDevice( gatewayId, luaState, 1 );

        if( !device ) {
            return 0;
        }
        if( isGatewayAndPluginGatewaySame( coreDRemoteInterface, device->getGatewayId(), getPluginId( luaState ) ) ) {
            auto result = coreDRemoteInterface.addDevice( *device );
            if( result.second ) {
                lua_pushlstring( luaState, result.first.c_str(), result.first.size() );
            }
            else {
                lua_pushnil( luaState );
            }
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to add device : %s", e.what() );
    }

    return 1;
}

/* arguments: "device_id" = string
 *             device (table)
 *             [ "name" ] = string,
 *             [ "type" ] = string,
 *             [ "device_type_id" ] = string,
 *             [ "category " ] = string,
 *             [ "battery_powered" ] = bool,
 *  optional   [ "subcategory" ] = string,
 *  optional   [ "room_id" ] = string,
 *  optional   [ "info" ] = { [ "some_key" ] = string, [ "some_key" ] = string, ... }
 *  optional   [ "firmware" ] = { { [ "id" ] = string, [ "version" ] = string }, ... }
 *  optional   [ "reachable" ] = bool
 *  optional   [ "parent_device_id" ] = string
 *  optional   [ "persistent" ] = bool
 *  optional   [ "parent_room" ] = bool
 *  optional   [ "house_modes_options" ] = { disarmed: string[], alarms_off: string[] }
 *      disarmed: optional
 *      alarms_off: optional
 *      cameras_off: optional
 */
int modifyDevice( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto deviceId = std::string{ lua_tostring( luaState, 1 ) };
        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, deviceId, getPluginId( luaState ) ) ) {
            std::string gatewayId = fetchGatewayId( luaState );

            auto device = pullDevice( gatewayId, luaState, 2 );

            if( !device ) {
                return 0;
            }

            coreDRemoteInterface.modifyDevice( deviceId, *device );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to modify device : %s", e.what() );
    }

    return 0;
}

/* TODO: do we have to expose this functionality to Lua? What to return? */

/* arguments: no
 * return: deviceS (table) or nil if failed to get devices
 *          { [0] = { device }, [1] = {},... [n - 1] = {}  } }
 *
 *          device = { [ "defaultName" ] = string,
 *                     [ "type" ] = string, ... } */
int getDevices( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto devices = coreDRemoteInterface.getDevices();

        if( devices.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            pushJsonAsLuaObj(
                luaState, devices, createMappingKeyFormatter( DefaultDbToLuaKeyMap, camelToSnakeCase ), 1 );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get devices : %s", e.what() );
    }

    return 1;
}

/* arguments: no
 * return: devices ids (array of strings) or nil if failed to get devices_ids */
int getDevicesIds( lua_State* luaState ) {
    using namespace CoreD::Model::Resources;
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto devices = coreDRemoteInterface.getDevices();

        if( devices.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            JsonArray ids;

            for( const auto& device : devices ) {
                ids.push_back( json_get_str( device, DeviceFields::IdFieldName ) );
            }

            pushJsonAsLuaObj( luaState, ids );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get devices ids : %s", e.what() );
    }

    return 1;
}

/* arguments: "device_id" = string
 * return: device (table) or nil if device doesn't exist by given device_id */
int getDevice( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    const auto deviceId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );
        const auto device = coreDRemoteInterface.getSingleDevice( deviceId );

        if( device.empty() )
            lua_pushnil( luaState );
        else
            pushJsonAsLuaObj( luaState, device, createMappingKeyFormatter( DefaultDbToLuaKeyMap, camelToSnakeCase ) );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get device with device id (%s) : %s", deviceId.c_str(), e.what() );
    }
}

/* arguments: "device_id" = string
 * return: device_id (string) or nil if device doesn't exist by given device_id */
int getRootDeviceId( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    const auto deviceId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );
        const auto rootDeviceId = coreDRemoteInterface.getRootDeviceId( deviceId );

        if( rootDeviceId.empty() )
            lua_pushnil( luaState );
        else
            lua_pushstring( luaState, rootDeviceId.c_str() );

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get device with device id (%s) : %s", deviceId.c_str(), e.what() );
    }
}

/* arguments: "dev_id" = string
 * return: none */
int removeDevice( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto deviceId = std::string{ lua_tostring( luaState, 1 ) };
        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, deviceId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.removeDevice( deviceId );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get device : %s", e.what() );
    }

    return 0;
}

/* arguments: "dev_id" = string
 * return: none */
int removeDeviceSync( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto deviceId = std::string{ lua_tostring( luaState, 1 ) };
        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, deviceId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.removeDeviceSync( deviceId );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove device : %s", e.what() );
    }

    return 0;
}

/* arguments: "gateway_id" = string
 * return: none */
int removeGatewayDevices( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const std::string gatewayId{ lua_tostring( luaState, 1 ) };
        if( isGatewayAndPluginGatewaySame( coreDRemoteInterface, gatewayId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.removeGatewayDevices( gatewayId );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove devices : %s", e.what() );
    }

    return 0;
}

/* arguments: item (table)
 *             [ "device_id" ] = string,
 *             [ "name" ] = string,
 *             [ "value_type" ] = string [int | bool | float | string | rgb | userCode | camLiveStream | token |
 * temperature],
 *             [ "value" ] = [int | bool | float | string | rgb | userCode | camLiveStream | temperature | button_state]
 *             [ "has_getter" ] = bool,
 *             [ "has_setter" ] = bool,
 *  optional   [ "show" ] = bool,
 *  optional   [ "enum" ] = array,
 *  optional   [ "value_min" ] = [int | bool | float | string | rgb | userCode | camLiveStream | temperature]
 *  optional   [ "value_max" ] = [int | bool | float | string | rgb | userCode | camLiveStream | temperature]
 *  optional   [ "elementsMaxNumber" ] = int
 *  optional   [ "userCodeRestriction" ] = string
 * return: item id (string) or nil if failed to add an item */
int addItem( lua_State* luaState ) {
    using namespace CoreD::Model::Resources;
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        auto item = pullItem( luaState, 1 );

        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, item.getDeviceId(), getPluginId( luaState ) ) ) {
            auto result = coreDRemoteInterface.addItem( item );
            if( result.second ) {
                lua_pushlstring( luaState, result.first.c_str(), result.first.size() );
            }
            else {
                lua_pushnil( luaState );
            }
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to add item : %s", e.what() );
    }

    return 1;
}

/* arguments: "item_id" = string
 *             item (table)
 *             [ "device_id" ] = string,
 *             [ "name" ] = string,
 *             [ "value_type" ] = string [int | bool | float | string | rgb | userCode | camLiveStream | token |
 * temperature],
 *             [ "value" ] = [int | bool | float | string | rgb | userCode | camLiveStream | temperature | button_state]
 *             [ "has_getter" ] = bool,
 *             [ "has_setter" ] = bool,
 *  optional   [ "show" ] = bool,
 *  optional   [ "enum" ] = array,
 *  optional   [ "value_min" ] = [int | bool | float | string | rgb | userCode | camLiveStream | temperature]
 *  optional   [ "value_max" ] = [int | bool | float | string | rgb | userCode | camLiveStream | temperature]
 *  optional   [ "elementsMaxNumber" ] = int
 *  optional   [ "userCodeRestriction" ] = string */
int modifyItem( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        using namespace CoreD::Model::Resources;
        const auto itemId = std::string{ lua_tostring( luaState, 1 ) };
        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            auto item = pullItem( luaState, 2 );
            auto itemJson = item.toJson();
            auto existingItem = coreDRemoteInterface.getItem( itemId );
            if( !existingItem.empty() ) {
                auto existingValueType = json_get_str( existingItem, ItemFields::TypeFieldName );
                auto newValueType = json_get_str( itemJson, ItemFields::TypeFieldName );
                if( !newValueType.empty() && existingValueType != newValueType ) {
                    if( json_contains( existingItem, ItemFields::ValueMaxFieldName ) &&
                        json_contains( existingItem, ItemFields::ValueMinFieldName ) ) {
                        if( !json_contains( itemJson, ItemFields::ValueMaxFieldName ) &&
                            !json_contains( itemJson, ItemFields::ValueMinFieldName ) ) {
                            return luaL_error( luaState, "Modify item should contain value_min and value_max fields " );
                        }
                    }
                }
            }
            coreDRemoteInterface.modifyItem( itemId, item );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to modify item : %s", e.what() );
    }
    return 0;
}

/* arguments: "item_id" = string
 * return: none */
int removeItem( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto itemId = std::string{ lua_tostring( luaState, 1 ) };
        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.removeItem( itemId );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove item : %s", e.what() );
    }
    return 0;
}

/* arguments: no
 * return: itemS (table) or nil if failed to get items
 *          { [0] = { item }, [1] = {},... [n - 1] = {}  } }
 *
 *          item = { [ "type" ] = string,
 *                     [ "show" ] = bool, ... } */
int getItems( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto items = coreDRemoteInterface.getItems();

        if( items.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            pushJsonAsLuaObj( luaState, items, createMappingKeyFormatter( ItemDbToLuaMap, camelToSnakeCase ), 1 );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get items : %s", e.what() );
    }
    return 1;
}

/* arguments: "dev_id" = string
 * return: device (table) or nil if failed to get device
 *          device = { [ "defaultName" ] = string,
 *                     [ "type" ] = string, ... } */
int getItemsByDeviceId( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto deviceId = std::string{ lua_tostring( luaState, 1 ) };
        const auto items = coreDRemoteInterface.getItemsByDeviceId( deviceId );

        if( items.empty() )
            lua_pushnil( luaState );
        else
            pushJsonAsLuaObj( luaState, items, createMappingKeyFormatter( ItemDbToLuaMap, camelToSnakeCase ), 1 );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get items : %s", e.what() );
    }
    return 1;
}

/* arguments: "item_id" = string
 * return: item (table) or nil if failed to get item
 *          item = { [ "type" ] = string,
 *                     [ "show" ] = bool, ... } */
int getItem( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        auto item = coreDRemoteInterface.getItem( itemId );

        if( item.empty() )
            lua_pushnil( luaState );
        else
            pushJsonAsLuaObj( luaState, item, createMappingKeyFormatter( ItemDbToLuaMap, camelToSnakeCase ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get item : %s", e.what() );
    }
    return 1;
}

int getSettingIdsByDeviceId( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto deviceId = std::string{ lua_tostring( luaState, 1 ) };
        auto settings = coreDRemoteInterface.getDeviceSettingIdsByDeviceId( deviceId );

        if( settings.empty() )
            lua_pushnil( luaState );
        else
            pushJsonAsLuaObj( luaState, settings );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get device settings : %s", e.what() );
    }
    return 1;
}

int getSetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        auto setting = coreDRemoteInterface.getDeviceSetting( settingId );

        if( setting.empty() )
            lua_pushnil( luaState );
        else
            pushJsonAsLuaObj( luaState, setting, createMappingKeyFormatter( DefaultDbToLuaKeyMap, camelToSnakeCase ) );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get device setting : %s", e.what() );
    }

    return 1;
}

int addSetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        const auto setting = pullSetting( luaState );
        const auto result = coreDRemoteInterface.addDeviceSetting( setting );

        if( result.second ) {
            lua_pushlstring( luaState, result.first.c_str(), result.first.size() );
        }
        else {
            lua_pushnil( luaState );
        }

        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to add device setting : %s", e.what() );
    }
}

int modifySetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto settingId = std::string{ lua_tostring( luaState, 1 ) };

        if( isSettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            auto setting = pullSetting( luaState, 2 );
            coreDRemoteInterface.modifyDeviceSetting( settingId, setting );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to modify setting : %s", e.what() );
    }

    return 0;
}

int removeSetting( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 1 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        coreDRemoteInterface.removeDeviceSetting( settingId );

        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove device setting : %s", e.what() );
    }
}

int updateUserCodeRestriction( lua_State* luaState ) {
    try {
        CHECK_LUA_INPUT_PARAMETERS( 2 );

        const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
        const auto restriction = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateUserCodeRestriction( itemId, restriction );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used. Please, use set_item_value instead of this method" );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to remove device setting : %s", e.what() );
    }

    return 0;
}

int updateItemValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto jsonNewValue = GET_LUA_INPUT_PARAMETERS( Json, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateItemValue( itemId, makeValueLoader( jsonNewValue ) );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used. Please, use set_item_value instead of this method" );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to update value for item (%s) : %s", itemId.c_str(), error.what() );
    }

    return 0;
}

int updateItemValueWithMinMax( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 4 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto newJsonValue = GET_LUA_INPUT_PARAMETERS( Json, 2 );
    const auto newJsonMinValue = GET_LUA_INPUT_PARAMETERS( Json, 3 );
    const auto newJsonMaxValue = GET_LUA_INPUT_PARAMETERS( Json, 4 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateItemValueWithMinMax( itemId,
                                                            makeValueLoader( newJsonValue ),
                                                            makeValueLoader( newJsonMinValue ),
                                                            makeValueLoader( newJsonMaxValue ) );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to update min/max value for item (%s) : %s", itemId.c_str(), error.what() );
    }

    return 0;
}

/* arguments: "dev_id" = string
 *              "reachable" = bool
 * return: none */
int updateReachableState( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto deviceId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto reachable = GET_LUA_INPUT_PARAMETERS( NativeBool, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, deviceId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateReachableState( deviceId, reachable );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
        return 0;
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to update reachable state with device id (%s) : %s", deviceId.c_str(), e.what() );
    }
}

/* arguments: "dev_id" = string
 *              "status" = string
 * return: none */
int updateDeviceStatus( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto deviceId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto status = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, deviceId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateDeviceStatus( deviceId, status );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to update device status with device id (%s) : %s", deviceId.c_str(), e.what() );
    }
}

/* arguments: "dev_id" = string
 *            "ready" = bool
 * return: none */
int updateReadyState( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto deviceId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto ready = GET_LUA_INPUT_PARAMETERS( NativeBool, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        if( isDeviceAndPluginGatewaysSame( coreDRemoteInterface, deviceId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateReadyState( deviceId, ready );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used" );
        }
        return 0;
    }
    catch( const std::exception& e ) {
        return luaL_error(
            luaState, "fail to update ready state with device id (%s) : %s", deviceId.c_str(), e.what() );
    }
}

int sendUIBroadcast( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        auto data = pullJson( luaState, 1 );
        coreDRemoteInterface.sendPluginUIBroadcast( getPluginId( luaState ), data );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to send ui broadcast: %s", e.what() );
    }
    return 0;
}

int generateItemDictionaryNumberId( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto minId = GET_LUA_INPUT_PARAMETERS( NativeInteger, 2 );
    const auto maxId = GET_LUA_INPUT_PARAMETERS( NativeInteger, 3 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        using ID = CoreDRemoteInterface::CoreDRemoteInterface::ItemDictionaryNumberID;
        const auto id = coreDRemoteInterface.generateItemDictionaryNumberId(
            itemId, static_cast< ID >( minId ), static_cast< ID >( maxId ) );
        lua_pushinteger( luaState, static_cast< NativeInteger >( id ) );
        return 1;
    }
    catch( const CoreD::Model::ItemDictionaryIdGenerationError& error ) {
        LOGGER_WARN( error.what() );
        lua_pushnil( luaState );
        return 1;
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to generate item dictionary item id : %s", e.what() );
    }
}

/* arguments: "itemId" = string
 *            "itemDictionaryId" = string
 *            "jsonValue" = Json return: none */
int updateItemDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto itemDictionaryId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    const auto jsonValue = GET_LUA_INPUT_PARAMETERS( Json, 3 );

    using namespace CoreD::Model::Resources;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateItemDictionaryValue( itemId, itemDictionaryId, makeValueLoader( jsonValue ) );
        }
        else {
            return luaL_error( luaState,
                               "wrong gateway is used. Please, use set_item_dictionary_value or "
                               "add_item_dictionary_value instead of this method" );
        }
        return 0;
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, "Bus message is too long" );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to update value for item (%s) : %s", itemId.c_str(), error.what() );
    }
}

int removeItemDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto itemDictionaryId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

    using namespace CoreD::Model::Resources;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isItemAndPluginGatewaysSame( coreDRemoteInterface, itemId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateItemDictionaryValue(
                itemId, itemDictionaryId, []( CoreD::Model::ValueType ) { return CoreDRemoteInterface::ValueUPtr(); } );
        }
        else {
            return luaL_error(
                luaState, "wrong gateway is used. Please, use delete_item_dictionary_value instead of this method" );
        }
        return 0;
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, "Bus message is too long" );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to update value for item (%s) : %s", itemId.c_str(), error.what() );
    }
}

Json pullResult( lua_State* luaState, int idx = -1 ) {
    Json result = pullJson( luaState, idx );
    auto text = json_get_str( result, "text" );
    if( !text.empty() ) {
        return JSON( "text" << std::move( text ) );
    }
    return {};
}

Json pullError( lua_State* luaState, int idx = -1 ) {
    Json result = pullJson( luaState, idx );

    if( result == nullptr ) {
        return nullptr;
    }

    if( json_get_int( result, "code", -1 ) == 3740 && json_get_str( result, "data" ) == "custom_error" &&
        !json_get_str( result, "message" ).empty() ) {
        return JSON( "code" << 3740 << "data"
                            << "custom_error"
                            << "message" << json_get_str( result, "message" ) );
    }

    return JSON( "code" << -32603 << "data"
                        << "ezlo.lua.script.error"
                        << "message"
                        << "Script error" );
}

int sendResponse( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );

    const auto operationId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    auto result = pullResult( luaState, 2 );
    auto error = pullError( luaState, 3 );

    std::string service;
    std::string uuid;

    auto delimiter = operationId.find( '|' );
    if( delimiter != std::string::npos ) {
        service = operationId.substr( 0, delimiter );
        uuid = operationId.substr( delimiter + 1 );
    }

    if( service.empty() || uuid.empty() ) {
        return luaL_error( luaState, "operation_id(%s) have invalid format", operationId.c_str() );
    }

    result[ "error" ] = std::move( error );

    getHostService( luaState ).sendAsyncResponse( service, result, uuid );
    return 0;
}

int subscribe( lua_State* luaState ) {
    using namespace CoreD::Resources;

    auto eventsSubscriber =
        createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( GroupsNames::CORED );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to subscribe" );
    }

    try {
        const auto id = eventsSubscriber->subscribe( getPluginEventsSubscriber( luaState, ModuleNames::Core ) );
        lua_pushstring( luaState, id.c_str() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to subscribe: %s", ex.what() );
    }

    return 1;
}

int setSubscriptionFilters( lua_State* luaState ) {
    using namespace CoreD::Resources;

    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto subscriberId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto rules = GET_LUA_INPUT_PARAMETERS( Json, 2 );

    try {
        changeSubscriberEventsFilterRules( luaState, GroupsNames::CORED, subscriberId, rules, ModuleNames::Core );
    }
    catch( const std::exception& ex ) {
        return luaL_error(
            luaState, "fail to set event filter rule for a (%s) subscription: %s", subscriberId.c_str(), ex.what() );
    }

    return 0;
}

int unsubscribe( lua_State* luaState ) {
    using namespace CoreD::Resources;

    auto eventsSubscriber =
        createFunctionalityCreator( luaState ).getPluginEventsSubscribeFunctionality( GroupsNames::CORED );
    if( !eventsSubscriber ) {
        Log::warn( cc::warn( "Attempt to use eventsSubscriber functionality without proper permissions" ) );
        return luaL_error( luaState, "fail to unsubscribe" );
    }

    try {
        eventsSubscriber->unsubscribe( getPluginEventsSubscriber( luaState, ModuleNames::Core ) );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to unsubscribe: %s", ex.what() );
    }

    return 0;
}

int setItemValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 2 );
    LOGGER_DEBUG( "setItemValue" );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto jsonNewValue = GET_LUA_INPUT_PARAMETERS( Json, 2 );
    auto source = LuaToNative< Json >{}( luaState, 3 ).valueOr( Json{} );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        JsonObject data = JSON( "_id" << itemId << "value" << std::move( jsonNewValue ) );
        if( !source.empty() )
            data[ "source" ] = std::move( source );
        coreDRemoteInterface.setItemValue( data, []( const JsonObject& response ) {
            if( isBusLongDataError( response ) ) {
                throw MaxMessageLengthError( "Bus message is longer than the limitation" );
            }
        } );
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to set value for item (%s) : %s", itemId.c_str(), error.what() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to set value for item (%s) : %s", itemId.c_str(), ex.what() );
    }
    return 0;
}

int setItemValueInBatch( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 2 );
    LOGGER_DEBUG( "setItemValueInBatch" );

    const auto itemIds = GET_LUA_INPUT_PARAMETERS( NativeStringVector, 1 );
    const auto jsonNewValue = GET_LUA_INPUT_PARAMETERS( Json, 2 );
    auto source = LuaToNative< Json >{}( luaState, 3 ).valueOr( Json{} );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        JsonObject data = JSON( "ids" << itemIds << "value" << std::move( jsonNewValue ) );
        if( !source.empty() )
            data[ "source" ] = std::move( source );
        coreDRemoteInterface.setItemValue( data, []( const JsonObject& response ) {
            if( isBusLongDataError( response ) ) {
                throw MaxMessageLengthError( "Bus message is longer than the limitation" );
            }
        } );
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to set item value in batch : %s", ex.what() );
    }
    return 0;
}

int setItemDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 3 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto itemDictionaryId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    const auto jsonValue = GET_LUA_INPUT_PARAMETERS( Json, 3 );
    auto source = LuaToNative< Json >{}( luaState, 4 ).valueOr( Json{} );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        auto elementType = getDictionaryElementType( coreDRemoteInterface, itemId );
        auto data = JSON( "_id" << itemId << "key" << itemDictionaryId << "element"
                                << JSON( "type" << elementType << "value" << jsonValue ) );
        if( !source.empty() )
            data[ "source" ] = std::move( source );
        coreDRemoteInterface.setItemDictionaryValue( data, []( const JsonObject& response ) {
            if( isBusLongDataError( response ) ) {
                throw MaxMessageLengthError( "Bus message is longer than the limitation" );
            }
        } );
    }

    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to set dictionary value for item (%s) : %s", itemId.c_str(), error.what() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to set dictionary value for item (%s) : %s", itemId.c_str(), ex.what() );
    }
    return 0;
}

int addItemDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 2 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto jsonValue = GET_LUA_INPUT_PARAMETERS( Json, 2 );
    auto source = LuaToNative< Json >{}( luaState, 3 ).valueOr( Json{} );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        auto elementType = getDictionaryElementType( coreDRemoteInterface, itemId );
        auto data = JSON( "_id" << itemId << "element" << JSON( "type" << elementType << "value" << jsonValue ) );
        if( !source.empty() )
            data[ "source" ] = std::move( source );

        coreDRemoteInterface.addItemDictionaryValue( data, []( const JsonObject& response ) {
            if( isBusLongDataError( response ) ) {
                throw MaxMessageLengthError( "Bus message is longer than the limitation" );
            }
        } );
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to add dictionary value for item (%s) : %s", itemId.c_str(), error.what() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to add dictionary value for item (%s) : %s", itemId.c_str(), ex.what() );
    }
    return 0;
}

int deleteItemDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 2 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto itemDictionaryId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    auto source = LuaToNative< Json >{}( luaState, 3 ).valueOr( Json{} );

    using namespace CoreD::Model::Resources;

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        auto data = JSON( "_id" << itemId << "key" << itemDictionaryId );
        if( !source.empty() )
            data[ "source" ] = std::move( source );

        coreDRemoteInterface.removeItemDictionaryValue( data, []( const JsonObject& response ) {
            if( isBusLongDataError( response ) ) {
                throw MaxMessageLengthError( "Bus message is longer than the limitation" );
            }
        } );
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error(
            luaState, "fail to delete dictionary value for item (%s) : %s", itemId.c_str(), error.what() );
    }
    catch( const std::exception& ex ) {
        return luaL_error( luaState, "fail to delete dictionary value for item (%s) : %s", itemId.c_str(), ex.what() );
    }
    return 0;
}

int getItemDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto itemId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto itemDictionaryId = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

    try {
        using namespace CoreD::Model::Resources;

        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        const auto result = coreDRemoteInterface.getItemDictionaryValue( itemId, itemDictionaryId );

        return pushJsonAsLuaObj( luaState, result );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get item dictionary value: %s", e.what() );
    }
}

int setSettingValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 )

    const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto value = GET_LUA_INPUT_PARAMETERS( Json, 2 );
    const auto status = GET_LUA_INPUT_PARAMETERS( NativeString, 3 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isSettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            coreDRemoteInterface.updateSettingValue(
                settingId, makeValueLoader( value ), CoreD::Model::SynchronizationStatus::fromString( status ) );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used." );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to set value for setting (%s) : %s", settingId.c_str(), error.what() );
    }
    return 0;
}

int setSettingDictionaryValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 )

    const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto key = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    const auto value = GET_LUA_INPUT_PARAMETERS( Json, 3 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isSettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            auto valueLoader = value.empty() ? makeEmptyValueLoader() : makeValueLoader( value );
            coreDRemoteInterface.updateSettingDictionaryValue( settingId, key, valueLoader );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used." );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to set value for configuration (%s) : %s", settingId.c_str(), error.what() );
    }
    return 0;
}

int updateSettingStatus( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 )

    const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto statusStr = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isSettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            const auto status = CoreD::Model::SynchronizationStatus::fromString( statusStr );
            coreDRemoteInterface.updateSettingStatus( settingId, status );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used." );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to set value for setting (%s) : %s", settingId.c_str(), error.what() );
    }
    return 0;
}

int updateSettingDictionaryStatus( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 )

    const auto settingId = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto key = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    const auto statusStr = GET_LUA_INPUT_PARAMETERS( NativeString, 3 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        if( isSettingAndPluginGatewaysSame( coreDRemoteInterface, settingId, getPluginId( luaState ) ) ) {
            const auto status = CoreD::Model::SynchronizationStatus::fromString( statusStr );
            coreDRemoteInterface.updateSettingDictionaryStatus( settingId, key, status );
        }
        else {
            return luaL_error( luaState, "wrong gateway is used." );
        }
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, "Bus message is too long" );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "fail to set value for configuration (%s) : %s", settingId.c_str(), error.what() );
    }
    return 0;
}

int getLocalTime( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 0 );

    const auto format = LuaToNative< NativeString >{}( luaState, 1 ).valueOr( std::string{} );

    auto now = time( nullptr );
    struct tm timeInfo {};
    localtime_r( &now, &timeInfo );

    if( format.empty() ) {
        lua_pushstring( luaState, TimeUtils::timeStructToString( timeInfo ).c_str() );
    }
    else {
        lua_pushstring( luaState, TimeUtils::timeStructToString( timeInfo, format ).c_str() );
    }
    return 1;
}

int getLocation( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        const auto& geoPosition = coreDRemoteInterface.getGeoPosition().first;
        JsonObject location = JSON_EMPTY_OBJ;
        json_add_float( location, "latitude", geoPosition.latitude );
        json_add_float( location, "longitude", geoPosition.longitude );
        pushJsonAsLuaObj( luaState, location );
    }
    catch( const MaxMessageLengthError& ex ) {
        return luaL_error( luaState, busLongErrorMessage().c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "get location error : %s", error.what() );
    }
    return 1;
}

int getTimezone( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        std::string timeZone = coreDRemoteInterface.getHubTimeZone();
        lua_pushstring( luaState, timeZone.c_str() );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "get timezone error : %s", error.what() );
    }
    return 1;
}

int isInsideWeekInterval( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 3 );

    const auto weekdays = GET_LUA_INPUT_PARAMETERS( NativeStringVector, 1 );
    const auto startTime = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );
    const auto endTime = GET_LUA_INPUT_PARAMETERS( NativeString, 3 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        bool result = coreDRemoteInterface.isInsideWeekInterval( weekdays, startTime, endTime );
        lua_pushboolean( luaState, result );
    }
    catch( const std::exception& e ) {
        lua_pushboolean( luaState, false );
    }

    return 1;
}

int getNextWeekEvent( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );

    const auto weekdays = GET_LUA_INPUT_PARAMETERS( NativeStringVector, 1 );
    const auto time = GET_LUA_INPUT_PARAMETERS( NativeString, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        auto result = coreDRemoteInterface.getNextWeekEvent( weekdays, time );
        if( !result.empty() ) {
            lua_pushstring( luaState, result.c_str() );
        }
        else {
            return 0;
        }
    }
    catch( const std::exception& e ) {
        return 0;
    }

    return 1;
}

int calculateTimeWithDelay( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );

    const auto delay = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };
        auto result = coreDRemoteInterface.calculateTimeWithDelay( delay );
        if( !result.empty() ) {
            lua_pushstring( luaState, result.c_str() );
        }
        else {
            return 0;
        }
    }
    catch( const std::exception& e ) {
        return 0;
    }

    return 1;
}

int getHubInfo( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        Uid::RemoteInterface uidRemoteInterface( getHostService( luaState ) );
        auto hubInfo = uidRemoteInterface.getHubInfo();
        JsonObject luaHubInfo = JSON_EMPTY_OBJ;
        luaHubInfo[ "architecture" ] = hubInfo[ "architecture" ];
        luaHubInfo[ "firmware" ] = hubInfo[ "firmware" ];
        luaHubInfo[ "kernel" ] = hubInfo[ "kernel" ];
        luaHubInfo[ "localtime" ] = hubInfo[ "localtime" ];
        luaHubInfo[ "model" ] = hubInfo[ "model" ];
        luaHubInfo[ "serial" ] = hubInfo[ "serial" ];
        luaHubInfo[ "uptime" ] = hubInfo[ "uptime" ];
        luaHubInfo[ "uuid" ] = hubInfo[ "uuid" ];
        JsonObject& locationSrc = hubInfo[ "location" ];
        JsonObject& locationDst = luaHubInfo[ "location" ];
        locationDst[ "latitude" ] = locationSrc[ "latitude" ];
        locationDst[ "longitude" ] = locationSrc[ "longitude" ];
        locationDst[ "state" ] = locationSrc[ "state" ];
        locationDst[ "timezone" ] = locationSrc[ "timezone" ];
        JsonObject& buildSrc = hubInfo[ "build" ];
        JsonObject& buildDst = luaHubInfo[ "build" ];
        buildDst[ "branch" ] = buildSrc[ "branch" ];
        buildDst[ "builder" ] = buildSrc[ "builder" ];
        buildDst[ "commit" ] = buildSrc[ "commit" ];
        buildDst[ "time" ] = buildSrc[ "time" ];
        auto it = hubInfo.find( "battery" );
        if( it != hubInfo.end() ) {
            JsonObject& batterySrc = *it;
            JsonObject& batteryDst = luaHubInfo[ "battery" ];
            batteryDst[ "status" ] = batterySrc[ "status" ];
            batteryDst[ "charge_state" ] = batterySrc[ "stateOfCharge" ];
            batteryDst[ "remaining_time" ] = batterySrc[ "remainingTime" ];
            batteryDst[ "health" ] = batterySrc[ "health" ];
        }
        pushJsonAsLuaObj( luaState, luaHubInfo );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState, "get timezone error : %s", error.what() );
    }
    return 1;
}

int sendDeviceFirmwareUpdateBroadcast( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 )

    const auto params = GET_LUA_INPUT_PARAMETERS( Json, 1 );
    if( params.count( "device_id" ) == 0 ) {
        return luaL_error( luaState, "field 'device_id' not found" );
    }
    if( params.count( "firmware_id" ) == 0 ) {
        return luaL_error( luaState, "field 'firmware_id' not found" );
    }
    if( params.count( "status" ) == 0 ) {
        return luaL_error( luaState, "field 'status' not found" );
    }

    try {
        auto& service = dynamic_cast< Services::Service& >( getHostService( luaState ) );
        service.sendBroadcast( HUB_DEVICE_FIRMWARE_UPDATE_PROGRESS, params );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to send device firmware update broadcast: %s", e.what() );
    }
    return 0;
}

int sendDeviceFirmwareDownloadBroadcast( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 )

    const auto params = GET_LUA_INPUT_PARAMETERS( Json, 1 );
    if( params.count( "image" ) == 0 ) {
        return luaL_error( luaState, "field 'image' not found" );
    }
    if( params.count( "status" ) == 0 ) {
        return luaL_error( luaState, "field 'status' not found" );
    }

    try {
        auto& service = dynamic_cast< Services::Service& >( getHostService( luaState ) );
        service.sendBroadcast( HUB_DEVICE_FIRMWARE_DOWNLOAD_PROGRESS, params );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to send device firmware download broadcast: %s", e.what() );
    }
    return 0;
}

int sendDeviceCheckedBroadcast( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 )

    auto params = GET_LUA_INPUT_PARAMETERS( Json, 1 );
    if( params.count( "device_id" ) == 0 ) {
        return luaL_error( luaState, "field 'device_id' not found" );
    }
    params[ "deviceId" ] = params[ "device_id" ];
    json_remove( params, "device_id" );
    if( params.count( "status" ) == 0 ) {
        return luaL_error( luaState, "field 'status' not found" );
    }
    try {
        auto& service = dynamic_cast< Services::Service& >( getHostService( luaState ) );
        service.sendBroadcast( HUB_DEVICE_CHECKED_PROGRESS, params );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to send device check broadcast broadcast: %s", e.what() );
    }
    return 0;
}

int notifyProcessStarted( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    const auto processString = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );
        std::string gatewayId = fetchGatewayId( luaState );

        const auto process = CoreD::Model::gatewayProcessFromString( processString );
        if( process == CoreD::Model::GatewayProcess::Unknown ) {
            return luaL_error( luaState, "Parameter 'process' have invalid value" );
        }

        coreDRemoteInterface.startGatewayProcess( gatewayId, process );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "failed to stop process: %s", e.what() );
    }

    return 0;
}

int notifyProcessStopped( lua_State* luaState ) {
    CHECK_LUA_INPUT_OPTIONAL_PARAMETERS( 1 );
    const auto processString = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    const auto resultString = LuaToNative< NativeString >{}( luaState, 2 ).valueOr( std::string() );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );
        std::string gatewayId = fetchGatewayId( luaState );

        const auto process = CoreD::Model::gatewayProcessFromString( processString );
        const auto result = CoreD::Model::gatewayProcessResultFromString( resultString );
        if( process == CoreD::Model::GatewayProcess::Unknown ) {
            return luaL_error( luaState, "Parameter 'process' have invalid value" );
        }

        coreDRemoteInterface.stopGatewayProcess( gatewayId, process, result );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "failed to stop process: %s", e.what() );
    }

    return 0;
}

int getCurrentNotifiedProcess( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        std::string gatewayId = fetchGatewayId( luaState );

        auto process = coreDRemoteInterface.getGatewayCurrentProcess( gatewayId );
        if( process != CoreD::Model::GatewayProcess::Unknown ) {
            lua_pushstring( luaState, CoreD::Model::toString( process ).c_str() );
            return 1;
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to add device : %s", e.what() );
    }

    return 0;
}

int addVeraDeviceMapping( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 )

    const auto veraDeviceId = GET_LUA_INPUT_PARAMETERS( NativeInteger, 1 );
    const auto deviceData = GET_LUA_INPUT_PARAMETERS( Json, 2 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface{ getHostService( luaState ) };

        coreDRemoteInterface.addVeraDeviceMapping( veraDeviceId, deviceData );
    }
    catch( const CoreD::Model::CoreDModelError& error ) {
        return luaL_error( luaState,
                           "fail to add vera device mapping, vera_device_id: %s, device_data: , %s",
                           veraDeviceId,
                           deviceData.dump().c_str(),
                           error.what() );
    }
    return 0;
}

/* arguments: no
 * return: rooms (table) or nil if failed to get rooms
 *          { [0] = { room }, [1] = {},... [n - 1] = {}  } }
 *
 *          room = { [ "id" ] = string,
 *                     [ "name" ] = string } */
int getRooms( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 0 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto rooms = coreDRemoteInterface.getAllRooms();

        if( rooms.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            pushJsonAsLuaObj( luaState, rooms, createMappingKeyFormatter( RoomDbToLuaMap, camelToSnakeCase ), 1 );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get rooms : %s", e.what() );
    }
    return 1;
}

/* arguments: id - identifier of a room
 * return: room (table) or nil if failed to get rooms
 *          { room }
 *
 *          room = { [ "id" ] = string, [ "name" ] = string }
 */
int getRoom( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    std::string id = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto room = coreDRemoteInterface.getRoom( id );

        if( room.empty() ) {
            lua_pushnil( luaState );
        }
        else {
            pushJsonAsLuaObj( luaState, room, createMappingKeyFormatter( RoomDbToLuaMap, camelToSnakeCase ), 1 );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to get room : %s", e.what() );
    }
    return 1;
}

int testItemValue( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 2 );
    const auto type = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );
    auto value = pullJson( luaState, 2 );

    using namespace CoreD::Model;
    try {
        auto error = CoreD::Model::ItemChecker::itemValueTest( type, value );

        if( !error.empty() ) {
            return luaL_error( luaState, "%s", error.c_str() );
        }
        else {
            lua_pushboolean( luaState, true );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to test item value : %s", e.what() );
    }

    return 1;
}

int isFirmwareVersionLess( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    const auto targetVersion = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    using namespace CoreD::Model;
    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );
        auto flag = coreDRemoteInterface.isFirmwareVersionLess( targetVersion );
        lua_pushboolean( luaState, flag );
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "fail to compare firmware version : %s", e.what() );
    }

    return 1;
}

int getFeatureStatus( lua_State* luaState ) {
    CHECK_LUA_INPUT_PARAMETERS( 1 );
    const auto featureName = GET_LUA_INPUT_PARAMETERS( NativeString, 1 );

    try {
        CoreDRemoteInterface::CoreDRemoteInterface coreDRemoteInterface( getHostService( luaState ) );

        const auto feature = coreDRemoteInterface.findFeature( featureName );
        if( feature ) {
            auto featureStatus = feature->getStatus();
            std::string featureStatusStr = featureStatus ? "on" : "off";
            lua_pushstring( luaState, featureStatusStr.c_str() );
        }
        else {
            lua_pushnil( luaState );
        }
    }
    catch( const std::exception& e ) {
        return luaL_error( luaState, "Fail to find feature : %s", e.what() );
    }

    return 1;
}

const char moduleName[] = "core";
const luaL_Reg module[] = { { "get_gateways", getGateways },
                            { "get_gateway", getGateway },
                            { "update_gateway", updateGateway },
                            { "add_gateway_setting", addGatewaySetting },
                            { "remove_gateway_setting", removeGatewaySetting },
                            { "set_gateway_setting_value", setGatewaySettingValue },
                            { "set_gateway_setting_status", setGatewaySettingStatus },
                            { "get_gateway_setting_ids_by_gateway_id", getGatewaySettingIdsByGatewayId },
                            { "get_gateway_setting", getGatewaySetting },
                            { "add_device", addDevice },
                            { "modify_device", modifyDevice },
                            { "remove_device", removeDevice },
                            { "remove_device_sync", removeDeviceSync },
                            { "remove_gateway_devices", removeGatewayDevices },
                            { "get_devices", getDevices },
                            { "get_devices_ids", getDevicesIds },
                            { "get_device", getDevice },
                            { "get_root_device_id", getRootDeviceId },
                            { "add_item", addItem },
                            { "modify_item", modifyItem },
                            { "remove_item", removeItem },
                            { "get_items", getItems },
                            { "get_items_by_device_id", getItemsByDeviceId },
                            { "get_item", getItem },
                            { "get_setting_ids_by_device_id", getSettingIdsByDeviceId },
                            { "get_setting", getSetting },
                            { "add_setting", addSetting },
                            { "modify_setting", modifySetting },
                            { "remove_setting", removeSetting },
                            { "update_user_code_restriction", updateUserCodeRestriction },
                            { "update_item_value", updateItemValue },
                            { "update_item_value_with_min_max", updateItemValueWithMinMax },
                            { "update_reachable_state", updateReachableState },
                            { "update_device_status", updateDeviceStatus },
                            { "update_ready_state", updateReadyState },
                            { "send_ui_broadcast", sendUIBroadcast },
                            { "generate_item_dictionary_number_id", generateItemDictionaryNumberId },
                            { "update_item_dictionary_value", updateItemDictionaryValue },
                            { "remove_item_dictionary_value", removeItemDictionaryValue },
                            { "send_response", sendResponse },
                            { "subscribe", subscribe },
                            { "set_subscription_filters", setSubscriptionFilters },
                            { "unsubscribe", unsubscribe },
                            { "set_item_value", setItemValue },
                            { "set_item_value_in_batch", errorCatchWrapper< setItemValueInBatch > },
                            { "set_item_dictionary_value", setItemDictionaryValue },
                            { "add_item_dictionary_value", addItemDictionaryValue },
                            { "get_item_dictionary_value", getItemDictionaryValue },
                            { "delete_item_dictionary_value", deleteItemDictionaryValue },
                            { "set_setting_value", setSettingValue },
                            { "set_setting_dictionary_value", setSettingDictionaryValue },
                            { "update_setting_status", updateSettingStatus },
                            { "update_setting_dictionary_status", updateSettingDictionaryStatus },
                            { "get_local_time", getLocalTime },
                            { "get_location", getLocation },
                            { "get_timezone", getTimezone },
                            { "is_inside_week_interval", isInsideWeekInterval },
                            { "get_next_week_event", getNextWeekEvent },
                            { "calculate_time_with_delay", calculateTimeWithDelay },
                            { "get_hub_info", getHubInfo },
                            { "send_device_firmware_update_broadcast", sendDeviceFirmwareUpdateBroadcast },
                            { "send_device_firmware_download_broadcast", sendDeviceFirmwareDownloadBroadcast },
                            { "send_device_checked_broadcast", sendDeviceCheckedBroadcast },
                            { "notify_process_started", notifyProcessStarted },
                            { "notify_process_stopped", notifyProcessStopped },
                            { "get_current_notified_process", getCurrentNotifiedProcess },
                            { "add_vera_device_mapping", addVeraDeviceMapping },
                            { "get_rooms", errorCatchWrapper< getRooms > },
                            { "get_room", errorCatchWrapper< getRoom > },
                            { "test_item_value", errorCatchWrapper< testItemValue > },
                            { "is_firmware_version_less", isFirmwareVersionLess },
                            { "get_feature_status", errorCatchWrapper< getFeatureStatus > },
                            { nullptr, nullptr } };

}  // namespace
}  // namespace HubPlatform

HUB_MODULE_ENTRY_DEFINITION( HubPlatform::module, HubPlatform::moduleName );
