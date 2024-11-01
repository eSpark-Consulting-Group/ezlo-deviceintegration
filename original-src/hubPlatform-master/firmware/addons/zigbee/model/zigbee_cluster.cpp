#include "zigbee/model/zigbee_cluster.h"
#include "zigbee/model/enums/cluster_id.h"

#include "zigbee/resources/zigbee_resources.h"
#include "database_client/dbqueryparser.h"
#include "json_utils/json_utils.h"

#include "common_utils/cpp14.h"

#include <cassert>

namespace HubPlatform {
namespace Zigbee {

namespace {
const char* toString( ZigbeeClusterBase::ClusterDataHolder holder ) {
    switch( holder ) {
    case ZigbeeClusterBase::ClusterDataHolder::Client:
        return Resources::Fields::Cluster::DataHolders::Client;
    case ZigbeeClusterBase::ClusterDataHolder::Server:
        return Resources::Fields::Cluster::DataHolders::Server;
    case ZigbeeClusterBase::ClusterDataHolder::ClientServer:
        return Resources::Fields::Cluster::DataHolders::ClientServer;
    default:
        return Resources::Fields::Cluster::DataHolders::Undefined;
    }
}

ZigbeeClusterBase::ClusterDataHolder dataHolderFromString( const std::string& name ) {
    if( name == Resources::Fields::Cluster::DataHolders::Client ) {
        return ZigbeeClusterBase::ClusterDataHolder::Client;
    }
    else if( name == Resources::Fields::Cluster::DataHolders::Server ) {
        return ZigbeeClusterBase::ClusterDataHolder::Server;
    }
    else if( name == Resources::Fields::Cluster::DataHolders::ClientServer ) {
        return ZigbeeClusterBase::ClusterDataHolder::ClientServer;
    }
    else if( name == Resources::Fields::Cluster::DataHolders::Undefined ) {
        return ZigbeeClusterBase::ClusterDataHolder::Undefined;
    }
    else {
        assert( !"Undefined data holder name" );
        return ZigbeeClusterBase::ClusterDataHolder::Undefined;
    }
}
}  // namespace

ZigbeeClusterBase::ZigbeeClusterBase( uint16_t id, uint8_t endpointId, ClusterDataHolder dataHolder )
: m_id( id )
, m_endpointId( endpointId )
, m_name( toString( static_cast< ClusterId >( m_id ) ) )
, m_dataHolder( dataHolder ) {}

ZigbeeClusterBase::ZigbeeClusterBase( const Json& fromJson )
: m_id( json_get_int( fromJson, Resources::Fields::Cluster::Id ) )
, m_endpointId( json_get_int( fromJson, Resources::Fields::Cluster::EndpointId ) )
, m_name( toString( static_cast< ClusterId >( m_id ) ) )
, m_dataHolder( dataHolderFromString( json_get_str( fromJson, Resources::Fields::Cluster::DataHolder ) ) ) {
    for( const auto& joAttr : fromJson[ Resources::Fields::Cluster::Attributes ] ) {
        auto attr = std::make_unique< ZigbeeAttribute >( joAttr );
        m_attributes.emplace( attr->getId(), std::move( attr ) );
    }

    for( const auto& joCommand : fromJson[ Resources::Fields::Cluster::Commands ] ) {
        auto command = std::make_unique< ZigbeeCommand >( joCommand );
        auto key = std::make_pair( command->getId(), command->getType() );
        m_commands.emplace( key, std::move( command ) );
    }
}

void ZigbeeClusterBase::setDataHolder( ClusterDataHolder dataHolder ) {
    m_dataHolder = static_cast< ClusterDataHolder >( static_cast< uint8_t >( m_dataHolder ) |
                                                     static_cast< uint8_t >( dataHolder ) );
}

void ZigbeeClusterBase::clrDataHolder( ClusterDataHolder dataHolder ) {
    m_dataHolder = static_cast< ClusterDataHolder >( static_cast< uint8_t >( m_dataHolder ) &
                                                     ~static_cast< uint8_t >( dataHolder ) );
}

uint16_t ZigbeeClusterBase::getId() const {
    return m_id;
}

uint8_t ZigbeeClusterBase::getEndpointId() const {
    return m_endpointId;
}

ZigbeeClusterBase::ClusterDataHolder ZigbeeClusterBase::getDataHolder() const {
    return m_dataHolder;
}

bool ZigbeeClusterBase::forEachAttribute( std::function< bool( ZigbeeAttribute& ) > callback ) {
    for( auto& data : m_attributes ) {
        if( !callback( *data.second ) )
            return false;
    }
    return true;
}

bool ZigbeeClusterBase::forEachCommand( std::function< bool( ZigbeeCommand& ) > callback ) {
    for( auto& data : m_commands ) {
        if( !callback( *data.second ) )
            return false;
    }
    return true;
}

const std::string& ZigbeeClusterBase::getName() const {
    return m_name;
}

ZigbeeAttribute* ZigbeeClusterBase::findAttribute( uint16_t attributeId ) {
    auto it = m_attributes.find( attributeId );
    return it != m_attributes.end() ? it->second.get() : nullptr;
}

const ZigbeeAttribute* ZigbeeClusterBase::findAttribute( uint16_t attributeId ) const {
    auto it = m_attributes.find( attributeId );
    return it != m_attributes.end() ? it->second.get() : nullptr;
}

const uint16_t* ZigbeeClusterBase::getLastAttributeId() const {
    return m_attributes.empty() ? nullptr : &m_attributes.rbegin()->first;
}

const uint16_t* ZigbeeClusterBase::getLastMfgSpecificAttributeId() const {
    for( auto it = m_attributes.rbegin(); it != m_attributes.rend(); ++it ) {
        if( it->second->isMfgSpecific() ) {
            return &it->first;
        }
    }

    return nullptr;
}

ZigbeeCommand* ZigbeeClusterBase::findCommand( uint8_t commandId, CommandType type ) {
    auto key = std::make_pair( commandId, type );
    return m_commands.find( key ) != m_commands.end() ? m_commands[ key ].get() : nullptr;
}

const uint8_t* ZigbeeClusterBase::getLastCommandId( CommandType type ) const {
    for( auto it = m_commands.rbegin(); it != m_commands.rend(); ++it ) {
        if( it->second->getType() == type ) {
            return &it->first.first;
        }
    }

    return nullptr;
}

const uint8_t* ZigbeeClusterBase::getLastMfgSpecificCommandId( CommandType type ) const {
    for( auto it = m_commands.rbegin(); it != m_commands.rend(); ++it ) {
        if( it->second->isMfgSpecific() && it->second->getType() == type ) {
            return &it->first.first;
        }
    }

    return nullptr;
}

Json ZigbeeClusterBase::toJson() const {
    JsonArray jaAttributes = JSON_EMPTY_ARR;
    for( const auto& item : m_attributes ) {
        jaAttributes.push_back( item.second->toJson() );
    }

    JsonArray jaCommands = JSON_EMPTY_ARR;
    for( const auto& item : m_commands ) {
        jaCommands.push_back( item.second->toJson() );
    }

    return JSON( Resources::Fields::Cluster::Id << m_id << Resources::Fields::Cluster::EndpointId << m_endpointId
                                                << Resources::Fields::Cluster::Name << m_name
                                                << Resources::Fields::Cluster::DataHolder << toString( m_dataHolder )
                                                << Resources::Fields::Cluster::Attributes << std::move( jaAttributes )
                                                << Resources::Fields::Cluster::Commands << std::move( jaCommands ) );
}

void ZigbeeClusterBase::addAttribute( ZigbeeAttribute::AttributeUPtr&& attribute ) {
    auto id = attribute->getId();
    m_attributes.emplace( id, std::move( attribute ) );
}

void ZigbeeClusterBase::addCommand( ZigbeeCommand::CommandUPtr&& command ) {
    auto key = std::make_pair( command->getId(), command->getType() );
    m_commands.emplace( key, std::move( command ) );
}

size_t ZigbeeClusterBase::getAttributesCount() const {
    return m_attributes.size();
}

bool ZigbeeClusterBase::isServer() const {
    return ( ( (uint8_t) m_dataHolder & (uint8_t) ClusterDataHolder::Server ) != 0 );
}

bool ZigbeeClusterBase::isClient() const {
    return ( ( (uint8_t) m_dataHolder & (uint8_t) ClusterDataHolder::Client ) != 0 );
}

bool ZigbeeClusterBase::isClientServer() const {
    return ( m_dataHolder == ClusterDataHolder::ClientServer );
}

bool ZigbeeClusterBase::isBindable() const {
    return ( m_id != static_cast< uint16_t >( Zigbee::ClusterId::Basic ) );
}

}  // namespace Zigbee
}  // namespace HubPlatform
