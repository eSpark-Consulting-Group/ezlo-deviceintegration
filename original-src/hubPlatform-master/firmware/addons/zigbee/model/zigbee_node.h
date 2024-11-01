#pragma once

#include "zigbee/model/zigbee_endpoint.h"

#include "json_utils/json_fwd.h"
#include "base_utils/optional.h"

#include <map>
#include <chrono>

namespace HubPlatform {
namespace Zigbee {

class ZigbeeNode final {
public:
    using EndpointUPtr = std::unique_ptr< ZigbeeEndpoint >;
    using Endpoints = std::map< int16_t, EndpointUPtr >;

public:
    explicit ZigbeeNode( const uint16_t nodeId,
                         const bool security,
                         const bool isReachable,
                         const bool isBatteryPowered,
                         const std::string& modelId = "",
                         const std::string& manufacturerId = "" );
    explicit ZigbeeNode( const Json& json );
    ZigbeeNode( const ZigbeeNode& fromNode ) = delete;
    ZigbeeNode( ZigbeeNode&& fromNode ) = delete;
    ~ZigbeeNode();

    void addEndpoint( EndpointUPtr&& fromEndpoint );
    void setCapabilities( const uint8_t capabilities );
    void setDiscovered( const bool discovered );
    void setManufacturerId( const std::string& newValue );
    void setModelId( const std::string& newValue );
    void setIsReachable( bool value );
    void deviceUpdated();
    void setReachableStateRecheckTimeout( std::chrono::system_clock::duration timeout );

    uint16_t getId() const;
    common::optional< uint8_t > getCapabilities() const;
    bool getDiscovered() const;
    const std::string& getManufacturerId() const;
    const std::string& getModelId() const;
    bool isReachable() const;
    bool isSleepy() const;
    std::chrono::system_clock::time_point getLastUpdateTime() const;
    std::chrono::system_clock::duration getReachableStateRecheckTimeout() const;

    bool forEachEndpoint( std::function< bool( ZigbeeEndpoint& ) > callback );
    ZigbeeEndpoint* findEndpoint( int16_t endpointId );
    const ZigbeeEndpoint* findEndpoint( int16_t endpointId ) const;

    Json toJson() const;

private:
    uint16_t m_id = 0;
    common::optional< uint8_t > m_capabilities = {};
    bool m_discovered = false;
    bool m_security = false;
    bool m_isReachable = false;
    bool m_isBatteryPowered = false;
    bool m_isSleepyDevice = false;
    std::string m_modelId;
    std::string m_manufacturerId;
    std::chrono::system_clock::time_point m_lastUpdateTime;
    std::chrono::system_clock::duration m_reachableStateRecheckTimeout;
    Endpoints m_endpoints;
};

}  // namespace Zigbee
}  // namespace HubPlatform
