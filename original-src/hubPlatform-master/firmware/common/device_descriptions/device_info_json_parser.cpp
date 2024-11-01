
#include "device_descriptions/device_info_json_parser.h"
#include "device_descriptions/device_info.h"

#include "log_utils/ezlo_log.h"

#include <fstream>

DeviceDescriptionsContainer DeviceInfoJsonParser::load( const FilenameType& fileName ) {
    auto strLogJsonFileName = cc::p( fileName );
    auto strLogMessage =
        cc::debug( "Loading JSON device info descriptions from " ) + strLogJsonFileName + cc::debug( "..." );
    Log::trace( strLogMessage );
    std::ifstream file( fileName );
    DeviceDescriptionsContainer container;

    if( file ) {
        JsonObject json;
        file >> json;
        try {
            auto manufacturers = parseManufacturers( json );
            auto types = parseTypes( json );
            auto descriptions = parseDescriptions( json );
            auto itemSubTypes = parseItemSubTypes( json );
            auto deviceSubTypes = parseDeviceSubTypes( json );
            auto devicesShowAsRuleExcludedItems = parseDevicesShowAsRuleExcludedItems( json );

            container.setDeviceManufacturers( std::move( manufacturers ) );
            container.setDeviceTypes( std::move( types ) );
            container.setDeviceDescriptions( std::move( descriptions ) );
            container.setDeviceItemSubtypes( std::move( itemSubTypes ) );
            container.setDeviceSubtypes( std::move( deviceSubTypes ) );
            container.setDevicesShowAsRuleExcludedItems( std::move( devicesShowAsRuleExcludedItems ) );
            strLogMessage = cc::success( "Success" ) + cc::debug( ", loaded JSON device info descriptions from " ) +
                            strLogJsonFileName + cc::debug( ": " ) + cc::j( json );
            Log::trace( strLogMessage );
        }
        catch( const std::exception& e ) {
            strLogMessage = cc::fatal( "FAIL" ) + cc::error( ", error loading JSON device info descriptions from " ) +
                            strLogJsonFileName + cc::error( ": " ) + cc::warn( e.what() );
            Log::error( strLogMessage );
            container.clear();
        }
        catch( ... ) {
            strLogMessage = cc::fatal( "FAIL" ) +
                            cc::error( ", unknown error loading JSON device info descriptions from " ) +
                            strLogJsonFileName;
            Log::error( strLogMessage );
            container.clear();
        }
    }

    return container;
}

ManufacturersNames DeviceInfoJsonParser::parseManufacturers( const JsonObject& rootObject ) {
    return parseIdNameNodes( "manufacturers", rootObject );
}

DeviceTypesList DeviceInfoJsonParser::parseTypes( const JsonObject& rootObject ) {
    return parseIdNameNodes( "types", rootObject );
}

DevicesShowAsRuleExcludedItems DeviceInfoJsonParser::parseDevicesShowAsRuleExcludedItems(
    const JsonObject& rootObject ) {
    JsonObject extra_fields;

    json_get_obj( rootObject, "extra_fields", extra_fields );

    DevicesShowAsRuleExcludedItems result{};

    if( !rootObject.empty() ) {
        for( auto iter = extra_fields.begin(); iter != extra_fields.end(); ++iter ) {
            auto& showAsRuleExcludedItems = iter.value()[ "showAsRuleExcludedItems" ];

            if( !showAsRuleExcludedItems.empty() ) {
                for( auto& excluded_item : showAsRuleExcludedItems ) {
                    result[ iter.key() ].push_back( excluded_item );
                }
            }
            else
                result[ iter.key() ] = {};
        }
    }

    return result;
}

DeviceDescriptions DeviceInfoJsonParser::parseDescriptions( const JsonObject& rootObject ) {
    auto descriptions = DeviceDescriptions();

    auto manufacturerID = DeviceId::ManufacturersEnumType();
    auto productType = int();
    auto deviceID = int();
    auto devName = std::string();
    auto devModel = std::string();
    auto devType = int();
    auto descriptionsNode = JSON_EMPTY_ARR;
    auto itemSubType = int();
    auto deviceSubType = int();
    auto mainItemName = std::string();
    auto hasExtraFields = bool();
    auto hasGetter = bool();
    auto hasSetter = bool();
    auto showAsRuleExcludedItems = JSON_EMPTY_ARR;

    if( getJsonArray( "devices", rootObject, descriptionsNode ) ) {
        for( const auto& descriptionNode : descriptionsNode ) {
            json_get_checked( descriptionNode, "manufacturerId", manufacturerID );
            json_get_checked( descriptionNode, "productType", productType );
            json_get_checked( descriptionNode, "deviceId", deviceID );
            json_get_checked( descriptionNode, "name", devName );
            json_get_checked( descriptionNode, "model", devModel );
            json_get_checked( descriptionNode, "device_subtype", deviceSubType );
            json_get_checked( descriptionNode, "type", devType );
            json_get_checked( descriptionNode, "item_subtype", itemSubType );
            json_get_optional( descriptionNode, "mainItemName", mainItemName, std::string() );
            json_get_optional( descriptionNode, "hasExtraFields", hasExtraFields, false );

            json_get_optional( descriptionNode, "hasGetter", hasGetter, false );
            json_get_optional( descriptionNode, "hasSetter", hasSetter, false );
            json_get_optional( descriptionNode, "showAsRuleExcludedItems", showAsRuleExcludedItems, JSON_EMPTY_ARR );

            DeviceId deviceId{ manufacturerID, productType, deviceID };
            auto deviceInfo = std::make_shared< DeviceInfo >( devName,
                                                              devModel,
                                                              devType,
                                                              deviceSubType,
                                                              DeviceInfo::ItemsSubTypesList{ { 0, itemSubType } },
                                                              std::move( mainItemName ),
                                                              DeviceConfigurations(),
                                                              hasExtraFields,
                                                              hasGetter,
                                                              hasSetter,
                                                              showAsRuleExcludedItems );

            auto configurations = parseConfigurationsOptional( descriptionNode );
            if( !configurations.empty() ) {
                Log::trace( "Configurations for device parsed" );
                deviceInfo->setConfigurations( std::move( configurations ) );
            }

            descriptions.emplace( std::move( deviceId ), std::move( deviceInfo ) );
        }
    }

    return descriptions;
}

DeviceConfigurations DeviceInfoJsonParser::parseConfigurationsOptional( const JsonObject& rootObject ) {
    DeviceConfigurations deviceConfigurations;
    auto channelId = int();
    auto paramNumber = int();
    auto paramSize = int();
    auto paramValue = int();
    auto paramDescription = std::string();
    auto setToDefault = int();
    auto configurationsNode = JSON_EMPTY_ARR;

    if( getJsonArray( "configurations", rootObject, configurationsNode ) ) {
        for( const auto& configuration : configurationsNode ) {
            json_get_checked( configuration, "channelId", channelId );
            json_get_checked( configuration, "paramNumber", paramNumber );
            json_get_checked( configuration, "paramSize", paramSize );
            json_get_checked( configuration, "paramValue", paramValue );
            json_get_checked( configuration, "setToDefault", setToDefault );
            json_get_checked( configuration, "paramDescription", paramDescription );

            DeviceConfiguration deviceConfiguration(
                channelId, paramNumber, paramSize, paramValue, setToDefault, paramDescription );

            deviceConfigurations.emplace_back( std::move( deviceConfiguration ) );
        }
    }

    return deviceConfigurations;
}

ItemSubTypesList DeviceInfoJsonParser::parseItemSubTypes( const JsonObject& rootObject ) {
    return parseIdNameNodes( "item_subtypes", rootObject );
}

DeviceSubTypesList DeviceInfoJsonParser::parseDeviceSubTypes( const JsonObject& rootObject ) {
    return parseIdNameNodes( "device_subtypes", rootObject );
}

DeviceInfoCommonList DeviceInfoJsonParser::parseIdNameNodes( const std::string& nodeName,
                                                             const JsonObject& rootObject ) {
    auto deviceInfoList = DeviceInfoCommonList();
    auto id = int();
    auto name = std::string();
    auto infoNode = JSON_EMPTY_ARR;

    if( getJsonArray( nodeName, rootObject, infoNode ) ) {
        for( const auto& currentInfo : infoNode ) {
            json_get_checked( currentInfo, "id", id );
            json_get_checked( currentInfo, "name", name );
            deviceInfoList.emplace( id, std::move( name ) );
        }
    }

    return deviceInfoList;
}

template < typename T >
void DeviceInfoJsonParser::json_get_checked( const Json& vals, const std::string& key, T& res ) {
    if( !json_get( vals, key, res ) ) {
        throw std::logic_error( "failed to json file, key=\"" + key + "\", JSON: " + vals.dump() );
    }
}

// TODO move void() implementation to T() implementation in all dependencies
template < typename T >
void DeviceInfoJsonParser::json_get_optional( const Json& vals, const std::string& key, T& res, T&& defaultValue ) {
    if( !json_get( vals, key, res ) ) {
        res = std::move( defaultValue );
    }
}

template < typename T >
T DeviceInfoJsonParser::json_get_optional( const Json& vals, const std::string& key, T&& defaultValue ) {
    T result( std::move( defaultValue ) );
    json_get( vals, key, result );
    return result;
}
