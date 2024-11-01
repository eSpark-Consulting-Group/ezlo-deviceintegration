#pragma once
#include "zigbee/model/enums/cluster_id.h"
#include "zigbee/model/enums/attribute_id.h"
#include "zigbee/model/enums/attribute_type.h"

#include "json_utils/json_fwd.h"

#include <stdint-gcc.h>
#include <map>
#include <memory>

namespace HubPlatform {
namespace Zigbee {

class ZigbeeAttribute {
public:
    using AttributeUPtr = std::unique_ptr< ZigbeeAttribute >;
    using Attributes = std::map< uint16_t, AttributeUPtr >;

public:
    explicit ZigbeeAttribute( uint16_t id, uint16_t clusterId, AttributeValueType type, bool isMfgSpecific );
    explicit ZigbeeAttribute( const Json& json );
    ZigbeeAttribute( const ZigbeeAttribute& ) = delete;
    ZigbeeAttribute( ZigbeeAttribute&& fromAttribute ) = delete;
    virtual ~ZigbeeAttribute() = default;

public:
    virtual uint16_t getId() const;
    AttributeValueType getType() const;
    std::string getName() const;
    bool isMfgSpecific() const;

    bool hasActualValue() const;
    const std::string& getValue() const;
    const uint16_t getMfgCode() const;

    void setValue( const std::string& newValue );
    void setMfgCode( const uint16_t newValue );
    virtual Json toJson() const;

private:
    uint16_t m_id, m_clusterId;
    const AttributeValueType m_type;
    const bool m_isMfgSpecific;
    uint16_t m_mfgCode = 0xffff;

    bool m_actualValue;
    std::string m_value;
};

class ZigbeeAttributeFactory {
public:
    ZigbeeAttributeFactory() = delete;
    ~ZigbeeAttributeFactory() = default;

public:
    static std::unique_ptr< ZigbeeAttribute > createZigbeeAttribute( uint16_t clusterId,
                                                                     uint16_t attributeId,
                                                                     AttributeValueType valueType,
                                                                     bool isMfgSpecific );
};

}  // namespace Zigbee
}  // namespace HubPlatform
