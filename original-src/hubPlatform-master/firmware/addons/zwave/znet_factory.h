#pragma once

#include "common_utils/cpp14.h"

#include <functional>
#include <memory>

namespace znet {

class IZNet;
class IZNetOwner;

}  // namespace znet

namespace HubPlatform {
namespace Zwave {

class ZNetFactory {
public:
    template < typename ZNetType >
    struct ZNetTypeTag {};

public:
    template < typename ZNetType >
    ZNetFactory( ZNetTypeTag< ZNetType > )
    : m_znetCreator( []( znet::IProvisioningListProvider& classicInclusionProvider,
                         znet::IProvisioningListProvider& provider,
                         znet::IProvisioningListObserver& observer,
                         znet::IZNetOwner& znetOwner ) {
        return std::make_unique< ZNetType >( classicInclusionProvider, provider, observer, znetOwner );
    } ) {}

    std::unique_ptr< znet::IZNet > createZNet( znet::IProvisioningListProvider& classicInclusionProvider,
                                               znet::IProvisioningListProvider& provider,
                                               znet::IProvisioningListObserver& observer,
                                               znet::IZNetOwner& znetOwner ) const {
        return m_znetCreator( classicInclusionProvider, provider, observer, znetOwner );
    }

private:
    std::function< std::unique_ptr< znet::IZNet >( znet::IProvisioningListProvider& classicInclusionProvider,
                                                   znet::IProvisioningListProvider& provider,
                                                   znet::IProvisioningListObserver& observer,
                                                   znet::IZNetOwner& ) >
        m_znetCreator;
};

}  // namespace Zwave
}  // namespace HubPlatform
