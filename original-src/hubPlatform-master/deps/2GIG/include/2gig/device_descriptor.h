#pragma once

#include "2gig/device_type.h"

#include <string>
#include <vector>

namespace TwoGig {

struct DeviceDescriptor {
    enum class DescriptorType { Model, DeviceType };

public:
    DeviceDescriptor( std::vector< uint32_t > txIds, const std::string& model )
    : m_txIds( std::move( txIds ) )
    , m_model( model ) {}

    DeviceDescriptor( std::vector< uint32_t > txIds, DeviceType::Enum deviceType )
    : m_txIds( std::move( txIds ) )
    , m_deviceType( deviceType ) {}

    DeviceDescriptor( std::vector< uint32_t > txIds, DeviceType::Enum deviceType, const std::string& model )
    : m_txIds( std::move( txIds ) )
    , m_deviceType( deviceType )
    , m_model( model ) {}

    DescriptorType getType() const {
        return m_model.empty() ? DescriptorType::DeviceType : DescriptorType::Model;
    }

    const std::vector< uint32_t >& getTxIds() const {
        return m_txIds;
    }

    const std::string& getModel() const {
        return m_model;
    }

    DeviceType::Enum getDeviceType() const {
        return m_deviceType;
    }

private:
    std::vector< uint32_t > m_txIds;

    DeviceType::Enum m_deviceType{ DeviceType::SmokeDetector };
    std::string m_model;
};

}  // namespace TwoGig
