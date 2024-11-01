#pragma once
#include "zigbee/model/zigbee_attribute.h"
#include "zigbee/model/zigbee_command.h"

#include "json_utils/json_fwd.h"

#include <stdint-gcc.h>

namespace HubPlatform {
namespace Zigbee {

class ZigbeeClusterBase {
public:
    enum class ClusterDataHolder : uint8_t {
        Undefined = 0,
        Server = 1,
        Client = 2,
        ClientServer = ClusterDataHolder::Server | ClusterDataHolder::Client
    };

public:
    ZigbeeClusterBase( const ZigbeeClusterBase& ) = delete;
    ZigbeeClusterBase( ZigbeeClusterBase&& fromCluster ) = delete;
    explicit ZigbeeClusterBase( uint16_t id,
                                uint8_t endpointId,
                                ClusterDataHolder dataHolder = ClusterDataHolder::Undefined );
    explicit ZigbeeClusterBase( const Json& json );
    ~ZigbeeClusterBase() = default;

public:
    void setDataHolder( ClusterDataHolder dataHolder );
    void clrDataHolder( ClusterDataHolder dataHolder );

public:
    uint16_t getId() const;
    uint8_t getEndpointId() const;

    ClusterDataHolder getDataHolder() const;

    ZigbeeAttribute* findAttribute( uint16_t attributeId );
    const ZigbeeAttribute* findAttribute( uint16_t attributeId ) const;
    const uint16_t* getLastAttributeId() const;
    const uint16_t* getLastMfgSpecificAttributeId() const;

    ZigbeeCommand* findCommand( uint8_t commandId, CommandType type );
    const uint8_t* getLastCommandId( CommandType type ) const;
    const uint8_t* getLastMfgSpecificCommandId( CommandType type ) const;

    Json toJson() const;

public:
    bool forEachAttribute( std::function< bool( ZigbeeAttribute& ) > callback );
    bool forEachCommand( std::function< bool( ZigbeeCommand& ) > callback );
    const std::string& getName() const;

    void addAttribute( ZigbeeAttribute::AttributeUPtr&& attribute );
    void addCommand( ZigbeeCommand::CommandUPtr&& command );

    size_t getAttributesCount() const;
    bool isServer() const;
    bool isClient() const;
    bool isClientServer() const;
    bool isBindable() const;

private:
    uint16_t m_id;
    uint8_t m_endpointId;
    const std::string m_name;
    ClusterDataHolder m_dataHolder;

    ZigbeeAttribute::Attributes m_attributes;
    ZigbeeCommand::Commands m_commands;
};

}  // namespace Zigbee
}  // namespace HubPlatform
