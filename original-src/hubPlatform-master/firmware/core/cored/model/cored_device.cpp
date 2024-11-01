#include "cored/model/cored_device.h"
#include "cored/model/resources/cored_resources.h"
#include "json_utils/json_utils.h"

namespace HubPlatform {
namespace CoreD {
namespace Model {

namespace {
const std::set< std::string > DeviceStatus = { "idle", "broken", "rediscovering", "updating", "locked" };
}

using namespace Resources::DeviceFields;

Device::Device( const std::string& gatewayId,
                const std::string& name,
                const std::string& type,
                const std::string& deviceTypeId,
                const std::string& category,
                bool batteryPowered,
                const std::string& protectConfig )
: m_name( name )
, m_type( type )
, m_deviceTypeId( deviceTypeId )
, m_category( category )
, m_gatewayId( gatewayId )
, m_batteryPowered( batteryPowered )
, m_reachable( true )
, m_security( Security::No )
, m_status( "idle" )
, m_ready( true )
, m_serviceNotification( false )
, m_persistent( false )
, m_parentRoom( true )
, m_protectConfig( protectConfig ) {}

const std::string& Device::getGatewayId() const {
    return m_gatewayId;
}

Device& Device::setAdditionalInfo( std::vector< AdditionalInfo >&& additionalInfo ) {
    m_additionalInfo = std::move( additionalInfo );
    return *this;
}

Device& Device::setFirmware( std::vector< FirmwareInfo >&& firmware ) {
    m_firmware = std::move( firmware );
    return *this;
}

Device& Device::setReachable( bool reachable ) {
    m_reachable = reachable;
    return *this;
}

Device& Device::setParentRoom( bool parentRoom ) {
    m_parentRoom = parentRoom;
    return *this;
}

Device& Device::setParentDeviceId( const std::string& parentDeviceId ) {
    m_parentDeviceId = parentDeviceId;
    return *this;
}

Device& Device::setSubcategory( const std::string& subcategory ) {
    m_subcategory = subcategory;
    return *this;
}

Device& Device::setRoomId( const std::string& roomId ) {
    m_roomId = roomId;
    return *this;
}

bool Device::setSecurity( const std::string& security ) {
    if( security.empty() ) {
        m_security = Security::No;
        return true;
    }
    if( Security::PossibleValues.find( security ) == Security::PossibleValues.end() ) {
        return false;
    }
    m_security = security;
    return true;
}

bool Device::setStatus( const std::string& status ) {
    if( !DeviceStatus.count( status ) ) {
        LOGGER_WARN( "wrong status '" + status + "'" );
        return false;
    }

    m_status = status;
    return true;
}

Device& Device::setReady( bool enableReady ) {
    m_ready = enableReady;
    return *this;
}

Device& Device::setServiceNotification( bool enable ) {
    m_serviceNotification = enable;
    return *this;
}

Device& Device::setPersistent( bool value ) {
    m_persistent = value;
    return *this;
}

Json Device::toJson() const {
    auto device = JSON_EMPTY_OBJ;

    device[ GatewayIdFieldName ] = m_gatewayId;
    device[ TypeFieldName ] = m_type;
    device[ DeviceTypeIdFieldName ] = m_deviceTypeId;
    device[ CategoryFieldName ] = m_category;
    device[ SubcategoryFieldName ] = m_subcategory;
    device[ ParentDeviceIdFieldName ] = m_parentDeviceId;
    device[ RoomIdFieldName ] = m_roomId;

    auto joInfo = JSON_EMPTY_OBJ;
    if( !m_additionalInfo.empty() ) {
        for( const auto& info : m_additionalInfo ) {
            joInfo[ info.m_label ] = info.m_value;
        }
    }
    device[ InfoFieldName ] = std::move( joInfo );

    if( !m_firmware.empty() ) {
        auto firmwareJson = Json::array();
        for( const auto& firmwareInfo : m_firmware ) {
            firmwareJson.push_back(
                JSON( FirmwareIdFieldName << firmwareInfo.id << FirmwareVersionFieldName << firmwareInfo.version ) );
        }
        device[ FirmwareFieldName ] = std::move( firmwareJson );
    }

    device[ BatteryPoweredFieldName ] = m_batteryPowered;
    device[ ReachableFieldName ] = m_reachable;
    device[ NameFieldName ] = m_name;
    device[ SecurityFieldName ] = m_security;
    device[ StatusFieldName ] = m_status;
    device[ ReadyFieldName ] = m_ready;
    device[ ServiceNotificationFieldName ] = m_serviceNotification;
    device[ PersistentFieldName ] = m_persistent;
    device[ HouseModesOptionsFieldName ] = m_houseModesOptions;
    device[ ProtectConfig ] = m_protectConfig;

    if( !m_parentDeviceId.empty() ) {
        device[ ParentRoomFieldName ] = m_parentRoom;
    }

    return device;
}

void Device::checkDeviceAsJson( const Json& device, bool isGetDeviceListCall ) {
    auto jsonCheckAdditionalInfo = []( const Json& dev ) {
        Json jsonInfo;

        if( !json_get( dev, InfoFieldName, jsonInfo ) || !jsonInfo.is_object() ) {
            throw std::runtime_error{ std::string{ "fail to fetch an object at '" } + InfoFieldName + "' key" };
        }

        if( jsonInfo.size() > m_maxAdditionalInfoSize ) {
            throw std::runtime_error{ std::string{ "object at '" } + InfoFieldName +
                                      "' key contains too much elements" };
        }

        for( auto it = jsonInfo.begin(); it != jsonInfo.end(); ++it ) {
            const auto& joValue = it.value();
            std::string value = joValue.is_string() ? joValue.get< std::string >() : std::string{};

            try {
                jsonCheckString( it.key() );
                jsonCheckString( value );
            }
            catch( ... ) {
                throw std::runtime_error{ std::string{ "object at '" } + InfoFieldName +
                                          "' key: key and/or value aren't strings" };
            }
        }
    };

    /* check mandatory fields */
    jsonCheckString( device, TypeFieldName );
    jsonCheckString( device, DeviceTypeIdFieldName );
    jsonCheckString( device, GatewayIdFieldName );
    jsonCheckString( device, CategoryFieldName );
    jsonCheckString( device, SubcategoryFieldName, JsonStringChecks::RESTRICTED_LENGTH );
    jsonCheckBool( device, BatteryPoweredFieldName );
    jsonCheckBool( device, ReachableFieldName );
    jsonCheckString( device, NameFieldName );
    jsonCheckString( device, ParentDeviceIdFieldName, JsonStringChecks::RESTRICTED_LENGTH );
    jsonCheckString( device, RoomIdFieldName, JsonStringChecks::RESTRICTED_LENGTH );
    jsonCheckString( device, StatusFieldName );
    jsonCheckAdditionalInfo( device );

    /* check optional fields */
    int optionalFieldsCounter = 0;

    if( device.count( IdFieldName ) ) {
        jsonCheckString( device, IdFieldName );
        optionalFieldsCounter++;
    }

    if( device.count( ArmedFieldName ) ) {
        jsonCheckBool( device, ArmedFieldName );
        optionalFieldsCounter++;
    }

    if( device.count( PersistentFieldName ) ) {
        jsonCheckBool( device, PersistentFieldName );
        optionalFieldsCounter++;
    }

    std::string parentId;
    if( !isGetDeviceListCall ) {
        if( json_get( device, ParentDeviceIdFieldName, parentId ) && !parentId.empty() ) {
            jsonCheckBool( device, ParentRoomFieldName, true );
            optionalFieldsCounter++;
        }
        else {
            jsonCheckBool( device, ParentRoomFieldName, false );
        }
    }

    if( device.count( HouseModesOptionsFieldName ) ) {
        const auto houseModesOptions = device[ HouseModesOptionsFieldName ];

        auto checkHouseModesOption = [&houseModesOptions]( const std::string& option ) -> void {
            // optional array of strings, identifiers of existing houseModes
            auto optionArrayIt = houseModesOptions.find( option );
            if( optionArrayIt != houseModesOptions.end() && optionArrayIt->is_array() && !optionArrayIt->empty() ) {
                for( const auto& item : *optionArrayIt ) {
                    if( !item.is_string() )
                        throw std::runtime_error( std::string{ "house_modes_options." }
                                                      .append( option )
                                                      .append( " item is not a string: " )
                                                      .append( item.dump() ) );
                    jsonCheckString( item.get< std::string >(), JsonStringChecks::NON_EMPTY );
                }
            }
        };

        checkHouseModesOption( HouseModesOptions::DisarmedFieldName );
        checkHouseModesOption( HouseModesOptions::AlarmsOffFieldName );

        optionalFieldsCounter++;
    }

    /* check if some 'trash' got into the json */
    if( device.size() > m_minFieldsAmount + optionalFieldsCounter ) {
        throw std::runtime_error{ "needless fields" };
    }
}

Device& Device::setHouseModesOptions( Json options ) {
    m_houseModesOptions = std::move( options );
    return *this;
}

}  // namespace Model
}  // namespace CoreD
}  // namespace HubPlatform
