#include "cored/remote_interface/cored_remote_interface.h"
#include "cored/model/values/values.h"
#include "cored/model/values/value_factory.h"
#include "cored/model/configuration/configuration_impl.h"
#include "cored/model/exceptions.h"
#include "cored/model/cored_device_setting.h"
#include "cored/house_modes/house_mode_utils.h"
#include "cored/house_modes/protect_button.h"
#include "cored/model/resources/cored_resources.h"
#include "hub_settings/hub_settings_reader.h"
#include "cored/model/cored_gateway_processes_utils.h"
#include "cored/exceptions.h"
#include "cored/what_happened.h"
#include "cored/group_operations/remote_interface/group_operation_interface.h"
#include "cored/policies/remote_interface/policies_operation_interface.h"
#include "cored/swinger_shutdown/remote_interface/swinger_shutdown_interface.h"

#include "service_interfaces/service.h"
#include "database_client/db_tables_names.h"
#include "system_utils/environment.h"
#include "json_utils/json_wrapper.h"
#include "jsonrpc/jsonrpc.h"
#include "bus/messagesclasses.h"
#include "log_utils/ezlo_log.h"

#include "jsonrpc/message.h"
#include "base_utils/utils.h"
#include "base_utils/unit_converter.h"
#include "cored/core_service.h"
#include "cored/model/resources/cored_resources.h"
#include "cored/model/values/scalable_value.h"

#include "base_utils/ezlodb.h"
#include "base_utils/acl.h"
#include "system_utils/filesystem.h"
#include "system_utils/timezone/debian_timezone.h"
#include "system_utils/timezone/openwrt_timezone.h"
#include "system_utils/timezone/timezone_utils.h"
#include "provision_data_utils/provision_data_manager.h"

#include "config_reader/config_keys.h"
#include "database_client/dbqueryparser.h"
#include "database_client/idbclient.h"

#include "uid/field_names.h"

#include "common_utils/cpp14.h"
#include "base_utils/timeutils.h"
#include "bus/base_bus.h"

#include "cored/model/values/value_visitor.h"
#include "cored/model/values/float_value.h"
#include "cored/model/visitors/item_value_float_visitor.h"
#include "cored/model/visitors/item_value_percent_apply_visitor.h"
#include "hub_settings/hub_setting.h"
#include "hub_settings/hub_settings_reader.h"
#include "common_utils/firmware_info_helper.h"

#include "cored/model/cored_policy.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

namespace HubPlatform {
namespace CoreDRemoteInterface {

namespace {
const auto ValueSetCommandTimeout = std::chrono::minutes( 2 );
constexpr uint32_t DictionaryKeyLengthLimit = 16;
constexpr uint32_t DeviceSettingNameLengthLimit = 128;
constexpr uint32_t DeviceSettingDescriptionLengthLimit = 1024;
}  // namespace

using namespace HubPlatform::CoreD;

CoreDRemoteInterface::CoreDRemoteInterface( Services::Service& hostService,
                                            const ProvisionDataUtils::ProvisionData& provisionData )
: m_hostService( hostService )
, m_deviceAddingWizard( new DeviceAddingWizard( Environment::getConfigsFolder() + "/" + "DeviceAddingWizard.json" ) )
, m_provisionData( provisionData )
, m_localTimeGetter( []() {
    std::time_t current_time;
    std::time( &current_time );
    struct std::tm* timeinfo = std::localtime( &current_time );

    return GregorianCalendar::Date( std::chrono::system_clock::now(), std::chrono::seconds( timeinfo->tm_gmtoff ) );
} ) {}

CoreDRemoteInterface::CoreDRemoteInterface( Services::Service& hostService,
                                            LocalTimeGetter localTimeGetter,
                                            const ProvisionDataUtils::ProvisionData& provisionData )
: m_hostService( hostService )
, m_deviceAddingWizard( new DeviceAddingWizard( Environment::getConfigsFolder() + "/" + "DeviceAddingWizard.json" ) )
, m_provisionData( provisionData )
, m_localTimeGetter( std::move( localTimeGetter ) ) {}

CoreDRemoteInterface::~CoreDRemoteInterface() = default;

Model::Configuration& CoreDRemoteInterface::getConfiguration() {
    if( !m_configuration )
        m_configuration = std::make_unique< Model::ConfigurationImpl >( getDBClient() );

    return *m_configuration;
}

CoreDRemoteInterface::GatewayEnsureResult CoreDRemoteInterface::ensureGateway( const Model::Gateway& gateway ) {
    auto jsonGate = gateway.toJson();

    try {
        Model::Gateway::checkGatewayAsJson( jsonGate );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::ensureGateway: " } + exception.what() );
        return { {}, false };
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::ensureGateway: bad gateway's parameters: " + jsonGate.dump() );
        return { {}, false };
    }

    auto response = m_hostService.sendRequest( GroupsNames::CORED, jsonGate, MessageSubClasses::ENSURE_GATEWAY );
    if( hasError( response ) ) {
        Log::error( "CoreDRemoteInterface::ensureGateway::sendRequest: " + getErrorMessage( response ) );
        return { {}, false };
    }

    DeviceID gatewayID;
    if( !json_get( response[ "id" ], gatewayID ) ) {
        Log::error( "CoreDRemoteInterface::ensureGateway: bad cored's response" );
        return { {}, false };
    }

    return { gatewayID, true };
}

void CoreDRemoteInterface::updateGateway( const GatewayID& gatewayId, const Json& gateway ) {
    auto params =
        JSON( Model::Resources::RPCParams::GatewayId << gatewayId << Model::Resources::RPCParams::Gateway << gateway );

    m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODIFY_GATEWAY );
}

CoreDRemoteInterface::RawGateways CoreDRemoteInterface::getGateways() {
    /* TODO: perform a permission check
     * JsonObject projections = JSON( "private" << false );
       return JSON( "gateways" << Db::findAll( Model::Resources::DBNames::GatewaysDB, JSON_EMPTY_OBJ, &projections ) );
     * */
    auto gateways = getDBClient().findAll( Model::Resources::DBNames::GatewaysDB, JSON_EMPTY_OBJ, nullptr );

    try {
        for( const auto& gate : gateways ) {
            Model::Gateway::checkGatewayAsJson( gate );
        }
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::getGateways: " } + exception.what() );
        return {};
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getGateways: gateways' entries in the db are corrupted" );
        return {};
    }

    return gateways;
}

CoreDRemoteInterface::GatewaysGetResult CoreDRemoteInterface::getGateways( const rpc::Message& /*request*/ ) {
    auto gateways = getGateways();
    if( gateways.empty() ) {
        return { JSON_EMPTY_OBJ, false };
    }
    for( auto& gate : gateways ) {
        json_remove( gate, Model::Resources::GatewayFields::SetItemValueResponsePolicyFieldName );
    }

    return { JSON( "gateways" << std::move( gateways ) ), true };
}

Json CoreDRemoteInterface::fetchGateway( const Json& gatewayDbQuery ) {
    auto gateway = getDBClient().findOne( Model::Resources::DBNames::GatewaysDB, gatewayDbQuery, nullptr );
    if( gateway.empty() ) {
        Log::error( "CoreDRemoteInterface::fetchGateway: fail to get a gateway registered for the gatewayDbQuery: " +
                    cc::j( gatewayDbQuery ) );
        return JSON_EMPTY_OBJ;
    }

    try {
        Model::Gateway::checkGatewayAsJson( gateway );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::fetchGateway: " } + exception.what() );
        return JSON_EMPTY_OBJ;
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::fetchGateway: gateway's entry in the db is corrupted" );
        return JSON_EMPTY_OBJ;
    }

    return gateway;
}

Json CoreDRemoteInterface::getGatewayByPluginId( const PluginId& pluginId ) {
    const auto query = JSON( Model::Resources::GatewayFields::PluginIdFieldName << pluginId );
    return fetchGateway( query );
}

Json CoreDRemoteInterface::getGatewayById( const GatewayID& gatewayId ) {
    const auto query = JSON( Model::Resources::GatewayFields::IdFieldName << gatewayId );
    return fetchGateway( query );
}

CoreDRemoteInterface::GatewaySettingAddResult CoreDRemoteInterface::addGatewaySetting(
    const CoreD::Model::GatewaySetting& setting ) {
    try {
        auto response =
            m_hostService.sendRequest( GroupsNames::CORED, setting.toJson(), MessageSubClasses::ADD_GATEWAY_SETTING );

        GatewaySettingId settingId;
        if( !json_get( response[ "id" ], settingId ) ) {
            LOGGER_ERROR( cc::error( "failed to add a gateway setting: " ) + cc::j( setting.toJson() ) +
                          cc::error( ", bad cored's response" ) );
            return { {}, false };
        }

        return { settingId, true };
    }
    catch( const std::exception& e ) {
        LOGGER_ERROR( cc::error( "failed to add a gateway setting: " ) + cc::j( setting.toJson() ) + cc::error( ", " ) +
                      cc::note( e.what() ) );
        return { {}, false };
    }
}

void CoreDRemoteInterface::removeGatewaySetting( const GatewaySettingId& settingId ) {
    sendMethodCall( REMOVE_GATEWAY_SETTING, JSON( Model::Resources::RPCParams::SettingId << settingId ) );
}

Json CoreDRemoteInterface::getGatewaySetting( const GatewaySettingId& gatewaySettingId ) {
    const auto query = JSON( Model::Resources::GatewaySettingFields::Id << gatewaySettingId );
    return getDBClient().findOne( Model::Resources::DBNames::GatewaysSettingsDB, query, nullptr );
}

CoreDRemoteInterface::RawGatewaysSettings CoreDRemoteInterface::getGatewaySettings( const GatewayID& gatewayId ) {
    const auto query = JSON( Model::Resources::GatewaySettingFields::GatewayId << gatewayId );
    return getDBClient().findAll( Model::Resources::DBNames::GatewaysSettingsDB, query, nullptr );
}

CoreDRemoteInterface::RawGatewaysSettings CoreDRemoteInterface::getGatewaysSettings() {
    return getDBClient().findAll( Model::Resources::DBNames::GatewaysSettingsDB, JSON_EMPTY_OBJ, nullptr );
}

bool CoreDRemoteInterface::isGatewaySettingReadOnly( const GatewaySettingId& gatewaySettingId ) {
    const auto gatewaySetting = getGatewaySetting( gatewaySettingId );
    if( !gatewaySetting.empty() &&
        json_get_bool( gatewaySetting, Model::Resources::GatewaySettingFields::HasSetter, true ) ) {
        return false;
    }

    return true;
}

void CoreDRemoteInterface::setGatewayStatus( const PluginId& pluginId, CoreD::Model::GatewayStatus status ) {
    const auto query = JSON( Model::Resources::GatewayFields::PluginIdFieldName << pluginId );

    auto gateway = getDBClient().findOne( Model::Resources::DBNames::GatewaysDB, query, nullptr );
    if( gateway.empty() ) {
        Log::error( "CoreDRemoteInterface::setGatewayStatus: fail to get a gateway registered for the plugin: " +
                    pluginId );
        return;
    }

    updateGateway( gateway[ db::ID_FIELD_NAME ],
                   JSON( Model::Resources::RPCParams::GatewayStatus << toString( status ) ) );
}

CoreDRemoteInterface::DeviceAddResult CoreDRemoteInterface::addDevice( const Model::Device& device ) {
    auto jsonDev = device.toJson();

    try {
        Model::Device::checkDeviceAsJson( jsonDev );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::addDevice: " } + exception.what() +
                    ", device: " + cc::j( jsonDev ) );
        return { {}, false };
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::addDevice: bad device's parameters: " + jsonDev.dump() );
        return { {}, false };
    }

    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, jsonDev, MessageSubClasses::ADD_DEVICE, m_busTimeout );

    DeviceID deviceID;
    if( !json_get( response[ "id" ], deviceID ) ) {
        Log::error( "CoreDRemoteInterface::addDevice: bad cored's response" );
        return { {}, false };
    }

    return { deviceID, true };
}

void CoreDRemoteInterface::modifyDevice( const DeviceID& deviceId, const CoreD::Model::Device& device ) {
    auto jsonDev = device.toJson();

    try {
        Model::Device::checkDeviceAsJson( jsonDev );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::modifyDevice: " } + exception.what() );
        return;
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::modifyDevice: bad device's parameters: " + jsonDev.dump() );
        return;
    }

    auto params =
        JSON( Model::Resources::RPCParams::DeviceId << deviceId << Model::Resources::RPCParams::Device << jsonDev );

    m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODIFY_DEVICE, m_busTimeout );
}

CoreDRemoteInterface::RawDevices CoreDRemoteInterface::getDevices() const {
    /* TODO: perform a permission check
     * getField( rpcMessage.getParams(), "sender", sender )
     *
     *  const auto allDevices = Permission::check( rpcMessage.getParams(), Resource::DEVICES, Permission::EDIT ) ?
     *                           Fo::getDevices() :
     *                           Fo::getDevices( JSON_EMPTY_OBJ, sender );
     */
    try {
        auto devices = findDevices( JSON_EMPTY_OBJ );
        for( const auto& dev : devices )
            Model::Device::checkDeviceAsJson( dev, true );

        return devices;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::getDevices: " } + exception.what() );
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getDevices: devices' entries in the db are corrupted" );
    }
    return {};
}

CoreDRemoteInterface::RawDevices CoreDRemoteInterface::getDevicesBy( const JsonObject& condition ) {
    try {
        auto devices = findDevices( condition );
        for( const auto& dev : devices )
            Model::Device::checkDeviceAsJson( dev );

        return devices;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string( "CoreDRemoteInterface::getDevicesBy(" ) + condition.dump() +
                    "), exception :" + std::string( exception.what() ) );
        return {};
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getDevicesBy(" + condition.dump() + "), unknown exception" );
        return {};
    }
}

JsonObject CoreDRemoteInterface::getSingleDevice( const DeviceID& deviceId ) {
    try {
        auto device = findDevice( JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ) );
        Model::Device::checkDeviceAsJson( device );
        return device;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string( "CoreDRemoteInterface::getSingleDevice(" ) + deviceId +
                    "), exception : " + std::string( exception.what() ) );
        return JSON_EMPTY_OBJ;
    }
    catch( ... ) {
        Log::error( std::string( "CoreDRemoteInterface::getSingleDevice(" ) + deviceId + "), unknown exception" );
        return JSON_EMPTY_OBJ;
    }
}

CoreDInterface::DeviceID CoreDRemoteInterface::getRootDeviceId( const DeviceID& deviceId ) {
    auto device = getSingleDevice( deviceId );
    if( device.empty() )
        return {};

    while( !device[ Model::Resources::DeviceFields::ParentDeviceIdFieldName ].get< std::string >().empty() ) {
        device = getSingleDevice( device[ Model::Resources::DeviceFields::ParentDeviceIdFieldName ] );
    }

    return device[ Model::Resources::DeviceFields::IdFieldName ];
}

bool isBusLongDataError( const JsonObject& response ) {
    auto error = response[ "error" ];
    if( !error.empty() && json_get_str( error, "data" ) == "bus.message.too.long" )
        return true;
    return false;
}

CoreDRemoteInterface::ItemAddResult CoreDRemoteInterface::addItem( const Model::Item& item ) {
    auto jsonItem = item.toJson();

    try {
        Model::Item::checkItemAsJson( jsonItem );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::addItem: " } + exception.what() );
        return { {}, false };
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::addItem: bad item's parameters: " + jsonItem.dump() );
        return { {}, false };
    }

    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, jsonItem, MessageSubClasses::ADD_ITEM, m_busTimeout );

    if( hasError( response ) ) {
        auto& error = response[ "error" ];
        Log::error( "CoreDRemoteInterface::addItem error: " + json_get_str( error, "message" ) +
                    ", reason: " + json_get_str( error, "reason" ) );

        if( isBusLongDataError( response ) ) {
            throw MaxMessageLengthError( "Bus message is too long" );
        }

        return { {}, false };
    }

    ItemID itemID;
    if( !json_get( response[ "id" ], itemID ) ) {
        Log::error( "CoreDRemoteInterface::addItem: bad cored's response" );
        return { {}, false };
    }

    return { itemID, true };
}

bool CoreDRemoteInterface::modifyItem( const ItemID& itemId, const CoreD::Model::Item& item ) {
    auto jsonItem = item.toJson();

    try {
        Model::Item::checkItemAsJson( jsonItem );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::modifyItem: " } + exception.what() );
        return false;
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::modifyItem: bad item's parameters: " + jsonItem.dump() );
        return false;
    }

    auto params =
        JSON( Model::Resources::RPCParams::ItemId << itemId << Model::Resources::RPCParams::Item << jsonItem );

    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODIFY_ITEM, m_busTimeout );

    if( hasError( response ) ) {
        if( isBusLongDataError( response ) ) {
            throw MaxMessageLengthError( "Bus message is too long" );
        }

        auto& error = response[ "error" ];
        Log::error( "CoreDRemoteInterface::modifyItem error: " + json_get_str( error, "message" ) +
                    ", reason: " + json_get_str( error, "reason" ) );
        return false;
    }

    return true;
}

CoreDRemoteInterface::DeviceSettingAddResult CoreDRemoteInterface::addDeviceSetting(
    const CoreD::Model::DeviceSetting& setting ) {
    try {
        auto response =
            m_hostService.sendRequest( GroupsNames::CORED, setting.toJson(), MessageSubClasses::ADD_DEVICE_SETTING );

        DeviceSettingId settingId;
        if( !json_get( response[ "id" ], settingId ) ) {
            Log::error( "CoreDRemoteInterface::addDeviceSetting: bad cored's response" );
            return { {}, false };
        }

        return { settingId, true };
    }
    catch( const std::exception& e ) {
        Log::error( std::string{ "CoreDRemoteInterface::addDeviceSetting: " } + e.what() );
        return { {}, false };
    }
}

bool CoreDRemoteInterface::modifyDeviceSetting( const DeviceSettingId& settingId,
                                                const CoreD::Model::DeviceSetting& setting ) {
    auto jsonSetting = setting.toJson();

    auto params = JSON( Model::Resources::RPCParams::SettingId << settingId << Model::Resources::RPCParams::Setting
                                                               << jsonSetting );

    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::MODIFY_DEVICE_SETTING );

    if( hasError( response ) ) {
        auto& error = response[ "error" ];
        Log::error( "CoreDRemoteInterface::modifyDeviceSetting error: " + json_get_str( error, "message" ) +
                    ", reason: " + json_get_str( error, "reason" ) );
        return false;
    }

    return true;
}

JsonObject CoreDRemoteInterface::getItem( const ItemID& itemId ) const {
    try {
        auto joItem = findItem( JSON( Model::Resources::ItemFields::IdFieldName << itemId ) );
        Model::Item::checkItemAsJson( joItem );
        Model::Item::decryptItemAsJson( m_provisionData, joItem );
        return joItem;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::getItem: " } + exception.what() + ", itemId: " + itemId );
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getItem: bad item's entry in the db is corrupted, itemId: " + itemId );
    }

    return JSON_EMPTY_OBJ;
}

CoreDRemoteInterface::RawItems CoreDRemoteInterface::getItems() {
    /* TODO: perform a permission check
     * getField( rpcMessage.getParams(), "sender", sender );
     * const auto items = Permission::check( rpcMessage.getParams(), Resource::DEVICES, Permission::EDIT ) ?
                               Fo::getItems() :
                               Fo::getItems( JSON_EMPTY_OBJ, sender );
                                */
    std::string latestId;
    try {
        auto items = findItems( JSON_EMPTY_OBJ );
        for( auto& item : items ) {
            latestId = json_get_str( item, CoreD::Model::Resources::ItemFields::IdFieldName );
            Model::Item::checkItemAsJson( item );
            Model::Item::decryptItemAsJson( m_provisionData, item );
        }

        return items;
    }
    catch( const std::exception& exception ) {
        Log::error( cc::error( "CoreDRemoteInterface::getItems: " ) + cc::note( exception.what() ) +
                    ", itemId: " + latestId );
    }
    catch( ... ) {
        Log::error( cc::error( "CoreDRemoteInterface::getItems: items' entries in the db are corrupted" ) +
                    ", itemId: " + latestId );
    }

    return {};
}

CoreDRemoteInterface::RawItems CoreDRemoteInterface::getItemsBy( const JsonObject& condition ) {
    std::string latestId;
    try {
        auto items = findItems( condition );
        for( auto& item : items ) {
            latestId = json_get_str( item, CoreD::Model::Resources::ItemFields::IdFieldName );
            Model::Item::checkItemAsJson( item );
            Model::Item::decryptItemAsJson( m_provisionData, item );
        }

        return items;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string( "CoreDRemoteInterface::getItems(" ) + condition.dump() +
                    std::string( "), exception :" ) + std::string( exception.what() ) + ", itemId: " + latestId );
    }
    catch( ... ) {
        Log::error( std::string( "CoreDRemoteInterface::getItems(" ) + condition.dump() +
                    std::string( ") unknown error" ) + ", itemId: " + latestId );
    }
    return {};
}

CoreDRemoteInterface::DeviceSettings CoreDRemoteInterface::getDeviceSettings() const {
    return getDeviceSettingsBy( JSON_EMPTY_OBJ );
}

CoreDRemoteInterface::DeviceSettings CoreDRemoteInterface::getDeviceSettingsBy( const JsonObject& condition ) const {
    try {
        DeviceSettings result;

        auto settings = getDBClient().findAll( Model::Resources::DBNames::DevicesSettingsDB, condition, nullptr );

        for( const auto& setting : settings ) {
            result.emplace( json_get_str( setting, Model::Resources::DeviceSettingFields::Id ),
                            Model::DeviceSetting::fromJson( setting ) );
        }

        return result;
    }
    catch( const std::exception& exception ) {
        Log::error( cc::error( "CoreDRemoteInterface::getDeviceSettingsBy: " ) + cc::error( exception.what() ) );
        return {};
    }
    catch( ... ) {
        Log::error( cc::error(
            "CoreDRemoteInterface::getDeviceSettingsBy: device_settings' entries in the db are corrupted" ) );
        return {};
    }
}

CoreDRemoteInterface::RawItems CoreDRemoteInterface::getItemsByDeviceId( const DeviceID& deviceId ) const {
    std::string latestId;
    try {
        auto items = findItems( JSON( Model::Resources::ItemFields::DeviceIdFieldName << deviceId ) );
        for( auto& item : items ) {
            latestId = json_get_str( item, CoreD::Model::Resources::ItemFields::IdFieldName );
            Model::Item::checkItemAsJson( item );
            Model::Item::decryptItemAsJson( m_provisionData, item );
        }

        return items;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::getItemsByDeviceId: " } + exception.what() +
                    ", itemId: " + latestId );
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getItemsByDeviceId: items' entries in the db are corrupted, itemId: " +
                    latestId );
    }

    return {};
}

CoreDRemoteInterface::SettingIDs CoreDRemoteInterface::getDeviceSettingIdsByDeviceId( const DeviceID& deviceId ) const {
    try {
        const auto query = JSON( Model::Resources::DeviceSettingFields::DeviceId << deviceId );

        auto settings = getDBClient().findAll( Model::Resources::DBNames::DevicesSettingsDB, query, nullptr );

        SettingIDs settingIds;
        for( auto& setting : settings ) {
            settingIds.push_back( setting[ Model::Resources::DeviceSettingFields::Id ] );
        }

        return settingIds;
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::getDeviceSettingIdsByDeviceId: " } + exception.what() );
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getDeviceSettingIdsByDeviceId: devices' entries in the db are corrupted" );
    }

    return {};
}

CoreD::Model::DeviceSetting CoreDRemoteInterface::getRawDeviceSetting( const DeviceSettingId& settingId ) const {
    const auto joSetting = getDeviceSetting( settingId );

    if( joSetting.empty() )
        throw DeviceSettingNotFoundError{ settingId };

    return CoreD::Model::DeviceSetting::fromJson( joSetting );
}

JsonObject CoreDRemoteInterface::getDeviceSetting( const DeviceSettingId& settingId ) const {
    try {
        const auto query = JSON( Model::Resources::DeviceSettingFields::Id << settingId );
        return getDBClient().findOne( Model::Resources::DBNames::DevicesSettingsDB, query, nullptr );
    }
    catch( const std::exception& exception ) {
        Log::error( std::string{ "CoreDRemoteInterface::getDeviceSetting: " } + exception.what() );
    }
    catch( ... ) {
        Log::error( "CoreDRemoteInterface::getDeviceSetting: bad setting's entry in the db is corrupted" );
    }

    return {};
}

CoreDRemoteInterface::DeviceWizardCategories CoreDRemoteInterface::getDeviceWizardCategories() const {
    return m_deviceAddingWizard->getCategories();
}

CoreDInterface::DeviceWizardDevices CoreDRemoteInterface::getDeviceWizardDevices(
    const std::string& categoryFilter ) const {
    return m_deviceAddingWizard->getDevices( categoryFilter );
}

void CoreDRemoteInterface::setDeviceAddingWizard( std::unique_ptr< DeviceAddingWizard > wizard ) {
    m_deviceAddingWizard = std::move( wizard );
}

void CoreDRemoteInterface::processCmd( const std::string& method, rpc::Message& rpcMessage ) {
    using SimpleRequestHandler = void ( CoreDRemoteInterface::* )( rpc::Message & rpcMessage );
    using SimpleRequestHandlersMap = std::unordered_map< std::string, SimpleRequestHandler >;
    static const SimpleRequestHandlersMap simpleRequestHandlers = {
        { "categories.list", &CoreDRemoteInterface::handleGetDeviceWizardCategories },
        { "devices.list", &CoreDRemoteInterface::handleGetDeviceWizardDevices },
    };
    const auto it = simpleRequestHandlers.find( method );
    if( it != simpleRequestHandlers.end() ) {
        const auto handler = it->second;
        ( this->*handler )( rpcMessage );
    }
    else {
        JsonObject& joAnswer = rpcMessage.getAnswer();
        setError( joAnswer, Errors::UNKNOWN_METHOD );
    }
}

void CoreDRemoteInterface::removePluginGateways( const PluginId& pluginId ) {
    auto data = prepareMethod( Model::Resources::RPC::RemovePluginGatewaysRPC );
    data[ "params" ] = JSON( Model::Resources::RPCParams::PluginId << pluginId );

    makeRPC( data );
}

void CoreDRemoteInterface::removeGatewayDevices( const GatewayID& gatewayId ) {
    auto data = prepareMethod( Model::Resources::RPC::RemoveGatewayDevicesRPC );
    data[ "params" ] = JSON( Model::Resources::RPCParams::GatewayId << gatewayId );

    makeRPC( data );
}

void CoreDRemoteInterface::removeDevice( const DeviceID& deviceId ) {
    auto data = prepareMethod( Model::Resources::RPC::RemoveDeviceRPC );
    data[ "params" ] = JSON( Model::Resources::RPCParams::DeviceId << deviceId );

    makeRPC( data );
}

void CoreDRemoteInterface::removeDeviceSync( const DeviceID& deviceId ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, deviceId, MessageSubClasses::REMOVE_DEVICE_SYNC );
}

void CoreDRemoteInterface::removeItem( const ItemID& itemId ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, itemId, MessageSubClasses::REMOVE_ITEM );
}

void CoreDRemoteInterface::removeDeviceSetting( const DeviceSettingId& settingId ) {
    m_hostService.sendRequest( GroupsNames::CORED, settingId, MessageSubClasses::REMOVE_DEVICE_SETTING );
}

void CoreDRemoteInterface::removeDeviceForce( const DeviceID& deviceId ) {
    auto data = prepareMethod( Model::Resources::RPC::ForceRemoveDeviceRPC );
    data[ "params" ] = JSON( Model::Resources::RPCParams::DeviceId << deviceId );

    makeRPC( data );
}

void CoreDRemoteInterface::removePolicy( const PolicyID& policyId ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, policyId, MessageSubClasses::REMOVE_POLICY );
}

Json CoreDRemoteInterface::renameDevice( const JsonObject& device ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, device, MessageSubClasses::RENAME_DEVICE );

    DeviceID deviceID;
    if( !json_get( response[ "id" ], deviceID ) ) {
        Log::error( "CoreDRemoteInterface::renameDevice: bad cored's response" );
        setError( response, Errors::NO_DEVICE_FOUND );
    }

    return response;
}

JsonObject CoreDRemoteInterface::placeDevice( const std::string& deviceId,
                                              const std::string& roomId,
                                              const JsonArray& separateChildDevices ) {
    using namespace Model::Resources;
    const JsonObject device =
        getDBClient().findOne( DBNames::DevicesDB, JSON( DeviceFields::IdFieldName << deviceId ), nullptr );
    JsonObject result;
    if( device.empty() ) {
        Log::error( "CoreDRemoteInterface::placeDevice: device not found" );
        setError( result, Errors::NO_DEVICE_FOUND );
        return result;
    }

    std::string parentDeviceId;
    using namespace HubPlatform::CoreD::Model::Resources;
    if( !json_get( device, DeviceFields::ParentDeviceIdFieldName, parentDeviceId ) ) {
        Log::error( "CoreDRemoteInterface::placeDevice ParentDeviceIdFieldName is absent in device: " + deviceId );
        setError( result, Errors::WRONG_FIELD );
        return result;
    }

    if( !parentDeviceId.empty() ) {
        bool parentRoom = json_get_bool( device, DeviceFields::ParentRoomFieldName, true );

        if( parentRoom ) {
            Log::error(
                "CoreDRemoteInterface::placeDevice Forbidden place subdevice alone ( only parent device with all "
                "subdevices or if child device parentRoom is 'false'" );
            setError( result, Errors::CANT_PLACE_SUBDEVICE );
            return result;
        }
    }

    const JsonObject rooms = getDBClient().findAll( HaRoomsDBName, JSON_EMPTY_OBJ, nullptr );
    const JsonObject room =
        getDBClient().findOne( HaRoomsDBName, JSON( DeviceFields::IdFieldName << roomId ), nullptr );

    std::string roomIdNew = roomId;
    if( roomIdNew == "0" )
        roomIdNew = std::string();

    if( room.empty() && !roomIdNew.empty() ) {
        Log::error( "CoreDRemoteInterface::placeDevice room not found" );
        setError( result, Errors::ROOM_NOT_FOUND );
        return result;
    }

    const auto params = JSON( DeviceFields::IdFieldName << deviceId << DeviceFields::RoomIdFieldName << roomIdNew
                                                        << DeviceFields::SeparateChildDevices << separateChildDevices );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::PLACE_DEVICE );

    DeviceID deviceID;
    if( !json_get( response[ "id" ], deviceID ) ) {
        Log::error( "CoreDRemoteInterface::placeDevice: bad cored's response" );
        setError( result, Errors::NO_DEVICE_FOUND );
        return result;
    }

    return response;
}

JsonObject CoreDRemoteInterface::setDeviceArmed( const DeviceID& deviceId, bool armed ) {
    const JsonObject device = getDBClient().findOne( Model::Resources::DBNames::DevicesDB,
                                                     JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ),
                                                     nullptr );
    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    bool isArmed;
    if( !json_get( device, Model::Resources::DeviceFields::ArmedFieldName, isArmed ) ) {
        LOGGER_DEBUG_RAW( "RemoteInterface::setDeviceArmed: device " + deviceId + " not affected by \"armed\" status" );
        JsonObject result;
        setError( result, Errors::WRONG_FIELD );
        return result;
    }

    const auto params = JSON( "_id" << deviceId << Model::Resources::DeviceFields::ArmedFieldName << armed );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, SET_DEVICE_ARMED );
    return response;
}

JsonObject CoreDRemoteInterface::setDeviceProtectConfig( const DeviceID& deviceId, std::string& protectConfig ) {
    const JsonObject device = getDBClient().findOne( Model::Resources::DBNames::DevicesDB,
                                                     JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ),
                                                     nullptr );

    const JsonObject policy = getDBClient().findOne(
        Model::Resources::DBNames::PoliciesDB, JSON( Model::Resources::PolicyFields::Id << protectConfig ), nullptr );

    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    if( policy.empty() ) {
        throw CoreD::PolicyNotFoundError{ protectConfig };
    }

    const auto params = JSON( "_id" << deviceId << Model::Resources::DeviceFields::ProtectConfig << protectConfig );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, SET_DEVICE_PROTECTCONFIG );
    return response;
}

namespace {

/// @brief throws Model::InvalidValueScale in case of error
void checkScaleValid( const std::string& requestScale, const Model::ValueType& valueType ) {
    if( requestScale.empty() )
        return;

    auto scales = valueType.getScales();
    if( scales.size() > 0 ) {
        auto scalesMatch =
            std::find_if( begin( scales ), end( scales ), [&requestScale]( const Model::ValueScale& currentScale ) {
                if( currentScale && requestScale == currentScale )
                    return true;
                return false;
            } );

        if( scalesMatch == end( scales ) ) {
            auto errorMessage = std::string( "Request scale cannot be converted to item's value scale: " ) +
                                requestScale + " to " + *scales.begin();
            throw Model::InvalidValueScale{ valueType, requestScale.c_str() };
        }
    }
}

/** Saves ValueType of a first checked element. For any other element, compares its valueType with saved
 * valueType. Returns false if saved.valueType != item.valueType.
 */
class SameItemValueTypesChecker {
public:
    using ValueType = Model::ValueType;

    bool operator()( ValueType& nextValueType ) {
        if( m_valueType.getMainType() != ValueType::Enum::NoValue ) {
            return nextValueType == m_valueType;
        }
        m_valueType = nextValueType;
        return true;
    }

protected:
    Model::ValueType m_valueType;
};

ValueUPtr makeScalableValue( const Model::ValueType& valueType,
                             const JsonObject& itemFetched,
                             const Model::Configuration& configuration,
                             const JsonObject& data,
                             const char* valueKey = "value" ) {
    const char* scaleKey = "scale";
    if( valueKey == nullptr )
        valueKey = "value";
    const std::string& reqScale = json_get_str( data, scaleKey );
    std::string scale = reqScale;

    ValueUPtr valueUPtr;
    if( reqScale.empty() && valueType.getScales().size() > 0 ) {
        const std::string& defaultScale = configuration.getScaleForValueType( valueType );

        scale = defaultScale;
        if( scale.empty() && !itemFetched.empty() ) {
            // get a scale from an item
            scale = json_get_str( itemFetched, scaleKey );
        }
    }

    if( !scale.empty() ) {
        valueUPtr = Model::ValueFactory::createItemValue( valueType,
                                                          JSON( valueKey << data[ valueKey ] << scaleKey << scale ) );
    }
    else {
        valueUPtr = Model::ValueFactory::createItemValue( valueType, data[ valueKey ] );
    }
    return valueUPtr;
};

ValueUPtr makeValueImpl( const JsonObject& item,
                         const Model::Configuration& configuration,
                         const JsonObject& data,
                         const std::string& typeField ) {
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;

    const auto valueTypeStr = json_get_str( item, typeField );
    const auto valueType = ValueType::fromString( valueTypeStr );

    ValueUPtr valueUPtr;
    bool isScalable = valueType.getScales().size() > 0;
    if( isScalable ) {
        valueUPtr = makeScalableValue( valueType, item, configuration, data );
    }
    else if( valueType != ValueType::Enum::Action ) {
        valueUPtr = ValueFactory::createItemValue( valueType, data[ "value" ] );
    }
    return valueUPtr;
}

ValueUPtr makeItemValue( const JsonObject& item, const Model::Configuration& configuration, const JsonObject& data ) {
    return makeValueImpl( item, configuration, data, CoreD::Model::Resources::ItemFields::TypeFieldName );
}

ValueUPtr makeSettingValue( const JsonObject& setting,
                            const Model::Configuration& configuration,
                            const JsonObject& data ) {
    return makeValueImpl( setting, configuration, data, CoreD::Model::Resources::DeviceSettingFields::ValueType );
}

};  // namespace

/* data = {
 *          "_id": "5c4df91d28afdb52a6c19534",
 *          "value" : <item_value>
 * }
 * @brief set new item value by specified id
 * @note it's an async call */
void CoreDRemoteInterface::setItemValue( const JsonObject& data, SendResponseCallback sendResponseCallback ) {
    LOGGER_DEBUG_RAW( "CoreDRemoteInterface::setItemValue()" + data.dump() );

    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;

    JsonObject newData;
    auto& ids = data[ "ids" ];
    auto& items = data[ "items" ];
    if( !ids.empty() ) {
        if( !ids.is_array() ) {
            Log::error( cc::error( "\"ids\" parameter should be array" ) );
            return;
        }
        LOGGER_DEBUG_RAW( cc::error( "CoreDRemoteInterface::setItemValue() - multiple: " ) + cc::j( data[ "ids" ] ) );
        newData = handleSetMultipleItemValues( data );
    }
    else if( !items.empty() ) {
        if( !items.is_object() ) {
            Log::error( cc::error( "\"items\" parameter should be object" ) );
            return;
        }
        LOGGER_DEBUG_RAW( cc::debug( "CoreDRemoteInterface::setItemValue() - different items: " ) +
                          cc::j( data[ "items" ] ) );
        newData = handleSetDifferentItemValues( data );
    }
    else {
        LOGGER_DEBUG_RAW( cc::debug( "CoreDRemoteInterface::setItemValue() - single: " ) + cc::j( data[ "_id" ] ) );
        newData = handleSetSingleItemValue( data );
    }

    Json source = JSON_EMPTY_OBJ;
    if( json_get( data, RPCParams::SourceField, source ) && !source.empty() ) {
        checkValueSource( data );
        newData[ RPCParams::SourceField ] = std::move( source );
    }

    LOGGER_DEBUG_RAW( cc::debug( "CoreDRemoteInterface::setItemValue(): " ) + cc::j( newData ) );
    m_hostService.sendAsyncRequest(
        GroupsNames::CORED, newData, SET_ITEM_VALUE, std::move( sendResponseCallback ), ValueSetCommandTimeout );
}

Json CoreDRemoteInterface::prepareSettingValue( const Json& requestData, const Json& setting ) {
    auto data = requestData;
    // replace data.value with value's json
    if( auto valueUPtr = makeSettingValue( setting, getConfiguration(), requestData ) ) {
        data[ "value" ] = valueUPtr->toJson();
    }

    return data;
}

JsonObject CoreDRemoteInterface::setSettingValue( const JsonObject& data ) {
    const std::string settingId = data[ "_id" ];
    auto setting = getDeviceSetting( settingId );
    if( setting.empty() ) {
        throw CoreD::DeviceSettingNotFoundError{ settingId };
    }

    if( isDeviceSettingReadOnly( settingId ) ) {
        throw CoreD::DeviceSettingReadOnlyError( settingId );
    }

    auto newData = prepareSettingValue( data, setting );

    return m_hostService.sendRequest( GroupsNames::CORED, newData, SET_DEVICE_SETTING_VALUE, ValueSetCommandTimeout );
}

JsonObject CoreDRemoteInterface::resetSetting( const JsonObject& data ) {
    const std::string settingId = json_get_str( data, "_id" );
    if( !settingId.empty() ) {
        if( isDeviceSettingReadOnly( settingId ) ) {
            throw CoreD::DeviceSettingReadOnlyError( settingId );
        }
    }

    return m_hostService.sendRequest( GroupsNames::CORED, data, RESET_DEVICE_SETTING, ValueSetCommandTimeout );
}

bool CoreDRemoteInterface::isDeviceSettingReadOnly( const DeviceSettingId& deviceSettingId ) {
    const auto deviceSetting = getDeviceSetting( deviceSettingId );
    if( deviceSetting.empty() ) {
        throw CoreD::DeviceSettingNotFoundError( deviceSettingId );
    }

    return !json_get_bool( deviceSetting, Model::Resources::DeviceSettingFields::HasSetter, true );
}

//  data = {
//      "_id": "deviceId",
//      "check": "value|status", (opt)
//      "items": ["itemId1", "itemId2"] (opt)
//  }
JsonObject CoreDRemoteInterface::checkDevice( const JsonObject& data ) {
    Json answer;
    Json params;

    DeviceID deviceId;
    if( !getNonEmptyField( data, Model::Resources::DeviceFields::IdFieldName, answer, deviceId ) ) {
        return answer;
    }
    params[ "device_id" ] = deviceId;

    std::string check = json_get_str( data, "check" );
    if( !check.empty() ) {
        if( check != "value" && check != "status" ) {
            JsonObject error = JSON_EMPTY_OBJ;
            Errors::getInvalidValueRange( "check", error, "Possible values: value, status" );
            setErrorObject( answer, error );
            return answer;
        }
        params[ "check" ] = check;
    }
    if( data.count( "items" ) ) {
        JsonArray items;
        if( !getFieldAsArray( data, "items", answer, items ) ) {
            return answer;
        }
        for( auto& item : items ) {
            if( !item.is_string() ) {
                JsonObject error = JSON_EMPTY_OBJ;
                Errors::getWrongParams( "items", error );
                setErrorObject( answer, error );
                return answer;
            }
        }
        params[ "items" ] = items;
    }

    return m_hostService.sendRequest( GroupsNames::CORED, params, CHECK_DEVICE, ValueSetCommandTimeout );
}

void CoreDRemoteInterface::setGatewaySettingValue( const JsonObject& data ) {
    if( !json_contains( data, "_id" ) ) {
        throw CoreD::EmptyParameter( "_id" );
    }

    const std::string settingId = data[ "_id" ];

    auto setting = getGatewaySetting( settingId );
    if( setting.empty() ) {
        throw CoreD::GatewaySettingNotFoundError( settingId );
    }

    if( isGatewaySettingReadOnly( settingId ) ) {
        throw CoreD::GatewaySettingReadOnlyError( settingId );
    }

    auto newData = prepareSettingValue( data, setting );

    sendMethodCall( SET_GATEWAY_SETTING_VALUE, newData );
}

void CoreDRemoteInterface::updateGatewaySettingValue( const GatewaySettingId& settingId, ValueLoader valueLoader ) {
    const auto setting = getGatewaySetting( settingId );
    if( setting.empty() ) {
        throw CoreD::GatewaySettingNotFoundError( settingId );
    }

    const auto typeStr = json_get_str( setting, Model::Resources::GatewaySettingFields::ValueType );
    const auto type = Model::ValueType::fromString( typeStr );

    auto newValue = valueLoader( type );

    auto data =
        JSON( Model::Resources::RPCParams::SettingId << settingId << Model::Resources::RPCParams::ValueType << typeStr
                                                     << Model::Resources::RPCParams::Value << newValue->toJson() );

    sendMethodCall( UPDATE_GATEWAY_SETTING_VALUE, data );
}

void CoreDRemoteInterface::updateGatewaySettingStatus( const GatewaySettingId& settingId,
                                                       CoreD::Model::SynchronizationStatus::Enum status ) {
    using namespace Model::Resources;

    if( getGatewaySetting( settingId ).empty() ) {
        throw CoreD::GatewaySettingNotFoundError( settingId );
    }

    auto data = JSON( RPCParams::SettingId << settingId << RPCParams::SynchronizationStatus
                                           << CoreD::Model::SynchronizationStatus::toString( status ) );

    sendMethodCall( UPDATE_GATEWAY_SETTING_STATUS, data );
}

void CoreDRemoteInterface::startGatewayProcess( const GatewayID& gatewayId, CoreD::Model::GatewayProcess process ) {
    using namespace Model::Resources;
    auto data = JSON( RPCParams::GatewayId << gatewayId << RPCParams::Process << Model::toString( process ) );
    sendMethodCall( START_GATEWAY_PROCESS, data );
}

void CoreDRemoteInterface::stopGatewayProcess( const GatewayID& gatewayId,
                                               CoreD::Model::GatewayProcess process,
                                               CoreD::Model::GatewayProcessResult result ) {
    using namespace Model::Resources;
    auto data = JSON( RPCParams::GatewayId << gatewayId << RPCParams::Process << Model::toString( process )
                                           << RPCParams::ProcessResult << Model::toString( result ) );
    sendMethodCall( STOP_GATEWAY_PROCESS, data );
}

CoreD::Model::GatewayProcess CoreDRemoteInterface::getGatewayCurrentProcess( const GatewayID& gatewayId ) const {
    return Model::getGatewayCurrentProcess( getDBClient(), gatewayId );
}

bool CoreDRemoteInterface::isProcessRunning( CoreD::Model::GatewayProcess process ) const {
    return Model::isProcessRunning( getDBClient(), process );
}

ValueUPtr CoreDRemoteInterface::getItemValueInMinMaxRange( const std::string& itemId, int percent ) {
    using namespace CoreD::Model;
    using namespace CoreD::Model::Resources;

    checkItemValueType( itemId, nullptr );

    auto item = getItem( itemId );

    const auto valueTypeStr = json_get_str( item, ItemFields::TypeFieldName );
    const auto valueType = ValueType::fromString( valueTypeStr );

    auto minIt = item.find( ItemFields::ValueMinFieldName );
    auto maxIt = item.find( ItemFields::ValueMaxFieldName );
    if( minIt == item.end() || maxIt == item.end() ) {
        throw ItemHasNoMinMaxFieldsError( itemId );
    }

    auto valueUPtr = ValueFactory::createItemValue( valueType, item[ std::string( ItemFields::ValueFieldName ) ] );

    auto minValueUPtr =
        ValueFactory::createItemValue( valueType, item[ std::string( ItemFields::ValueMinFieldName ) ] );
    auto maxValueUPtr =
        ValueFactory::createItemValue( valueType, item[ std::string( ItemFields::ValueMaxFieldName ) ] );

    ItemValuePercentApplyVisitor modifyCurrentValueInRange( *minValueUPtr.get(), *maxValueUPtr.get(), percent );
    valueUPtr->accept( modifyCurrentValueInRange );

    return valueUPtr;
}

void CoreDRemoteInterface::addItemDictionaryValue( const JsonObject& data, SendResponseCallback sendResponseCallback ) {
    checkItemValueType( data[ "_id" ], std::bind( &Model::ValueType::isDictionary, std::placeholders::_1 ) );
    if( !canAddElementToDictionary( data[ "_id" ] ) ) {
        throw DictionaryValueOverflow();
    }
    Json source = JSON_EMPTY_OBJ;
    if( json_get( data, "source", source ) && !source.empty() )
        checkValueSource( data );

    m_hostService.sendAsyncRequest( GroupsNames::CORED,
                                    data,
                                    ADD_ITEM_DICTIONARY_VALUE,
                                    std::move( sendResponseCallback ),
                                    ValueSetCommandTimeout );
}

void CoreDRemoteInterface::setItemDictionaryValue( const JsonObject& data, SendResponseCallback sendResponseCallback ) {
    checkItemValueType( data[ "_id" ], std::bind( &Model::ValueType::isDictionary, std::placeholders::_1 ) );
    Json source = JSON_EMPTY_OBJ;
    if( json_get( data, "source", source ) && !source.empty() )
        checkValueSource( data );

    m_hostService.sendAsyncRequest( GroupsNames::CORED,
                                    data,
                                    SET_ITEM_DICTIONARY_VALUE,
                                    std::move( sendResponseCallback ),
                                    ValueSetCommandTimeout );
}

void CoreDRemoteInterface::removeItemDictionaryValue( const JsonObject& data,
                                                      SendResponseCallback sendResponseCallback ) {
    checkItemValueType( data[ "_id" ], std::bind( &Model::ValueType::isDictionary, std::placeholders::_1 ) );
    Json source = JSON_EMPTY_OBJ;
    if( json_get( data, "source", source ) && !source.empty() )
        checkValueSource( data );

    m_hostService.sendAsyncRequest( GroupsNames::CORED,
                                    data,
                                    REMOVE_ITEM_DICTIONARY_VALUE,
                                    std::move( sendResponseCallback ),
                                    ValueSetCommandTimeout );
}

JsonObject CoreDRemoteInterface::getItemDictionaryValue( const ItemID& itemId, const std::string& dictionaryId ) {
    auto item = checkItemValueType( itemId, std::bind( &Model::ValueType::isDictionary, std::placeholders::_1 ) );

    auto valueIt = item.find( CoreD::Model::Resources::ItemFields::ValueFieldName );
    if( valueIt == item.end() ) {
        return {};
    }

    auto elementIt = valueIt->find( dictionaryId );
    return elementIt != valueIt->end() ? *elementIt : JsonObject();
}

void CoreDRemoteInterface::clearItemDictionary( const JsonObject& data, SendResponseCallback sendResponseCallback ) {
    checkItemValueType( data[ "_id" ], std::bind( &Model::ValueType::isDictionary, std::placeholders::_1 ) );
    Json source = JSON_EMPTY_OBJ;
    if( json_get( data, "source", source ) && !source.empty() )
        checkValueSource( data );

    m_hostService.sendAsyncRequest(
        GroupsNames::CORED, data, CLEAR_ITEM_DICTIONARY, std::move( sendResponseCallback ), ValueSetCommandTimeout );
}

JsonObject CoreDRemoteInterface::setSettingDictionaryValue( const JsonObject& data ) {
    checkSettingValueType( data[ "_id" ], std::bind( &Model::ValueType::isDictionary, std::placeholders::_1 ) );

    auto& value = data[ "value" ];

    if( json_get_str( value, "name" ).size() > DeviceSettingNameLengthLimit )
        throw CoreD::Model::StringIsTooLongError{ "value.name", DeviceSettingNameLengthLimit };

    if( json_get_str( value, "description" ).size() > DeviceSettingDescriptionLengthLimit )
        throw CoreD::Model::StringIsTooLongError{ "value.description", DeviceSettingDescriptionLengthLimit };

    if( json_get_str( data, "key" ).size() > DictionaryKeyLengthLimit )
        throw CoreD::Model::StringIsTooLongError{ "key", DictionaryKeyLengthLimit };

    return m_hostService.sendRequest(
        GroupsNames::CORED, data, SET_DEVICE_SETTING_DICTIONARY_VALUE, ValueSetCommandTimeout );
}

void CoreDRemoteInterface::updateUserCodeRestriction( const ItemID& itemId, const std::string& restriction ) {
    const auto item = getItem( itemId );
    if( item.empty() )
        throw CoreD::ItemNotFoundError{ itemId };

    const auto typeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
    const auto valueType = Model::ValueType::fromString( typeStr );

    if( !valueType.hasType( Model::ValueType::Enum::UserCode ) ) {
        throw CoreD::InvalidValueTypeError( itemId, "user_code_restriction", typeStr );
    }

    using namespace Model::Resources;

    auto message = prepareMethod( RPC::UpdateItemFieldRPC );
    message[ "params" ] =
        JSON( RPCParams::ItemId << itemId << RPCParams::ItemField << ItemFields::UserCodeRestrictionFieldName
                                << RPCParams::ItemFieldValue << restriction );

    makeRPC( message );
}

void CoreDRemoteInterface::updateItemValue( const ItemID& itemId, ValueLoader valueLoader ) {
    const auto item = getItem( itemId );
    if( item.empty() )
        throw CoreD::ItemNotFoundError{ itemId };

    const auto typeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
    const auto valueType = Model::ValueType::fromString( typeStr );

    auto newValue = valueLoader( valueType );

    auto message = prepareMethod( Model::Resources::RPC::UpdateItemValueRPC );
    message[ "params" ] =
        JSON( Model::Resources::RPCParams::ItemId << itemId << Model::Resources::RPCParams::ItemType << typeStr
                                                  << Model::Resources::RPCParams::ItemValue << newValue->toJson() );

    // Try to reduce memory usage for big objects
    newValue.reset();

    makeRPC( message );
}

void CoreDRemoteInterface::updateItemValueWithMinMax( const ItemID& itemId,
                                                      ValueLoader valueLoader,
                                                      ValueLoader valueMinLoader,
                                                      ValueLoader valueMaxLoader ) {
    using namespace CoreD::Model::Resources;
    using namespace CoreDRemoteInterface;

    const auto item = getItem( itemId );
    if( item.empty() ) {
        throw CoreD::ItemNotFoundError{ itemId };
    }

    const auto typeStr = json_get_str( item, ItemFields::TypeFieldName );
    const auto valueType = Model::ValueType::fromString( typeStr );

    const auto newValue = valueLoader( valueType );
    const auto newMinValue = valueMinLoader( valueType );
    const auto newMaxValue = valueMaxLoader( valueType );

    if( *newValue < *newMinValue || *newValue > *newMaxValue ) {
        LOGGER_WARN( "item value is out of range." );
        return;
    }

    auto message = prepareMethod( RPC::UpdateItemValueRPC );
    message[ "params" ] =
        JSON( RPCParams::ItemId << itemId << RPCParams::ItemType << typeStr << RPCParams::ItemValue
                                << newValue->toJson() << RPCParams::ItemValueMin << newMinValue->toJson()
                                << RPCParams::ItemValueMax << newMaxValue->toJson() );

    makeRPC( message );
}

CoreDRemoteInterface::ItemDictionaryNumberID CoreDRemoteInterface::generateItemDictionaryNumberId(
    const ItemID& itemId,
    ItemDictionaryNumberID minId,
    ItemDictionaryNumberID maxId ) {
    const auto item = getItem( itemId );
    if( item.empty() )
        throw CoreD::ItemNotFoundError{ itemId };

    const auto valueType =
        Model::ValueType::fromString( json_get_str( item, Model::Resources::ItemFields::TypeFieldName ) );
    if( !valueType.isDictionary() ) {
        throw HubPlatform::CoreD::Model::UnexpectedValueType{ valueType };
    }

    const Json& joCurrentValue = item[ Model::Resources::ItemFields::ValueFieldName ];
    ItemDictionaryNumberID result = minId;
    for( ; result <= maxId && joCurrentValue.find( std::to_string( result ) ) != joCurrentValue.end(); ++result )
        ;
    if( result > maxId ) {
        throw Model::ItemDictionaryIdGenerationError{};
    }
    return result;
}

void CoreDRemoteInterface::updateItemDictionaryValue( const ItemID& itemId,
                                                      const DictionaryId& itemDictionaryId,
                                                      ValueLoader valueLoader ) {
    const auto item = getItem( itemId );
    if( item.empty() )
        throw CoreD::ItemNotFoundError{ itemId };

    const auto& joCurrentValue = item[ Model::Resources::ItemFields::ValueFieldName ];
    const auto typeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );
    const auto type = Model::ValueType::fromString( typeStr );
    auto dictionaryValue = Model::DictionaryValue::createValue( type, joCurrentValue );
    auto newValue = valueLoader( dictionaryValue->getValuesType() );

    // NOTE: probably we can live without this old value
    const auto oldValue = dictionaryValue->getValue( itemDictionaryId );

    if( !newValue && !oldValue || ( newValue && oldValue && *newValue == *oldValue ) ) {
        LOGGER_TRACE( "update dictionary item to the same value." );
        return;
    }

    if( !canAddElementToDictionary( itemId ) && !oldValue /* we're modifying, not adding a dictionary element*/ ) {
        LOGGER_WARN( LOGGER_PRETTY_FUNCTION +
                     ": adding new element would cause a dictionary to exceed a max allowed elements amount" );
        return;  // to behave like updateItemValueWithMinMax
    }

    Json newValueJson = ( newValue ) ? newValue->toJson() : JSON_EMPTY_OBJ;

    using namespace Model::Resources;
    auto message = prepareMethod( RPC::UpdateItemDictionaryValueRPC );
    message[ "params" ] = JSON( RPCParams::ItemId << itemId << RPCParams::ItemDictionaryId << itemDictionaryId
                                                  << RPCParams::ItemDictionaryValue << newValueJson );

    makeRPC( message );
}

void CoreDRemoteInterface::updateSettingValue( const DeviceSettingId& settingId,
                                               ValueLoader valueLoader,
                                               CoreD::Model::SynchronizationStatus::Enum status ) {
    const auto setting = getDeviceSetting( settingId );
    if( setting.empty() )
        throw CoreD::DeviceSettingNotFoundError{ settingId };

    const auto typeStr = json_get_str( setting, Model::Resources::DeviceSettingFields::ValueType );
    const auto type = Model::ValueType::fromString( typeStr );

    auto newValue = valueLoader( type );

    auto message = prepareMethod( Model::Resources::RPC::UpdateSettingValueRPC );
    message[ "params" ] =
        JSON( Model::Resources::RPCParams::SettingId << settingId << Model::Resources::RPCParams::ValueType << typeStr
                                                     << Model::Resources::RPCParams::Value << newValue->toJson()
                                                     << Model::Resources::RPCParams::SynchronizationStatus
                                                     << CoreD::Model::SynchronizationStatus::toString( status ) );

    makeRPC( message );
}

void CoreDRemoteInterface::updateSettingDictionaryValue( const DeviceSettingId& settingId,
                                                         const DictionaryId& key,
                                                         ValueLoader valueLoader ) {
    const auto setting = getDeviceSetting( settingId );
    if( setting.empty() )
        throw CoreD::DeviceSettingNotFoundError{ settingId };

    const auto typeStr = json_get_str( setting, Model::Resources::DeviceSettingFields::ValueType );
    const auto type = Model::ValueType::fromString( typeStr );

    const auto& currentValue = setting[ Model::Resources::DeviceSettingFields::Value ];
    const auto dictionaryValue = Model::DictionaryValue::createValue( type, currentValue );

    const auto oldValue = dictionaryValue->getValue( key );
    const auto newValue = valueLoader( dictionaryValue->getValuesType() );

    if( !newValue && !oldValue || ( newValue && oldValue && *newValue == *oldValue ) ) {
        LOGGER_TRACE( "update configuration to the same value" );
        return;
    }

    auto newValueJson = newValue ? newValue->toJson() : Json{};

    using namespace Model::Resources;
    auto message = prepareMethod( RPC::UpdateSettingDictionaryValueRPC );
    message[ "params" ] = JSON( RPCParams::SettingId << settingId << RPCParams::DictionaryKey << key
                                                     << RPCParams::DictionaryValue << newValueJson );

    makeRPC( message );
}

void CoreDRemoteInterface::updateSettingStatus( const DeviceSettingId& settingId,
                                                CoreD::Model::SynchronizationStatus::Enum status ) {
    using namespace Model::Resources;

    auto message = prepareMethod( RPC::UpdateSettingStatusRPC );
    message[ "params" ] = JSON( RPCParams::SettingId << settingId << RPCParams::SynchronizationStatus
                                                     << CoreD::Model::SynchronizationStatus::toString( status ) );

    makeRPC( message );
}

void CoreDRemoteInterface::updateSettingDictionaryStatus( const DeviceSettingId& settingId,
                                                          const DictionaryId& key,
                                                          CoreD::Model::SynchronizationStatus::Enum status ) {
    using namespace Model::Resources;

    const auto setting = getDeviceSetting( settingId );
    if( setting.empty() )
        throw CoreD::DeviceSettingNotFoundError{ settingId };

    const auto typeStr = json_get_str( setting, DeviceSettingFields::ValueType );
    const auto type = Model::ValueType::fromString( typeStr );

    const auto& currentValue = setting[ DeviceSettingFields::Value ];
    const auto dictionaryValue = Model::DictionaryValue::createValue( type, currentValue );

    auto oldValue = dictionaryValue->getValue( key );

    if( !oldValue ) {
        LOGGER_TRACE( "value doesn't exist for the key" );
        return;
    }

    Json newValueJson = oldValue->toJson();
    newValueJson[ DeviceSettingFields::SynchronizationStatus ] = Model::SynchronizationStatus::toString( status );

    auto message = prepareMethod( RPC::UpdateSettingDictionaryValueRPC );
    message[ "params" ] = JSON( RPCParams::SettingId << settingId << RPCParams::DictionaryKey << key
                                                     << RPCParams::DictionaryValue << newValueJson );

    makeRPC( message );
}

JsonObject CoreDRemoteInterface::makeWhatHappenedRequest( const JsonObject& data, const MessageSubClasses subclass ) {
    if( subclass == GET_WHAT_HAPPENED ) {
        JsonObject answer;
        CoreD::WhatHappened whatHappened( getDBClient() );
        whatHappened.getWhatHappened( data, answer );
        return answer;
    }
    return m_hostService.sendRequest( GroupsNames::CORED, data, subclass );
}

void CoreDRemoteInterface::updateReachableState( const DeviceID& deviceId, bool reachable ) {
    const JsonObject device = findDevice( JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ) );
    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    if( json_get_bool( device, Model::Resources::DeviceFields::ReachableFieldName ) == reachable ) {
        LOGGER_DEBUG( "Update reachable state to the same value." );
        return;
    }

    auto message = prepareMethod( Model::Resources::RPC::UpdateReachableStateRPC );
    message[ "params" ] = JSON( Model::Resources::RPCParams::DeviceId
                                << deviceId << Model::Resources::RPCParams::ReachableState << reachable );

    makeRPC( message );
}

void CoreDRemoteInterface::updateDeviceStatus( const CoreDInterface::DeviceID& deviceId, const std::string& status ) {
    LOGGER_DEBUG( "device id: " + cc::debug( deviceId ) + ", status: " + cc::debug( status ) );

    const JsonObject device = findDevice( JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ) );
    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    if( json_get_str( device, Model::Resources::DeviceFields::StatusFieldName ) == status ) {
        LOGGER_DEBUG( "Update device status to the same value." );
        return;
    }

    auto params = JSON_EMPTY_OBJ;
    params[ Model::Resources::RPCParams::DeviceId ] = deviceId;
    params[ Model::Resources::RPCParams::DeviceStatus ] = status;

    sendMethodCall( UPDATE_DEVICE_STATUS, params );
}

void CoreDRemoteInterface::updateReadyState( const CoreDInterface::DeviceID& deviceId, bool ready ) {
    const JsonObject device = findDevice( JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ) );
    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    if( json_get_bool( device, Model::Resources::DeviceFields::ReadyFieldName ) == ready ) {
        LOGGER_DEBUG( "Update ready state to the same value." );
        return;
    }

    auto message = prepareMethod( Model::Resources::RPC::UpdateReadyStateRPC );
    message[ "params" ] =
        JSON( Model::Resources::RPCParams::DeviceId << deviceId << Model::Resources::RPCParams::ReadyState << ready );
    makeRPC( message );
}

void CoreDRemoteInterface::updateDeviceName( const DeviceID& deviceId, const std::string& deviceName ) {
    const JsonObject device = findDevice( JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ) );
    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    if( json_get_str( device, Model::Resources::DeviceFields::NameFieldName ) == deviceName ) {
        LOGGER_DEBUG( "Update device name to the same value." );
        return;
    }

    auto message = prepareMethod( Model::Resources::RPC::UpdateDeviceNameRPC );
    message[ "params" ] = JSON( Model::Resources::RPCParams::DeviceId
                                << deviceId << Model::Resources::RPCParams::DeviceName << deviceName );

    makeRPC( message );
}

void CoreDRemoteInterface::updateDeviceRoomId( const DeviceID& deviceId, const std::string& deviceRoomId ) {
    const JsonObject device = findDevice( JSON( Model::Resources::DeviceFields::IdFieldName << deviceId ) );
    if( device.empty() )
        throw CoreD::DeviceNotFoundError{ deviceId };

    if( json_get_str( device, Model::Resources::DeviceFields::RoomIdFieldName ) == deviceRoomId ) {
        LOGGER_DEBUG( "Update room Id to the same value." );
        return;
    }

    auto message = prepareMethod( Model::Resources::RPC::UpdateDeviceRoomIdRPC );
    message[ "params" ] = JSON( Model::Resources::RPCParams::DeviceId
                                << deviceId << Model::Resources::RPCParams::DeviceRoomId << deviceRoomId );

    makeRPC( message );
}

void CoreDRemoteInterface::sendPluginUIBroadcast( std::string pluginId, JsonObject broadcastData ) {
    broadcastData[ "plugin" ] = std::move( pluginId );
    m_hostService.sendBroadcast( PLUGIN_UI_BROADCAST, broadcastData, GroupsNames::UI_GROUP );
}

JsonObject CoreDRemoteInterface::setHouseMode( const HouseModeId& houseModeId ) {
    const MessageSubClasses subclass = SET_HOUSE_MODE;
    LOGGER_DEBUG_RAW( "RemoteInterface::setHouseMode, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( CoreDHouseModes::HOUSE_MODE_ENTRY << houseModeId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );

    return response;
}

CoreDInterface::HouseModeId CoreDRemoteInterface::cancelHouseModeSwitching() {
    const MessageSubClasses subclass = CANCEL_SWITCHING_HOUSE_MODE;
    LOGGER_DEBUG_RAW( "RemoteInterface::cancelHouseModeSwitching, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    const auto response = m_hostService.sendRequest( GroupsNames::CORED, JSON_EMPTY_OBJ, subclass );
    return json_get_str( response, "modeId" );
}

JsonObject CoreDRemoteInterface::cancelEntryDelay() {
    const MessageSubClasses subclass = HOUSE_MODE_CANCEL_ENTRY_DELAY;
    LOGGER_DEBUG_RAW( "RemoteInterface::cancelEntryDelay, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    return m_hostService.sendRequest( GroupsNames::CORED, JSON_EMPTY_OBJ, subclass );
}

JsonObject CoreDRemoteInterface::skipEntryDelay() {
    const MessageSubClasses subclass = HOUSE_MODE_SKIP_ENTRY_DELAY;
    LOGGER_INFO_RAW( cc::info( "Skip entry delay" ) );

    return m_hostService.sendRequest( GroupsNames::CORED, JSON_EMPTY_OBJ, subclass );
}

JsonObject CoreDRemoteInterface::setEntryDelay( const JsonObject& params ) {
    const MessageSubClasses subclass = HOUSE_MODE_SET_ENTRY_DELAY;
    LOGGER_DEBUG_RAW( "RemoteInterface::resetEntryDelay, sending house mode Entry Delay Set Command " );

    return m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
}

JsonObject CoreDRemoteInterface::resetEntryDelay() {
    const MessageSubClasses subclass = HOUSE_MODE_ENTRY_DELAY_RESET;
    LOGGER_DEBUG_RAW( "RemoteInterface::resetEntryDelay, sending house mode Entry Delay Reset Command " );

    return m_hostService.sendRequest( GroupsNames::CORED, JSON_EMPTY_OBJ, subclass );
}

CoreDInterface::HouseModesGetResult CoreDRemoteInterface::getHouseModes() const {
    return CoreDHouseModes::getHouseModesList( getDBClient() );
}

CoreDInterface::DeviceIDs CoreDRemoteInterface::getArmedDeviceIDsAffectedByHouseModes() const {
    return CoreDHouseModes::getArmedDeviceIDsAffectedByHouseModes( getDBClient() );
}

CoreDInterface::DeviceIDs CoreDRemoteInterface::getAlarmDeviceIDsAffectedByHouseModes() const {
    return CoreDHouseModes::getAlarmDeviceIDsAffectedByHouseModes( getDBClient() );
}

CoreDInterface::HouseModeId CoreDRemoteInterface::getCurrentHouseModeId() const {
    return CoreDHouseModes::getCurrentHouseModeId( getDBClient() );
}

CoreDInterface::HouseModeId CoreDRemoteInterface::getHouseModeIdSwitchTo() const {
    return CoreDHouseModes::getHouseModeSwitchTo( getDBClient() );
}

std::unique_ptr< CoreDHouseModes::HouseModeEntry > CoreDRemoteInterface::getHouseModeById(
    const HouseModeId& id ) const {
    return CoreDHouseModes::getHouseMode( getDBClient(), id, true );
}

std::unique_ptr< CoreDHouseModes::HouseModeEntry > CoreDRemoteInterface::getHouseModeByName(
    const std::string& name ) const {
    return CoreDHouseModes::getHouseModeByName( getDBClient(), name, true );
}

void CoreDRemoteInterface::setSwitchDelay( int delay ) {
    CoreDHouseModes::setSwitchDelay( getDBClient(), delay );
}

void CoreDRemoteInterface::setAlarmDelay( int delay ) {
    CoreDHouseModes::setAlarmDelay( getDBClient(), delay );
}

int CoreDRemoteInterface::getSwitchDelayLeft() {
    return CoreDHouseModes::getSwitchDelayLeft( getDBClient() );
}

int CoreDRemoteInterface::getSwitchDelay() {
    return CoreDHouseModes::getSwitchDelay( getDBClient() );
}

void CoreDRemoteInterface::setAlarmPhase( const std::string& phase, const std::string& status ) {
    return CoreDHouseModes::setAlarmPhase( getDBClient(), phase, status );
}

std::tuple< std::string, std::string > CoreDRemoteInterface::getAlarmPhase() {
    return CoreDHouseModes::getAlarmPhase( getDBClient() );
}

std::string CoreDRemoteInterface::getSwitchToPhase() {
    return CoreDHouseModes::getSwitchToPhase( getDBClient() );
}

int CoreDRemoteInterface::getAlarmDelay() {
    return CoreDHouseModes::getAlarmDelay( getDBClient() );
}
Json CoreDRemoteInterface::getEntryDelay() {
    return CoreDHouseModes::getEntryDelay( getDBClient() );
}

Json CoreDRemoteInterface::getAbortWindow() {
    return CoreDHouseModes::getAbortWindow( getDBClient() );
}

JsonObject CoreDRemoteInterface::setDisarmedDefault( const HouseModeId& houseModeId, bool disarmedDefault ) {
    const MessageSubClasses subclass = SET_DISARMED_DEFAULT_HOUSE_MODE;
    LOGGER_DEBUG_RAW( "RemoteInterface::setDisarmedDefault, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "disarmedDefault" << disarmedDefault );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::houseModeProtectSet( const HouseModeId& houseModeId, bool protect ) {
    return m_hostService.sendRequest(
        GroupsNames::CORED, JSON( "modeId" << houseModeId << "protect" << protect ), HOUSE_MODE_PROTECT_SET );
}

JsonObject CoreDRemoteInterface::addDisarmedDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) {
    const MessageSubClasses subclass = HOUSE_MODE_ADD_DISARMED_DEVICE;
    LOGGER_DEBUG_RAW( "RemoteInterface::addDisarmedDevice, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceId" << deviceId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::removeDisarmedDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) {
    const MessageSubClasses subclass = HOUSE_MODE_REMOVE_DISARMED_DEVICE;
    LOGGER_DEBUG_RAW( "RemoteInterface::removeDisarmedDevice, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceId" << deviceId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::setNotificationsForHouseMode( const HouseModeId& houseModeId,
                                                               bool notifyAll,
                                                               const std::vector< UserId >& userIds ) {
    const MessageSubClasses subclass = SET_NOTIFICATIONS_HOUSE_MODE;
    LOGGER_DEBUG_RAW( "RemoteInterface::setNotificationsForHouseMode, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "all" << notifyAll << "userIds" << userIds );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::addAlarmsOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) {
    const MessageSubClasses subclass = HOUSE_MODE_ADD_ALARMS_OFF_DEVICE;
    LOGGER_DEBUG_RAW( "RemoteInterface::addAlarmsOffDevice, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceId" << deviceId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::removeAlarmsOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) {
    const MessageSubClasses subclass = HOUSE_MODE_REMOVE_ALARMS_OFF_DEVICE;
    LOGGER_DEBUG_RAW( "RemoteInterface::removeAlarmsOffDevice, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceId" << deviceId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::addCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) {
    const MessageSubClasses subclass = HOUSE_MODE_ADD_CAMERAS_OFF_DEVICE;
    LOGGER_DEBUG_RAW( "RemoteInterface::addCamerasOffDevice, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceId" << deviceId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::removeCamerasOffDevice( const HouseModeId& houseModeId, const DeviceID& deviceId ) {
    const MessageSubClasses subclass = HOUSE_MODE_REMOVE_CAMERAS_OFF_DEVICE;
    LOGGER_DEBUG_RAW( "RemoteInterface::removeCamerasOffDevice, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceId" << deviceId );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::addBypassDevices( const HouseModeId& houseModeId,
                                                   const std::vector< DeviceID >& deviceIds ) {
    const MessageSubClasses subclass = HOUSE_MODE_ADD_BYPASS_DEVICES;
    LOGGER_DEBUG_RAW( "RemoteInterface::addBypassDevices, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceIds" << Json( deviceIds ) );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::removeBypassDevices( const HouseModeId& houseModeId,
                                                      const std::vector< DeviceID >& deviceIds ) {
    const MessageSubClasses subclass = HOUSE_MODE_REMOVE_BYPASS_DEVICES;
    LOGGER_DEBUG_RAW( "RemoteInterface::removeBypassDevices, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "modeId" << houseModeId << "deviceIds" << Json( deviceIds ) );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

CoreDRemoteInterface::GeoPositionState CoreDRemoteInterface::getGeoPosition() const {
    const auto latField = JSON( SettingsFields::LAT_NAME << 1 );
    const auto lngField = JSON( SettingsFields::LNG_NAME << 1 );

    JsonObject latObj =
        getDBClient().findOne( HUBSettings, JSON( db::ID_FIELD_NAME << SettingsFields::LAT_NAME ), &latField );
    JsonObject lngObj =
        getDBClient().findOne( HUBSettings, JSON( db::ID_FIELD_NAME << SettingsFields::LNG_NAME ), &lngField );

    bool geopositionIsDefault = false;
    GeoPosition pos{ DefaultLatitude, DefaultLongitude };
    JsonObject::const_iterator latIt = latObj.find( SettingsFields::LAT_NAME );
    JsonObject::const_iterator lngIt = lngObj.find( SettingsFields::LNG_NAME );
    if( latIt != latObj.cend() && lngIt != lngObj.cend() ) {
        pos.latitude = latIt->get< float >();
        pos.longitude = lngIt->get< float >();
    }
    else {
        geopositionIsDefault = true;
    }
    return GeoPositionState( pos, geopositionIsDefault );
}

JsonObject CoreDRemoteInterface::setGeoPosition( const GeoPosition& pos ) {
    JsonObject value = { { SettingsFields::LAT_NAME, pos.latitude }, { SettingsFields::LNG_NAME, pos.longitude } };
    JsonObject res = m_hostService.sendRequest( GroupsNames::CORED, value, HUB_SET_COORDINATES );
    return res;
}

std::string CoreDRemoteInterface::getHubTimeZone() {
    try {
#if defined( __HUB_TYPE_LOCAL__ )
        JsonObject timezone =
            getDBClient().findOne( HUBSettings, JSON( db::ID_FIELD_NAME << SettingsFields::ZONE_NAME ), nullptr );
        return json_get_str( timezone, "value" );
#elif defined( __HUB_TYPE_VHUB__ )
        return system_utils::DebianTimeZone().getTimeZone();
#else
        return system_utils::OpenWrtTimeZone().getTimeZone();
#endif
    }
    catch( std::exception& e ) {
        LOGGER_ERROR( cc::error( "Failed to get hub timezone: " ) + cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Failed to get hub timezone: unknown exception" ) );
    }
    return {};
}

bool CoreDRemoteInterface::isInsideWeekInterval( const std::vector< std::string >& weekdays,
                                                 const std::string& startTime,
                                                 const std::string& endTime ) {
    auto weekdaysSet = convertWeekdays( weekdays );
    assert( m_localTimeGetter );
    auto currentDate = m_localTimeGetter();

    // Wrong week day
    if( weekdaysSet.find( currentDate.getWeekday() ) == weekdaysSet.end() ) {
        return false;
    }

    // Check time intervals
    GregorianCalendar::Date startDate = getDateWithModifiedTime( currentDate, startTime );
    GregorianCalendar::Date endDate = getDateWithModifiedTime( currentDate, endTime );

    return ( startDate <= currentDate ) && ( currentDate <= endDate );
}

std::string CoreDRemoteInterface::getNextWeekEvent( const std::vector< std::string >& weekdays,
                                                    const std::string& time ) {
    auto weekdaysSet = convertWeekdays( weekdays );
    assert( m_localTimeGetter );
    auto currentDate = m_localTimeGetter();
    GregorianCalendar::Date targetDate = getDateWithModifiedTime( currentDate, time );

    for( int i = 0; i <= 1; ++i, targetDate.incrementWeek() ) {
        for( const auto& weekday : weekdaysSet ) {
            targetDate.moveToWeekday( weekday );

            if( currentDate < targetDate ) {
                return targetDate.toString( "%FT%T" );
            }
        }
    }

    return "";
}

std::string CoreDRemoteInterface::calculateTimeWithDelay( const std::string& time ) {
    auto currentDate = m_localTimeGetter();
    auto tm = TimeUtils::stringToTimeStruct( time, "%H:%M:%S" );

    currentDate.addHours( tm.tm_hour );
    currentDate.addMinutes( tm.tm_min );
    currentDate.addSeconds( tm.tm_sec );

    return currentDate.toString( "%FT%T" );
}

std::string CoreDRemoteInterface::getHubLocationState( const CoreDRemoteInterface::GeoPositionState& locationState ) {
    const auto& timeZoneIsDefault = isHubTimeZoneDefault();

    if( timeZoneIsDefault && locationState.second ) {
        return "default";
    }
    else if( timeZoneIsDefault && !locationState.second ) {
        return "customCoordinates";
    }
    else if( !timeZoneIsDefault && locationState.second ) {
        return "customTimezone";
    }

    return "customAll";
}

bool CoreDRemoteInterface::isHubTimeZoneDefault() {
    try {
#if defined( __HUB_TYPE_VHUB__ )
        return system_utils::DebianTimeZone().isTimeZoneDefault();
#else
        return system_utils::OpenWrtTimeZone().isTimeZoneDefault();
#endif
    }
    catch( std::exception& e ) {
        LOGGER_ERROR( cc::error( "Failed to check hub timezone: " ) + cc::note( e.what() ) );
    }
    catch( ... ) {
        LOGGER_ERROR( cc::error( "Failed to check hub timezone: unknown exception" ) );
    }
    return false;
}

JsonObject CoreDRemoteInterface::setHubTimeZone( const JsonObject& params ) {
    return m_hostService.sendRequest( GroupsNames::CORED, params, HUB_SET_TIME_ZONE );
}

JsonObject CoreDRemoteInterface::getAllSupportedTimeZones() const {
    auto timeZones = system_utils::getAllSupportedTimeZones();
    JsonArray listOfTimeZones;
    if( !timeZones.empty() ) {
        listOfTimeZones = timeZones;
    }

    JsonObject result{ { HubPlatform::Uid::FieldNames::ListLocationsFieldName, listOfTimeZones } };
    return result;
}

JsonObject CoreDRemoteInterface::getAllRooms() {
    JsonArray rooms = getDBClient().findAll( Model::Resources::DBNames::RoomsDB, JSON_EMPTY_OBJ, nullptr );
    if( rooms.size() > 1 ) {
        Json userOrdering =
            getDBClient().findOne( Model::Resources::DBNames::RoomsOrderDB,
                                   JSON( db::ID_FIELD_NAME << Model::Resources::RoomRecords::OrderRecordId ),
                                   nullptr );
        JsonArray roomsId = JSON_EMPTY_ARR;
        if( !userOrdering.empty() && json_get_arr( userOrdering, "roomsId", roomsId ) )
            jsonSort( rooms, roomsId );
    }
    return rooms;
}

JsonObject CoreDRemoteInterface::createRoom( const std::string& name ) {
    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, JSON( "name" << name ), MessageSubClasses::CREATE_ROOM );
    return response;
}

JsonObject CoreDRemoteInterface::setRoomName( const CoreDInterface::RoomId& roomId, const std::string& newName ) {
    auto response = m_hostService.sendRequest(
        GroupsNames::CORED, JSON( "_id" << roomId << "name" << newName ), MessageSubClasses::SET_ROOM_NAME );
    return response;
}

JsonObject CoreDRemoteInterface::setRoomsOrder( const std::vector< CoreDInterface::RoomId >& orderedRooms ) {
    auto response = m_hostService.sendRequest(
        GroupsNames::CORED, JSON( "roomsId" << orderedRooms ), MessageSubClasses::SET_ROOM_ORDER );
    return response;
}

JsonObject CoreDRemoteInterface::getRoom( const CoreDInterface::RoomId& roomId ) {
    return getDBClient().findOne( Model::Resources::DBNames::RoomsDB, JSON( "_id" << roomId ), nullptr );
}

JsonObject CoreDRemoteInterface::deleteRoom( const CoreDInterface::RoomId& roomId ) {
    auto response =
        m_hostService.sendRequest( GroupsNames::CORED, JSON( "_id" << roomId ), MessageSubClasses::DELETE_ROOM );
    return response;
}

JsonObject CoreDRemoteInterface::deleteAllRooms() {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, JSON_EMPTY_OBJ, MessageSubClasses::DELETE_ROOM_ALL );
    return response;
}

JsonObject CoreDRemoteInterface::setServiceNotifications( const JsonArray& ids ) {
    return m_hostService.sendRequest(
        GroupsNames::CORED, JSON( "ids" << ids ), MessageSubClasses::SET_SERVICE_NOTIFICATIONS );
}

void CoreDRemoteInterface::sendMethodCall( MessageSubClasses methodType, const JsonObject& data ) {
    m_hostService.sendMethodCall( GroupsNames::CORED, methodType, data );
}

void CoreDRemoteInterface::makeRPC( const Json& rpcMessage ) const {
    m_hostService.sendUnsafe( { GroupsNames::CORED, JSON_RPC, CALL_METHOD, rpcMessage } );
}

JsonObject CoreDRemoteInterface::findItem( const Json& filter ) const {
    return findOne( filter, Model::Resources::DBNames::ItemsDB, Model::Resources::DBNames::TempItemsDB );
}

CoreDRemoteInterface::RawItems CoreDRemoteInterface::findItems( const Json& filter ) const {
    return findAll( filter,
                    Model::Resources::DBNames::ItemsDB,
                    Model::Resources::DBNames::TempItemsDB,
                    Model::Resources::ItemFields::IdFieldName );
}

JsonObject CoreDRemoteInterface::findDevice( const Json& filter ) const {
    return findOne( filter, Model::Resources::DBNames::DevicesDB, Model::Resources::DBNames::TempDevicesDB );
}

CoreDRemoteInterface::RawDevices CoreDRemoteInterface::findDevices( const Json& filter ) const {
    return findAll( filter,
                    Model::Resources::DBNames::DevicesDB,
                    Model::Resources::DBNames::TempDevicesDB,
                    Model::Resources::DeviceFields::IdFieldName );
}

JsonObject CoreDRemoteInterface::findPolicy( const Json& filter ) const {
    return findOne( filter, Model::Resources::DBNames::PoliciesDB, nullptr );
}

CoreDRemoteInterface::RawDevices CoreDRemoteInterface::findPolicies( const Json& filter ) const {
    return findAll( filter, Model::Resources::DBNames::PoliciesDB, nullptr, Model::Resources::PolicyFields::Id );
}

JsonObject CoreDRemoteInterface::findOne( const Json& filter, const char* dbName, const char* tempDbName ) const {
    auto element = getDBClient().findOne( dbName, filter, nullptr );
    auto elementDynamic = getDBClient().findOne( tempDbName, filter, nullptr );

    return JSON( element << elementDynamic );
}

CoreDRemoteInterface::JsonObjects CoreDRemoteInterface::findAll( const Json& filter,
                                                                 const char* dbName,
                                                                 const char* tempDbName,
                                                                 const char* idFieldName ) const {
    const auto elements = getDBClient().findAll( dbName, filter, nullptr );
    JsonObjects resultElements;

    for( const auto& element : elements ) {
        const auto elementDynamic =
            getDBClient().findOne( tempDbName, JSON( idFieldName << element[ idFieldName ] ), nullptr );
        resultElements.push_back( JSON( element << elementDynamic ) );
    }

    return resultElements;
}

IDbClient& CoreDRemoteInterface::getDBClient() {
    return m_hostService.getDBClient();
}

const IDbClient& CoreDRemoteInterface::getDBClient() const {
    return m_hostService.getDBClient();
}

void CoreDRemoteInterface::handleGetDeviceWizardCategories( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    DeviceWizardCategories response = getDeviceWizardCategories();
    if( !response.second ) {
        setErrorIfHas( joAnswer, response.first );
        return;
    }
    joAnswer[ "result" ] = std::move( response.first[ "result" ] );
}

void CoreDRemoteInterface::handleGetDeviceWizardDevices( rpc::Message& rpcMessage ) {
    JsonObject& joAnswer = rpcMessage.getAnswer();
    std::string categoryFilter;
    rpcMessage.getOptionalParamsField( "wizardCategory", categoryFilter );
    DeviceWizardDevices response = getDeviceWizardDevices( categoryFilter );
    if( !response.second ) {
        setErrorIfHas( joAnswer, response.first );
        return;
    }
    joAnswer[ "result" ] = std::move( response.first[ "result" ] );
}

Json CoreDRemoteInterface::handleSetMultipleItemValues( JsonObject data ) {
    std::vector< JsonObject > itemsFetched;
    auto valueChecker = getValueChecker( data );
    for( const auto& itemId : data[ "ids" ] ) {
        LOGGER_DEBUG_RAW( "checkItemValueType(): " + cc::j( itemId ) );
        itemsFetched.push_back( checkItemValueType( itemId, valueChecker ) );
    }
    if( !itemsFetched.empty() ) {
        LOGGER_DEBUG_RAW( "itemsFetched: " + std::to_string( itemsFetched.size() ) );
        ValueUPtr valueUPtr = makeItemValue( itemsFetched[ 0 ], getConfiguration(), data );
        // replace data.value with value's json
        if( valueUPtr != nullptr ) {
            data[ "value" ] = valueUPtr->toJson();
        }
    }
    return data;
}

Json CoreDRemoteInterface::handleSetDifferentItemValues( JsonObject data ) {
    auto& items = data[ "items" ];
    for( auto it = items.begin(); it != items.end(); ++it ) {
        auto& value = it.value()[ "value" ];
        auto item = checkItemValueType( it.key(), getValueChecker( value ) );
        ValueUPtr valueUPtr = makeItemValue( item, getConfiguration(), JSON( "_id" << it.key() << "value" << value ) );
        if( valueUPtr != nullptr ) {
            value = valueUPtr->toJson();
        }
    }
    return data;
}

Json CoreDRemoteInterface::handleSetSingleItemValue( JsonObject data ) {
    JsonObject item = checkItemValueType( data[ "_id" ], getValueChecker( data ) );
    ValueUPtr valueUPtr = makeItemValue( item, getConfiguration(), data );
    // replace data.value with value's json
    if( valueUPtr != nullptr ) {
        data[ "value" ] = valueUPtr->toJson();
    }
    return data;
}

JsonObject CoreDRemoteInterface::getHubStatus( const rpc::Message& request ) {
    auto id = request.getId();
    auto response = m_hostService.sendRequest( "cored", JSON( "id" << id ), MessageSubClasses::GET_HUB_STATUS );

    if( response.empty() ) {
        Log::error( "CoreDRemoteInterface::getHubStatus: bad cored's response" );
        return JSON_EMPTY_OBJ;
    }

    return response;
}

JsonObject CoreDRemoteInterface::updateDeviceFirmware( const JsonObject& request ) {
    auto response = m_hostService.sendRequest( GroupsNames::CORED, request, MessageSubClasses::UPDATE_DEVICE_FIRMWARE );
    return response;
}

JsonObject CoreDRemoteInterface::checkValueType( JsonObject object,
                                                 const std::string& typeStr,
                                                 CheckPred checkPred ) const {
    using namespace CoreD::Model::Resources;

    checkBusyDevice( json_get_str( object, ItemFields::DeviceIdFieldName ) );

    const auto actualValueType = Model::ValueType::fromString( typeStr );
    if( checkPred && !checkPred( actualValueType ) )
        throw Model::UnexpectedValueType( actualValueType );

    return object;
}

JsonObject CoreDRemoteInterface::checkItemValueType( const ItemID& itemId, CheckPred checkPred ) const {
    using namespace CoreD::Model::Resources;
    auto item = getItem( itemId );
    if( item.empty() ) {
        throw CoreD::ItemNotFoundError{ itemId };
    }

    auto typeStr = json_get_str( item, Model::Resources::ItemFields::TypeFieldName );

    return checkValueType( std::move( item ), typeStr, checkPred );
}

JsonObject CoreDRemoteInterface::checkSettingValueType( const DeviceSettingId& settingId, CheckPred checkPred ) const {
    using namespace CoreD::Model::Resources;
    auto setting = getDeviceSetting( settingId );
    if( setting.empty() ) {
        throw CoreD::DeviceSettingNotFoundError{ settingId };
    }

    auto typeStr = json_get_str( setting, Model::Resources::DeviceSettingFields::ValueType );

    return checkValueType( std::move( setting ), typeStr, checkPred );
}

bool CoreDRemoteInterface::canAddElementToDictionary( const ItemID& itemId ) {
    const auto item = getItem( itemId );
    if( item.empty() )
        throw CoreD::ItemNotFoundError{ itemId };

    if( !Model::ValueType::fromString( json_get_str( item, Model::Resources::ItemFields::TypeFieldName ) )
             .isDictionary() ) {
        return false;
    }

    const auto& joCurrentValue = item[ Model::Resources::ItemFields::ValueFieldName ];
    auto dictionaryValue = Model::DictionaryValue::createValue(
        Model::ValueType::fromString( json_get_str( item, Model::Resources::ItemFields::TypeFieldName ) ),
        joCurrentValue );

    auto maxDictionaryElements = json_get_int( item, Model::Resources::ItemFields::ElementsMaxNumberFieldName, -1 );
    if( maxDictionaryElements == -1 ) {
        maxDictionaryElements = Model::DictionaryValue::m_maxElementsAmountDefault;
    }

    if( dictionaryValue->getValuesCount() + 1 > maxDictionaryElements ) {
        return false;
    }

    return true;
}

void CoreDRemoteInterface::checkBusyDevice( const DeviceID& deviceId ) const {
    using namespace CoreD::Model::Resources;

    const JsonObject device = findDevice( JSON( DeviceFields::IdFieldName << deviceId ) );
    if( device.empty() ) {
        throw DeviceNotFoundError{ deviceId };
    }

    if( !json_get_bool( device, DeviceFields::ReadyFieldName ) ) {
        throw DeviceBusy{ deviceId };
    }
}

std::function< bool( Model::ValueType valueType ) > CoreDRemoteInterface::getValueChecker( const JsonObject& data ) {
    const auto requestScaleIt = data.find( CoreD::Model::Resources::ScalableFields::Scale );
    std::string scale;
    if( requestScaleIt != data.cend() )
        scale = json_get_str( *requestScaleIt );
    SameItemValueTypesChecker sameItemValueTypesChecker;

    return [sameItemValueTypesChecker, scale]( CoreD::Model::ValueType valueType ) mutable -> bool {
        checkScaleValid( scale, valueType );
        return sameItemValueTypesChecker( valueType );
    };
}

std::set< uint8_t > CoreDRemoteInterface::convertWeekdays( const std::vector< std::string >& weekDays ) const {
    std::set< uint8_t > result;
    static std::map< const std::string, uint8_t > mappings{ { "monday", 1 },   { "tuesday", 2 }, { "wednesday", 3 },
                                                            { "thursday", 4 }, { "friday", 5 },  { "saturday", 6 },
                                                            { "sunday", 7 } };

    for( const auto& weekDay : weekDays ) {
        auto find_it = mappings.find( weekDay );
        if( find_it != mappings.end() ) {
            result.insert( find_it->second );
        }
    }

    return result;
}

GregorianCalendar::Date CoreDRemoteInterface::getDateWithModifiedTime( const GregorianCalendar::Date& date,
                                                                       const std::string& time ) const {
    auto tm = TimeUtils::stringToTimeStruct( time, "%H:%M:%S" );

    GregorianCalendar::Date result( date );
    result.setHour( tm.tm_hour );
    result.setMinute( tm.tm_min );
    result.setSecond( tm.tm_sec );
    result.setMillisecond( 0 );

    return result;
}

std::vector< Model::Feature > CoreDRemoteInterface::getFeatures() const {
    using namespace CoreD::Model::Resources;
    std::vector< Model::Feature > result;
    auto jsons = getDBClient().findAll( DBNames::FeaturesDB, JSON_EMPTY_OBJ, nullptr );
    result.reserve( jsons.size() );
    for( auto& json : jsons ) {
        result.emplace_back( json );
    }
    return result;
}

std::unique_ptr< Model::Feature > CoreDRemoteInterface::findFeature( const std::string& feature ) const {
    using namespace CoreD::Model::Resources;
    auto json = getDBClient().findOne( DBNames::FeaturesDB, JSON( db::ID_FIELD_NAME << feature ), nullptr );
    if( !json.empty() ) {
        return std::make_unique< Model::Feature >( json );
    }

    return nullptr;
}

bool CoreDRemoteInterface::checkFeatureStatus( const std::string& featureName ) const {
    const auto feature = findFeature( featureName );
    return feature && feature->getStatus();
}

void CoreDRemoteInterface::setFeatureStatus( const Model::Feature& feature ) {
    m_hostService.sendMethodCall( GroupsNames::CORED,
                                  MessageSubClasses::SET_FEATURE_STATUS,
                                  JSON( "feature" << feature.getName() << "status" << feature.getStatus() ) );
}

std::vector< Hub::Setting > CoreDRemoteInterface::getSettings() const {
    using Hub::SettingsReader;
    SettingsReader reader( getDBClient() );
    std::vector< Hub::Setting > settings;
    reader.getSettings( [&settings]( Hub::Setting&& setting ) { settings.push_back( setting ); } );
    return settings;
}

std::unique_ptr< Hub::Setting > CoreDRemoteInterface::findSetting( const std::string& setting ) const {
    Hub::SettingsReader reader( getDBClient() );
    return reader.findSetting( setting );
}

void CoreDRemoteInterface::saveSetting( const Hub::Setting& setting ) {
    m_hostService.sendMethodCall( GroupsNames::CORED,
                                  MessageSubClasses::SET_HUB_SETTING_VALUE,
                                  JSON( "name" << setting.getName() << "value" << setting.getValue() ) );
}

void CoreDRemoteInterface::reloadGlobalLogOptions() {
    m_hostService.sendMethodCall( GroupsNames::CORED, MessageSubClasses::RELOAD_GLOBAL_LOG_OPTIONS, JSON_EMPTY_OBJ );
}

void CoreDRemoteInterface::addVeraDeviceMapping( const int veraDeviceId, const Json& deviceData ) {
    auto params = JSON( Model::Resources::RPCParams::VeraDeviceId
                        << veraDeviceId << Model::Resources::RPCParams::DeviceData << deviceData );

    m_hostService.sendRequest( GroupsNames::CORED, params, MessageSubClasses::ADD_VERA_DEVICE_MAPPING, m_busTimeout );
}

JsonObject CoreDRemoteInterface::houseModesProtectButtonsSet(
    const CoreDInterface::DeviceID& deviceId,
    const CoreDHouseModes::ProtectServiceEnum& protectService ) {
    const MessageSubClasses subclass = HOUSE_MODE_PROTECT_BUTTONS_SET;

    JsonObject params = { { "deviceId", deviceId }, { "service", toString( protectService ) } };
    return m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
}

std::vector< CoreDHouseModes::ProtectButton > CoreDRemoteInterface::getHouseModesProtectButtons() {
    return CoreDHouseModes::getProtectButtons( getDBClient() );
}

JsonObject CoreDRemoteInterface::houseModesProtectDevicesAdd( const CoreDInterface::DeviceIDs& deviceIds ) {
    const MessageSubClasses subclass = HOUSE_MODES_PROTECT_DEVICES_ADD;
    LOGGER_DEBUG_RAW( "RemoteInterface::houseModesProtectDevicesAdd, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "deviceIds" << deviceIds );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

JsonObject CoreDRemoteInterface::houseModesProtectDevicesRemove( const CoreDInterface::DeviceIDs& deviceIds ) {
    const MessageSubClasses subclass = HOUSE_MODES_PROTECT_DEVICES_REMOVE;
    LOGGER_DEBUG_RAW( "RemoteInterface::houseModesProtectDevicesRemove, send request with subclass " +
                      std::to_string( static_cast< int >( subclass ) ) );

    JsonObject params = JSON( "deviceIds" << deviceIds );
    auto response = m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
    return response;
}

CoreDInterface::DeviceIDs CoreDRemoteInterface::getHouseModesProtectDevicesIDs() {
    return CoreDHouseModes::getProtectDevicesIds( getDBClient(), JSON_EMPTY_OBJ );
}

CoreDInterface::DeviceIDs CoreDRemoteInterface::getCameraDeviceIDs() {
    CoreDInterface::DeviceIDs deviceIds;
    auto results = CoreDHouseModes::getCamerasDeviceItemPairs( getDBClient(), JSON_EMPTY_OBJ );
    if( !results.empty() ) {
        for( const auto& pair : results ) {
            deviceIds.push_back( pair.first );
        }
    }
    return deviceIds;
}

void CoreDRemoteInterface::setBusTimeout( std::chrono::seconds timeoutValue ) {
    m_busTimeout = timeoutValue;
}

std::chrono::seconds CoreDRemoteInterface::getBusTimeout() const {
    return m_busTimeout;
}

/* @brief Checks validity of a "source" object for the setItemValue call: "type" field is required, should be a string.
 * source = { type: string; ... }
 * */
JsonObject CoreDRemoteInterface::checkValueSource( JsonObject object ) const {
    if( object.empty() )
        return object;
    auto sourceIt = object.find( CoreD::Model::Resources::RPCParams::SourceField );
    if( sourceIt == object.end() )
        return object;

    auto typeIt = sourceIt->find( "type" );
    if( typeIt != sourceIt->end() ) {
        if( !typeIt->is_string() )
            throw CoreD::InvalidValueSourceTypeError( "not a string" );
        else if( typeIt->empty() )
            throw CoreD::InvalidValueSourceTypeError();
    }
    else {
        throw CoreD::InvalidValueSourceTypeError();
    }
    return object;
}

std::unique_ptr< GroupOperationInterface > CoreDRemoteInterface::groupOperations() {
    return std::make_unique< GroupOperationInterface >( m_hostService );
}

std::unique_ptr< PolicyOperationInterface > CoreDRemoteInterface::policyOperations() {
    return std::make_unique< PolicyOperationInterface >( m_hostService );
}

std::unique_ptr< SwingerShutdown::SwingerShutdownInterface > CoreDRemoteInterface::swingerShutdown() {
    return std::make_unique< SwingerShutdown::SwingerShutdownInterface >( m_hostService );
}

std::unique_ptr< WalkTest::WalkTestInterface > CoreDRemoteInterface::walkTest() {
    return std::make_unique< WalkTest::WalkTestInterface >( m_hostService );
}

bool CoreDRemoteInterface::isFirmwareVersionLess( const std::string& targetVersion ) {
    auto sourceVersion = FirmwareInfoHelper::getFirmwareVersionString();
    return isFirmwareVersionOlder( targetVersion, sourceVersion );
}

bool CoreDRemoteInterface::isFirmwareVersionOlder( const std::string& targetVersion,
                                                   const std::string& sourceVersion ) {
    auto extractVersionComponents = []( const std::string& version ) -> std::vector< std::string > {
        std::vector< std::string > components;
        std::istringstream stream( version );
        std::string component;

        while( getline( stream, component, '.' ) ) {
            components.push_back( component );
        }
        return components;
    };
    auto sourceVersionComponents = extractVersionComponents( sourceVersion );
    auto targetVersionComponents = extractVersionComponents( targetVersion );
    int counter = sourceVersionComponents.size() <= targetVersionComponents.size() ? sourceVersionComponents.size() :
                                                                                     targetVersionComponents.size();
    try {
        for( int i = 0; i < counter; i++ ) {
            if( i == 3 ) {
                if( sourceVersionComponents[ i ].size() == 1 && std::stoi( sourceVersionComponents[ i ] ) == 0 ) {
                    return false;
                }
                else if( sourceVersionComponents[ i ].size() != 1 && targetVersionComponents[ i ].size() == 1 &&
                         std::stoi( targetVersionComponents[ i ] ) == 0 ) {
                    return true;
                }
                for( int j = 0; j < sourceVersionComponents[ i ].size(); j++ ) {
                    if( sourceVersionComponents[ i ][ j ] < targetVersionComponents[ i ][ j ] ) {
                        return true;
                    }
                    else if( sourceVersionComponents[ i ][ j ] > targetVersionComponents[ i ][ j ] ) {
                        return false;
                    }
                }
            }
            else if( std::stoi( sourceVersionComponents[ i ] ) < std::stoi( targetVersionComponents[ i ] ) ) {
                return true;
            }
        }
    }
    catch( const std::invalid_argument& ex ) {
        LOGGER_ERROR( cc::error( "Version component is not a valid integer: " ) + ex.what() );
    }
    catch( const std::out_of_range& ex ) {
        LOGGER_ERROR( cc::error( "Version component is out of range: " ) + ex.what() );
    }

    return false;
}

JsonObject CoreDRemoteInterface::turnOffLocalAlarm( const JsonObject& params ) {
    const MessageSubClasses subclass = HOUSE_MODES_LOCAL_ALARM_OFF;
    LOGGER_INFO_RAW( cc::info( "house mode local alarm off" ) );

    return m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
}

JsonObject CoreDRemoteInterface::forceDisarm( const JsonObject& params ) {
    const MessageSubClasses subclass = HOUSE_MODES_FORCE_DISARM;
    LOGGER_INFO_RAW( cc::info( "house mode force disarm" ) );

    return m_hostService.sendRequest( GroupsNames::CORED, params, subclass );
}

std::unique_ptr< Protect::ProtectContactIdInterface > CoreDRemoteInterface::contactIdOperations() {
    return std::make_unique< Protect::ProtectContactIdInterface >( m_hostService );
}

std::unique_ptr< Protect::ProtectSecurityConfigInterface > CoreDRemoteInterface::securityConfigOperations() {
    return std::make_unique< Protect::ProtectSecurityConfigInterface >( m_hostService );
}

}  // namespace CoreDRemoteInterface
}  // namespace HubPlatform
