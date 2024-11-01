#include "cameras/remote_interface/cameras_remote_interface.h"
#include "addon_interfaces/addon.h"
#include "cameras/common_res.h"
#include "jsonrpc/jsonrpc.h"

namespace HubPlatform {
namespace CamerasRemoteInterface {

CamerasRemoteInterface::CamerasRemoteInterface( Services::Service& hostService )
: m_hostService( hostService )
, m_groupName( Addons::Addon::createQualifiedName( CamerasAddonName ) ) {}

void CamerasRemoteInterface::getStreamSnapshot( const std::string& streamId,
                                                CamerasRemoteInterface::Callback callback ) const {
    m_hostService.sendAsyncRequest( m_groupName,
                                    JSON( "streamId" << streamId ),
                                    GET_STREAM_SNAPSHOT,
                                    [callback]( const Json& message ) { callback( message ); },
                                    [callback]() {
                                        callback( JSON( "error"
                                                        << "timeout" ) );
                                    },
                                    std::chrono::seconds( 60 ) );
}

}  // namespace CamerasRemoteInterface
}  // namespace HubPlatform
