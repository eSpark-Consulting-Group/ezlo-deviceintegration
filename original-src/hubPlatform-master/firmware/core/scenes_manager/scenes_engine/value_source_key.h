#pragma once
#include <string>
#include <tuple>

namespace SceneEngine {

struct ValueSourceKey {
    using ControllerId = std::string;
    using SourceId = std::string;
    enum class SourceType { Item, Expression, Device, DeviceItemGroups };

    ValueSourceKey(
        ControllerId controllerId,
        SourceId itemId,
        SourceType type = SourceType::Item )  // TODO don't use default constructor to highlight dependencies
    : m_key( std::move( controllerId ), std::move( itemId ) )
    , m_type{ type } {}

    const SourceId& getSourceId() const {
        return std::get< 1 >( m_key );
    }

    const ControllerId& getControllerId() const {
        return std::get< 0 >( m_key );
    }

    SourceType getType() const {
        return m_type;
    }

    bool isItem() const {
        return m_type == SourceType::Item;
    }

    bool isExpression() const {
        return m_type == SourceType::Expression;
    }

    bool isDevice() const {
        return m_type == SourceType::Device;
    }

    bool isDeviceItemGroups() const {
        return m_type == SourceType::DeviceItemGroups;
    }

    bool operator<( const ValueSourceKey& r ) const {
        if( getSourceId() < r.getSourceId() )
            return true;
        if( getSourceId() == r.getSourceId() && getControllerId() < r.getControllerId() )
            return true;
        return false;
    }

    bool operator==( const ValueSourceKey& r ) const {
        if( m_type != r.m_type || m_key != r.m_key )
            return false;
        return true;
    }

    static std::string packDeviceItemGroupIds( std::string deviceGroupId, std::string itemGroupId ) {
        std::string packedId;
        if( !deviceGroupId.empty() )
            packedId = std::move( deviceGroupId );
        if( !itemGroupId.empty() )
            packedId += getDelimiter() + std::move( itemGroupId );
        return packedId;
    }

    static std::pair< std::string, std::string > unpackDeviceItemGroupIds(
        const std::string& packedDeviceItemGroupIds ) {
        std::string deviceGroupId;
        std::string itemGroupId;
        auto pos = packedDeviceItemGroupIds.find( getDelimiter() );
        if( pos == std::string::npos ) {
            deviceGroupId = packedDeviceItemGroupIds;
        }
        else {
            deviceGroupId = packedDeviceItemGroupIds.substr( 0, pos );
            pos += getDelimiter().size();
            if( pos < packedDeviceItemGroupIds.size() )
                itemGroupId = packedDeviceItemGroupIds.substr( pos );
        }
        return { std::move( deviceGroupId ), std::move( itemGroupId ) };
    }

    static std::string getDelimiter() {
        return "/";
    }

private:
    std::tuple< ControllerId, SourceId > m_key;
    SourceType m_type = SourceType::Item;
};

}  // namespace SceneEngine
