#include "core/pipeline/packet_converter.h"

#include "utils/byte_cast.h"

#include <endian.h>
#include <cassert>

namespace TwoGig {

template < typename To >
PacketConverter< To >::PacketConverter( const Logger& logger )
: PipeProducer< To >( logger ) {}

template < typename To >
void PacketConverter< To >::processPayload( PacketBytes packet ) {
    static_assert( sizeof( To ) == sizeof( uint64_t ), "Unsupported packet size" );

    assert( packet.size() == sizeof( To ) );

    const uint64_t data = be64toh( fromBytes< uint64_t >( packet.data() ) );

    this->dispatchPayload( fromBytes< To >( &data ) );
}

template <>
void PacketConverter< Supervisory >::processPayload( PacketBytes packet ) {
    assert( packet.size() == sizeof( Supervisory ) );

    this->dispatchPayload( fromBytes< Supervisory >( packet.data() ) );
}

template class PacketConverter< Transmission >;
template class PacketConverter< Jamming >;
template class PacketConverter< PowerOn >;
template class PacketConverter< Supervisory >;

}  // namespace TwoGig
