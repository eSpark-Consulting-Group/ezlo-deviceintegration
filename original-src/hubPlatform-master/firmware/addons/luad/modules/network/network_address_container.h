#pragma once
#include "luad/modules/network/network_address.h"

namespace HubPlatform {
namespace LuaNetwork {

class IAddressContainer {
public:
    using AddressPtr = std::shared_ptr< IAddress >;

    virtual ~IAddressContainer() = default;

    virtual bool next() = 0;
    virtual bool isFinished() = 0;
    virtual AddressPtr getCurrent() = 0;
    virtual AddressPtr getLast() = 0;
    virtual void setFirst() = 0;
};

class AddressRange : public IAddressContainer {
public:
    AddressRange( AddressPtr&& min, AddressPtr&& max );

    virtual bool next() override;
    virtual bool isFinished() override;
    virtual AddressPtr getCurrent() override;
    virtual AddressPtr getLast() override;
    virtual void setFirst() override;

private:
    AddressPtr m_min;
    AddressPtr m_max;
    bool m_isFinished;
    IAddress::BaseAddress m_first;
};

class AddressList : public IAddressContainer {
public:
    using Addresses = std::vector< AddressPtr >;
    AddressList( Addresses&& addresses );

    virtual bool next() override;
    virtual bool isFinished() override;
    virtual AddressPtr getCurrent() override;
    virtual AddressPtr getLast() override;
    virtual void setFirst() override;

private:
    mutable std::size_t m_iterator;
    Addresses m_addresses;
    bool m_isFinished;
};

}  // namespace LuaNetwork
}  // namespace HubPlatform
