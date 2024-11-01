#include "zwave/provisioning/provisioning_list_utils.h"

#include "jsonrpc/jsonrpc.h"
#include "json_utils/json_utils.h"

#include <znet/znet.h>

namespace HubPlatform {
namespace Zwave {

Json ProvisioningListUtils::idFromJson( const Json& json, uint32_t& id ) {
    Json result;

    std::string idStr;
    if( !json_get( json, "_id", idStr ) ) {
        Errors::getBadRequest( "_id", result );
        return result;
    }

    if( !idStr.empty() ) {
        auto provisioningId = idFromString( idStr );
        if( !provisioningId ) {
            Errors::getInvalidValueRange( "_id", result, "" );
            return result;
        }
        id = *provisioningId;
    }

    return result;
}

void ProvisioningListUtils::idToJson( uint32_t id, Json& json ) {
    json[ "_id" ] = idToString( id );
}

Json ProvisioningListUtils::dskFromJson( const Json& json, znet::DSK& dsk, bool dbFormat ) {
    Json result;

    if( !json.count( "dsk" ) ) {
        Errors::getBadRequest( "dsk", result );
        return result;
    }

    if( dbFormat ) {
        dskFromString( json_get_str( json, "dsk" ), dsk );
    }
    else {
        auto vals = json_get_vec< uint16_t >( json, "dsk" );
        if( vals.size() != 8 ) {
            Errors::getInvalidValueRange( "dsk", result, "" );
            return result;
        }
        std::copy( vals.begin(), vals.end(), dsk.begin() );
    }

    return result;
}

void ProvisioningListUtils::dskToJson( const znet::DSK& dsk, Json& json, bool dbFormat ) {
    if( dbFormat ) {
        json[ "dsk" ] = dskToString( dsk );
    }
    else {
        json[ "dsk" ] = dsk;
    }
}

Json ProvisioningListUtils::entryFieldsFromJson( const Json& json,
                                                 znet::ProvisioningListEntryFields& fields,
                                                 bool strict ) {
    Json result;

    if( strict ) {
        if( !json.count( "name" ) ) {
            Errors::getBadRequest( "name", result );
            return result;
        }

        if( !json.count( "addingMode" ) ) {
            Errors::getBadRequest( "addingMode", result );
            return result;
        }
    }

    if( json.count( "supportedAddingModes" ) ) {
        for( const auto& mode : json[ "supportedAddingModes" ] ) {
            auto modeEnum = addingModeFromString( mode );
            if( strict && !modeEnum ) {
                Errors::getInvalidValueRange( "supportedAddingModes", result, "" );
                return result;
            }

            fields.m_supportedAddingModes.push_back( *modeEnum );
        }
    }

    if( json.count( "addingMode" ) ) {
        fields.m_addingMode = addingModeFromString( json_get_str( json, "addingMode" ) );
        if( strict && !fields.m_addingMode ) {
            Errors::getInvalidValueRange( "addingMode", result, "" );
            return result;
        }
    }

    if( json.count( "inclusionSetting" ) ) {
        fields.m_inclusionSetting = inclusionSettingFromString( json_get_str( json, "inclusionSetting" ) );
    }

    if( json.count( "product" ) ) {
        if( json[ "product" ].is_null() ) {
            fields.m_product.reset();
        }
        else {
            if( !fields.m_product ) {
                fields.m_product = znet::ProvisioningListEntryFields::Product{ 0, 0, 0, 0, 0 };
            }

            if( !json_get( json[ "product" ], "manufacturer", fields.m_product->m_manufacturerId ) ) {
                Errors::getBadRequest( "product.manufacturer", result );
                return result;
            }

            if( !json_get( json[ "product" ], "type", fields.m_product->m_productType ) ) {
                Errors::getBadRequest( "product.type", result );
                return result;
            }

            if( !json_get( json[ "product" ], "id", fields.m_product->m_productId ) ) {
                Errors::getBadRequest( "product.id", result );
                return result;
            }

            if( !json_get( json[ "product" ], "version", fields.m_product->m_version ) ) {
                Errors::getBadRequest( "product.version", result );
                return result;
            }

            if( !json_get( json[ "product" ], "subVersion", fields.m_product->m_subversion ) ) {
                Errors::getBadRequest( "product.subVersion", result );
                return result;
            }
        }
    }

    if( json.count( "type" ) ) {
        if( json[ "type" ].is_null() ) {
            fields.m_type.reset();
        }
        else {
            if( !fields.m_type ) {
                fields.m_type = znet::ProvisioningListEntryFields::Type{ 0, 0, 0 };
            }

            if( !json_get( json[ "type" ], "generic", fields.m_type->m_generic ) ) {
                Errors::getBadRequest( "type.generic", result );
                return result;
            }

            if( !json_get( json[ "type" ], "specific", fields.m_type->m_specific ) ) {
                Errors::getBadRequest( "type.specific", result );
                return result;
            }

            if( !json_get( json[ "type" ], "icon", fields.m_type->m_icon ) ) {
                Errors::getBadRequest( "type.icon", result );
                return result;
            }
        }
    }

    if( json.count( "maxRequestInterval" ) && json[ "maxRequestInterval" ].is_null() ) {
        fields.m_maxRequestInterval.reset();
    }
    else {
        int maxRequestInterval;
        if( json_get( json, "maxRequestInterval", maxRequestInterval ) )
            fields.m_maxRequestInterval = maxRequestInterval;
    }

    if( json.count( "uuidInformation" ) ) {
        if( json[ "uuidInformation" ].is_null() ) {
            fields.m_uuidInformation.reset();
        }
        else {
            if( !fields.m_uuidInformation ) {
                fields.m_uuidInformation =
                    znet::ProvisioningListEntryFields::UuidInformation{ 0, std::array< uint8_t, 16 >() };
            }

            if( !json_get( json[ "uuidInformation" ],
                           "presentationFormat",
                           fields.m_uuidInformation->m_presentationFormat ) ) {
                Errors::getBadRequest( "uuidInformation.presentationFormat", result );
                return result;
            }

            if( !json[ "uuidInformation" ].count( "uuid" ) ) {
                Errors::getBadRequest( "uuidInformation.uuid", result );
                return result;
            }
            else if( json[ "uuidInformation" ][ "uuid" ].size() != 16 ) {
                Errors::getBadRequest( "uuidInformation.uuid", result );
                return result;
            }
            else {
                for( int i = 0; i < 16; ++i ) {
                    fields.m_uuidInformation->m_uuid[ i ] = json[ "uuidInformation" ][ "uuid" ][ i ];
                }
            }
        }
    }

    if( json.count( "uuid" ) && json[ "uuid" ].is_null() ) {
        fields.m_uuid.reset();
    }
    else {
        std::string uuid;
        if( json_get( json, "uuid", uuid ) )
            fields.m_uuid = uuid;
    }

    if( json.count( "name" ) && json[ "name" ].is_null() ) {
        Errors::getInvalidValueRange( "name", result, "" );
        return result;
    }
    else {
        std::string name;
        if( json_get( json, "name", name ) )
            fields.m_name = name;
    }

    if( json.count( "room" ) && json[ "room" ].is_null() ) {
        fields.m_room.reset();
    }
    else {
        std::string room;
        if( json_get( json, "room", room ) )
            fields.m_room = room;
    }

    if( json.count( "location" ) && json[ "location" ].is_null() ) {
        fields.m_location.reset();
    }
    else {
        std::string location;
        if( json_get( json, "location", location ) )
            fields.m_location = location;
    }

    auto securityModesIt = json.find( "securityModes" );
    if( securityModesIt != json.end() ) {
        if( securityModesIt->is_null() ) {
            fields.m_securityModes.reset();
        }
        else {
            uint8_t securityModes = 0;
            for( auto& mode : *securityModesIt ) {
                if( mode.is_string() )
                    securityModes |= securityModeFromString( mode.get< std::string >() );
            }
            fields.m_securityModes = securityModes;
        }
    }

    auto nodeSecurityModes = json.find( "nodeSecurityModes" );
    if( nodeSecurityModes != json.end() ) {
        if( nodeSecurityModes->is_null() ) {
            fields.m_nodeSecurityModes.reset();
        }
        else {
            uint8_t securityModes = 0;
            for( auto& mode : *nodeSecurityModes ) {
                if( mode.is_string() )
                    securityModes |= securityModeFromString( mode.get< std::string >() );
            }
            fields.m_nodeSecurityModes = securityModes;
        }
    }

    auto supportedModes = json.find( "supportedModes" );
    if( supportedModes != json.end() ) {
        if( supportedModes->is_null() ) {
            fields.m_supportedModes.reset();
        }
        else {
            uint8_t securityModes = 0;
            for( auto& mode : *supportedModes ) {
                if( mode.is_string() )
                    securityModes |= securityModeFromString( mode.get< std::string >() );
            }
            fields.m_supportedModes = securityModes;
        }
    }
    else {
        fields.m_supportedModes = ZNET_SECURITY2_S2_CLASS_0 | ZNET_SECURITY2_S2_CLASS_1 | ZNET_SECURITY2_S2_CLASS_2 |
                                  ZNET_SECURITY2_S0_NET_KEY;
    }

    return result;
}

void ProvisioningListUtils::entryFieldsToJson( const znet::ProvisioningListEntryFields& fields, Json& json ) {
    if( fields.m_addingMode )
        json[ "addingMode" ] = addingModeToString( *fields.m_addingMode );

    if( !fields.m_supportedAddingModes.empty() ) {
        JsonArray modes;
        for( const auto& mode : fields.m_supportedAddingModes ) {
            modes.push_back( addingModeToString( mode ) );
        }

        json[ "supportedAddingModes" ] = std::move( modes );
    }

    if( fields.m_inclusionSetting )
        json[ "inclusionSetting" ] = inclusionSettingToString( *fields.m_inclusionSetting );

    if( fields.m_product ) {
        Json product;
        product[ "manufacturer" ] = fields.m_product->m_manufacturerId;
        product[ "type" ] = fields.m_product->m_productType;
        product[ "id" ] = fields.m_product->m_productId;
        product[ "version" ] = fields.m_product->m_version;
        product[ "subVersion" ] = fields.m_product->m_subversion;
        json[ "product" ] = std::move( product );
    }

    if( fields.m_type ) {
        Json type;
        type[ "generic" ] = fields.m_type->m_generic;
        type[ "specific" ] = fields.m_type->m_specific;
        type[ "icon" ] = fields.m_type->m_icon;
        json[ "type" ] = std::move( type );
    }

    if( fields.m_maxRequestInterval )
        json[ "maxRequestInterval" ] = *fields.m_maxRequestInterval;

    if( fields.m_uuidInformation ) {
        Json info;
        info[ "presentationFormat" ] = fields.m_uuidInformation->m_presentationFormat;
        info[ "uuid" ] = fields.m_uuidInformation->m_uuid;
        json[ "uuidInformation" ] = std::move( info );
    }

    if( fields.m_uuid )
        json[ "uuid" ] = *fields.m_uuid;

    if( fields.m_name )
        json[ "name" ] = *fields.m_name;

    if( fields.m_room )
        json[ "room" ] = *fields.m_room;

    if( fields.m_location )
        json[ "location" ] = *fields.m_location;

    if( fields.m_securityModes ) {
        json[ "securityModes" ] = securityModeToJson( *fields.m_securityModes );
    }

    if( fields.m_nodeSecurityModes ) {
        json[ "nodeSecurityModes" ] = securityModeToJson( *fields.m_nodeSecurityModes );
    }

    if( fields.m_supportedModes ) {
        json[ "supportedModes" ] = securityModeToJson( *fields.m_supportedModes );
    }
}

void ProvisioningListUtils::entryFieldsToJsonUiFiltered( const znet::ProvisioningListEntryFields& fields, Json& json ) {
    entryFieldsToJson( fields, json );

    auto removeInternalField = []( Json& json, const std::string& fieldName ) {
        if( json.count( fieldName ) ) {
            json.erase( fieldName );
        }
    };

    removeInternalField( json, "inclusionSetting" );
    removeInternalField( json, "uuidInformation" );
    removeInternalField( json, "location" );
}

void ProvisioningListUtils::genEntryFieldsFromJson( const Json& json, znet::ProvisioningListEntryFields& fields ) {
    auto node = json_get_int( json, "node" );
    if( node != -1 )
        fields.m_node = node;

    fields.m_status = statusFromString( json_get_str( json, "status" ) );
}

void ProvisioningListUtils::genEntryFieldsToJson( const znet::ProvisioningListEntryFields& fields, Json& json ) {
    if( fields.m_node )
        json[ "node" ] = *fields.m_node;

    if( fields.m_status )
        json[ "status" ] = statusToString( *fields.m_status );
}

bool ProvisioningListUtils::entryFromDbJson( const Json& json, znet::ProvisioningListEntry& entry ) {
    if( json.empty() )
        return false;

    if( !ProvisioningListUtils::idFromJson( json, entry.m_provisioningId ).empty() )
        return false;
    if( !ProvisioningListUtils::dskFromJson( json, entry.m_dsk, true ).empty() )
        return false;
    if( !ProvisioningListUtils::entryFieldsFromJson( json, entry.m_fields, true ).empty() )
        return false;

    ProvisioningListUtils::genEntryFieldsFromJson( json, entry.m_fields );
    return true;
}

void ProvisioningListUtils::entryToJson( const znet::ProvisioningListEntry& entry, Json& json, bool dbFormat ) {
    idToJson( entry.m_provisioningId, json );
    dskToJson( entry.m_dsk, json, dbFormat );
    entryFieldsToJson( entry.m_fields, json );
    genEntryFieldsToJson( entry.m_fields, json );
}

void ProvisioningListUtils::entryToJsonUiFiltered( const znet::ProvisioningListEntry& entry,
                                                   Json& json,
                                                   bool dbFormat ) {
    idToJson( entry.m_provisioningId, json );
    dskToJson( entry.m_dsk, json, dbFormat );
    entryFieldsToJsonUiFiltered( entry.m_fields, json );
    genEntryFieldsToJson( entry.m_fields, json );
}

znet::utils::optional< uint32_t > ProvisioningListUtils::idFromString( const std::string& provisioningId ) {
    using o = znet::utils::optional< uint32_t >;
    if( provisioningId.empty() )
        return o();

    char* end = nullptr;
    uint32_t id = strtoul( provisioningId.c_str(), &end, 10 );
    return *end == '\0' ? o( id ) : o();
}

std::string ProvisioningListUtils::idToString( uint32_t provisioningId ) {
    return std::to_string( provisioningId );
}

void ProvisioningListUtils::dskFromString( const std::string& str, znet::DSK& dsk ) {
    if( str.size() < 32 )
        return;

    sscanf( str.c_str(),
            "%04hx%04hx%04hx%04hx%04hx%04hx%04hx%04hx",
            &dsk[ 0 ],
            &dsk[ 1 ],
            &dsk[ 2 ],
            &dsk[ 3 ],
            &dsk[ 4 ],
            &dsk[ 5 ],
            &dsk[ 6 ],
            &dsk[ 7 ] );
}

std::string ProvisioningListUtils::dskToString( znet::DSK dsk ) {
    std::string str;
    str.resize( 32, '0' );
    sprintf( const_cast< char* >( str.data() ),
             "%04hx%04hx%04hx%04hx%04hx%04hx%04hx%04hx",
             dsk[ 0 ],
             dsk[ 1 ],
             dsk[ 2 ],
             dsk[ 3 ],
             dsk[ 4 ],
             dsk[ 5 ],
             dsk[ 6 ],
             dsk[ 7 ] );
    return str;
}

znet::utils::optional< znet::ProvisioningListEntryFields::Status > ProvisioningListUtils::statusFromString(
    const std::string& status ) {
    using o = znet::utils::optional< znet::ProvisioningListEntryFields::Status >;
    if( status == "notDetected" )
        return o( znet::ProvisioningListEntryFields::Status::NOT_DETECTED );
    if( status == "added" )
        return o( znet::ProvisioningListEntryFields::Status::ADDED );
    if( status == "otherNetwork" )
        return o( znet::ProvisioningListEntryFields::Status::OTHER_NETWORK );
    if( status == "failed" )
        return o( znet::ProvisioningListEntryFields::Status::FAILED );

    return o();
}

const char* ProvisioningListUtils::statusToString( znet::ProvisioningListEntryFields::Status status ) {
    switch( status ) {
    case znet::ProvisioningListEntryFields::Status::NOT_DETECTED:
        return "notDetected";
    case znet::ProvisioningListEntryFields::Status::ADDED:
        return "added";
    case znet::ProvisioningListEntryFields::Status::OTHER_NETWORK:
        return "otherNetwork";
    case znet::ProvisioningListEntryFields::Status::FAILED:
        return "failed";
    default:
        assert( false );
        return "";
    }
}

znet::utils::optional< znet::ProvisioningListEntryFields::AddingMode > ProvisioningListUtils::addingModeFromString(
    const std::string& addingMode ) {
    using o = znet::utils::optional< znet::ProvisioningListEntryFields::AddingMode >;
    if( addingMode == "s2" )
        return o( znet::ProvisioningListEntryFields::AddingMode::S2 );
    if( addingMode == "smart" )
        return o( znet::ProvisioningListEntryFields::AddingMode::SMART );

    return o();
}

znet::utils::optional< znet::ProvisioningListEntryFields::InclusionSetting >
ProvisioningListUtils::inclusionSettingFromString( const std::string& inclusionSetting ) {
    using InclusionSettingOptional = znet::utils::optional< znet::ProvisioningListEntryFields::InclusionSetting >;
    if( inclusionSetting == "pending" )
        return InclusionSettingOptional( znet::ProvisioningListEntryFields::InclusionSetting::PENDING );
    if( inclusionSetting == "passive" )
        return InclusionSettingOptional( znet::ProvisioningListEntryFields::InclusionSetting::PASSIVE );
    if( inclusionSetting == "ignored" )
        return InclusionSettingOptional( znet::ProvisioningListEntryFields::InclusionSetting::IGNORED );

    return InclusionSettingOptional();
}

const char* ProvisioningListUtils::addingModeToString( znet::ProvisioningListEntryFields::AddingMode addingMode ) {
    switch( addingMode ) {
    case znet::ProvisioningListEntryFields::AddingMode::S2:
        return "s2";
    case znet::ProvisioningListEntryFields::AddingMode::SMART:
        return "smart";
    default:
        assert( false );
        return "";
    }
}

const char* ProvisioningListUtils::inclusionSettingToString(
    znet::ProvisioningListEntryFields::InclusionSetting setting ) {
    switch( setting ) {
    case znet::ProvisioningListEntryFields::InclusionSetting::PENDING:
        return "pending";
    case znet::ProvisioningListEntryFields::InclusionSetting::PASSIVE:
        return "passive";
    case znet::ProvisioningListEntryFields::InclusionSetting::IGNORED:
        return "ignored";
    default:
        assert( false );
        return "";
    }
}

uint8_t ProvisioningListUtils::securityModeFromString( const std::string& securityMode ) {
    if( securityMode == "accessControl" )
        return ZNET_SECURITY2_S2_CLASS_2;
    if( securityMode == "authenticated" )
        return ZNET_SECURITY2_S2_CLASS_1;
    if( securityMode == "unauthenticated" )
        return ZNET_SECURITY2_S2_CLASS_0;
    if( securityMode == "s0" )
        return ZNET_SECURITY2_S0_NET_KEY;

    return 0;
}

Json ProvisioningListUtils::securityModeToJson( uint8_t modes ) {
    auto securityModes = JSON_EMPTY_ARR;
    if( modes & ZNET_SECURITY2_S2_CLASS_2 )
        securityModes.push_back( "accessControl" );
    if( modes & ZNET_SECURITY2_S2_CLASS_1 )
        securityModes.push_back( "authenticated" );
    if( modes & ZNET_SECURITY2_S2_CLASS_0 )
        securityModes.push_back( "unauthenticated" );
    if( modes & ZNET_SECURITY2_S0_NET_KEY )
        securityModes.push_back( "s0" );

    return securityModes;
}

}  // namespace Zwave
}  // namespace HubPlatform
