#include "2gig/events/device_added.h"
#include "2gig/resources/resources.h"

#include "2gig/resources/enum_utils.h"

namespace HubPlatform {
namespace TwoGig {

DeviceAdded::DeviceAdded( const ::TwoGig::DeviceDescriptor& descriptor )
: Services::ScriptEvent( Resources::Events::DeviceAdded )
, m_descriptor( descriptor ) {}

Json DeviceAdded::toJson() const {
    auto result = Services::ScriptEvent::toJson();

    result[ Resources::TxIds ] = m_descriptor.getTxIds();

    switch( m_descriptor.getType() ) {
    case ::TwoGig::DeviceDescriptor::DescriptorType::Model:
        result[ Resources::Events::Parameters::Model ] = m_descriptor.getModel();
        break;

    case ::TwoGig::DeviceDescriptor::DescriptorType::DeviceType:
        result[ Resources::Events::Parameters::DeviceType ] =
            Resources::EnumUtils< ::TwoGig::DeviceType >::toString( m_descriptor.getDeviceType() );
        break;

    default:
        break;
    }

    return result;
}

}  // namespace TwoGig
}  // namespace HubPlatform
