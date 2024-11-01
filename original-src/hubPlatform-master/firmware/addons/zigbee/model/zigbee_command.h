#pragma once
#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/attribute_id.h"
#include "zigbee/model/enums/command_type.h"

#include "json_utils/json_fwd.h"

#include <stdint-gcc.h>
#include <map>
#include <memory>

namespace HubPlatform {
namespace Zigbee {

class ZigbeeCommand {
public:
    using CommandUPtr = std::unique_ptr< ZigbeeCommand >;
    using Commands = std::map< std::pair< uint8_t, CommandType >, CommandUPtr >;

public:
    explicit ZigbeeCommand( uint8_t id,
                            const std::string& name,
                            uint16_t clusterId,
                            CommandType type,
                            bool isMfgSpecific );
    explicit ZigbeeCommand( const Json& json );

    virtual ~ZigbeeCommand() = default;

public:
    virtual uint8_t getId() const;
    CommandType getType() const;
    const std::string& getName() const;

    bool isMfgSpecific() const;
    const uint16_t getMfgCode() const;

    void setMfgCode( const uint16_t newValue );
    virtual Json toJson() const;

private:
    uint8_t m_id;
    std::string m_name;
    uint16_t m_clusterId;
    const CommandType m_type;
    const bool m_isMfgSpecific;
    uint16_t m_mfgCode = 0xffff;
};

class ZigbeeCommandFactory {
public:
    ZigbeeCommandFactory() = delete;
    ~ZigbeeCommandFactory() = default;

public:
    static std::unique_ptr< ZigbeeCommand > createZigbeeCommand( uint16_t clusterId,
                                                                 uint8_t commandId,
                                                                 const std::string& commandName,
                                                                 CommandType type,
                                                                 bool isMfgSpecific );
};

}  // namespace Zigbee
}  // namespace HubPlatform
