#include "zwave/zwave_addon_commands_handler.h"
#include "zwave/provisioning/provisioning_list_utils.h"
#include "zwave/resources/zwave_resources.h"

#include "json_utils/json_utils.h"
#include "jsonrpc/jsonrpc.h"

#include <set>

namespace HubPlatform {
namespace Zwave {

namespace {

void logStatus( const std::string& operation, znet::api::IProvisioningListController::OperationStatus status ) {
    switch( status ) {
    case znet::api::IProvisioningListController::OperationStatus::SUCCESS:
        Log::trace( cc::trace( operation + ": " ) + cc::note( "success" ) );
        break;
    case znet::api::IProvisioningListController::OperationStatus::ENTRY_LIMIT_REACHED:
        Log::error( cc::error( operation + ": " ) + cc::note( "failed (entry limit reached)" ) );
        break;
    case znet::api::IProvisioningListController::OperationStatus::ENTRY_REPLACED:
        Log::trace( cc::trace( operation + ": " ) + cc::note( "replaced" ) );
        break;
    case znet::api::IProvisioningListController::OperationStatus::ENTRY_NOT_FOUND:
        Log::error( cc::error( operation + ": " ) + cc::note( "failed (entry not found)" ) );
        break;
    case znet::api::IProvisioningListController::OperationStatus::STORAGE_NOT_AVAILABLE:
        Log::error( cc::error( operation + ": " ) + cc::note( "failed (storage not available)" ) );
        break;
    default:
        break;
    }
}

}  // namespace

ZwaveAddonCommandsHandler::ZwaveAddonCommandsHandler(
    znet::IProvisioningListProvider& provisioningListProvider,
    znet::api::IProvisioningListController& provisioningListController )
: m_provisioningListProvider( provisioningListProvider )
, m_provisioningListController( provisioningListController ) {}

JsonObject ZwaveAddonCommandsHandler::handleAddonMessage( const JsonObject& request ) {
    LOGGER_TRACE( cc::trace( "start" ) );
    using RequestHandler = JsonObject ( ZwaveAddonCommandsHandler::* )( const JsonObject& request );
    using RequestHandlersMap = std::map< std::string, RequestHandler >;
    static const RequestHandlersMap requestHandlers = {
        { Resources::RPC::ProvisioningList, &ZwaveAddonCommandsHandler::handleProvisioningList },
        { Resources::RPC::ProvisioningAdd, &ZwaveAddonCommandsHandler::handleProvisioningAdd },
        { Resources::RPC::ProvisioningSet, &ZwaveAddonCommandsHandler::handleProvisioningSet },
        { Resources::RPC::ProvisioningRemove, &ZwaveAddonCommandsHandler::handleProvisioningRemove },
    };

    auto method = json_get_str( request, "method" );
    const auto it = requestHandlers.find( method );
    if( it != requestHandlers.end() ) {
        const auto handler = it->second;
        return ( this->*handler )( request[ "params" ] );
    }

    Json answer;
    setError( answer, Errors::UNKNOWN_METHOD );
    return answer;
}

JsonObject ZwaveAddonCommandsHandler::handleProvisioningList( const JsonObject& params ) {
    LOGGER_TRACE( cc::trace( "start" ) );
    std::set< uint32_t > provisioningIdsSet;
    auto provisioningIds = json_get_vec< std::string >( params, "ids" );
    for( auto& id : provisioningIds ) {
        auto provisioningId = ProvisioningListUtils::idFromString( id );
        if( provisioningId )
            provisioningIdsSet.insert( *provisioningId );
    }

    std::set< uint8_t > nodeIdsSet;
    auto nodeIds = json_get_vec< uint8_t >( params, "nodeIds" );
    nodeIdsSet.insert( nodeIds.begin(), nodeIds.end() );

    LOGGER_TRACE( cc::trace( "loadProvisioningList" ) );
    auto provisioning = JSON_EMPTY_ARR;
    m_provisioningListProvider.loadProvisioningList( [&]( const znet::ProvisioningListEntry& entry ) {
        if( ( nodeIdsSet.empty() && provisioningIdsSet.empty() ) ||
            provisioningIdsSet.count( entry.m_provisioningId ) ||
            ( entry.m_fields.m_node && nodeIdsSet.count( *entry.m_fields.m_node ) ) ) {
            provisioning.push_back( JSON_EMPTY_OBJ );
            ProvisioningListUtils::entryToJsonUiFiltered( entry, provisioning.back(), false );
        }
        return false;
    } );

    Json answer;
    answer[ "provisioning" ] = provisioning;
    LOGGER_TRACE( cc::trace( "finish" ) );
    return answer;
}

JsonObject ZwaveAddonCommandsHandler::handleProvisioningAdd( const JsonObject& params ) {
    LOGGER_TRACE( cc::trace( "start" ) );
    Json answer = JSON_EMPTY_OBJ;
    Json error;

    znet::ProvisioningListEntry entry;
    error = ProvisioningListUtils::dskFromJson( params, entry.m_dsk, false );
    if( !error.empty() ) {
        setErrorObject( answer, error );
        return answer;
    }
    error = ProvisioningListUtils::entryFieldsFromJson( params, entry.m_fields, true );
    if( !error.empty() ) {
        setErrorObject( answer, error );
        return answer;
    }

    LOGGER_TRACE( cc::trace( "addProvisioningListEntry" ) );
    auto status = m_provisioningListController.addProvisioningListEntry( entry.m_dsk, entry.m_fields );
    logStatus( "Adding provision entry", status );

    if( status == znet::api::IProvisioningListController::OperationStatus::ENTRY_LIMIT_REACHED ) {
        setError( answer, Errors::ENTRY_LIMIT_REACHED );
    }
    else if( status == znet::api::IProvisioningListController::OperationStatus::INCORRECT_ADDING_MODE ) {
        Errors::getInvalidValueRange( "addingMode", error, "" );
        setErrorObject( answer, error );
    }
    LOGGER_TRACE( cc::trace( "finish" ) );
    return answer;
}

JsonObject ZwaveAddonCommandsHandler::handleProvisioningSet( const JsonObject& params ) {
    LOGGER_TRACE( cc::trace( "start" ) );
    Json answer = JSON_EMPTY_OBJ;
    Json error;

    uint32_t id;
    error = ProvisioningListUtils::idFromJson( params, id );
    if( !error.empty() ) {
        setErrorObject( answer, error );
        return answer;
    }

    LOGGER_TRACE( cc::trace( "findProvisioningListEntryByID" ) );
    znet::ProvisioningListEntry entry;
    if( !m_provisioningListProvider.findProvisioningListEntryByID( id, entry ) ) {
        setError( answer, Errors::OBJECT_NOT_FOUND );
        return answer;
    }

    error = ProvisioningListUtils::entryFieldsFromJson( params, entry.m_fields, false );
    if( !error.empty() ) {
        setErrorObject( answer, error );
        return answer;
    }

    LOGGER_TRACE( cc::trace( "setProvisioningListEntry" ) );
    auto status = m_provisioningListController.setProvisioningListEntry( entry.m_provisioningId, entry.m_fields );
    logStatus( "Updating provision entry", status );

    if( status == znet::api::IProvisioningListController::OperationStatus::INCORRECT_ADDING_MODE ) {
        Errors::getInvalidValueRange( "addingMode", error, "" );
        setErrorObject( answer, error );
    }

    LOGGER_TRACE( cc::trace( "finish" ) );
    return answer;
}

JsonObject ZwaveAddonCommandsHandler::handleProvisioningRemove( const JsonObject& params ) {
    LOGGER_TRACE( cc::trace( "start" ) );
    Json answer = JSON_EMPTY_OBJ;
    Json error;

    if( !checkField( params, "provisioning", answer ) )
        return answer;

    auto provisioningIds = json_get_vec< std::string >( params, "provisioning" );
    if( provisioningIds.empty() ) {
        Errors::getInvalidValueRange( "provisioning", error, "" );
        setErrorObject( answer, error, "" );
        return answer;
    }
    LOGGER_TRACE( cc::trace( "remove" ) );
    for( auto& id : provisioningIds ) {
        auto provisioningId = ProvisioningListUtils::idFromString( id );
        if( !provisioningId ) {
            Errors::getInvalidValueRange( "provisioning", error, "" );
            setErrorObject( answer, error, "" );
            return answer;
        }
        auto status = m_provisioningListController.removeProvisioningListEntry( *provisioningId );
        logStatus( "Removing provision entry", status );
        if( status != znet::api::IProvisioningListController::OperationStatus::SUCCESS ) {
            setError( answer, Errors::OBJECT_NOT_FOUND );
        }
    }
    LOGGER_TRACE( cc::trace( "finish" ) );
    return answer;
}

}  // namespace Zwave
}  // namespace HubPlatform
