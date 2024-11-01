#include "upnp_service.h"

#include <stdexcept>
#include <cstring>
#include <stdlib.h>

auto RootDeviceDescriptionXml = "luaupnp.xml";
auto UpnpServiceBindIp = "0.0.0.0";

UPnPService::UPnPService( HubPlatform::Addons::Addon& addon, const std::string& webRoot )
: BaseService( addon, "49451", webRoot )
, m_clientHandle( -1 )
, m_deviceHandle( -1 )
, m_upnpOpenWrapper( this )
, m_upnpReadWrapper( this )
, m_upnpWriteWrapper( this )
, m_upnpSeekWrapper( this )
, m_upnpCloseWrapper( this )
, m_upnpGetInfoWrapper( this )
, m_upnpEventHandlerWrapper( this ) {}

UPnPService::~UPnPService() {
    UpnpFinish();
}

void UPnPService::start() {
    if( UpnpInit( UpnpServiceBindIp, strtol( getPort().c_str(), NULL, 10 ) ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpInit failed" );
    }

    UpnpVirtualDirCallbacks callbacks;
    callbacks.get_info = m_upnpGetInfoWrapper;
    callbacks.open = m_upnpOpenWrapper;
    callbacks.read = m_upnpReadWrapper;
    callbacks.write = m_upnpWriteWrapper;
    callbacks.seek = m_upnpSeekWrapper;
    callbacks.close = m_upnpCloseWrapper;

    if( UpnpSetVirtualDirCallbacks( &callbacks ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpSetVirtualDirCallbacks failed" );
    }

    if( UpnpAddVirtualDir( LuaDir.c_str() ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpAddVirtualDir failed" );
    }

    if( UpnpAddVirtualDir( DataRequestDir.c_str() ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpAddVirtualDir failed" );
    }

    if( UpnpSetWebServerRootDir( getWebRoot().c_str() ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpSetWebServerRootDir failed" );
    }

    const auto docUrl = std::string( "http://" ) + UpnpGetServerIpAddress() + ':' +
                        std::to_string( UpnpGetServerPort() ) + '/' + RootDeviceDescriptionXml;

    if( UpnpRegisterRootDevice( docUrl.c_str(), m_upnpEventHandlerWrapper, NULL, &m_deviceHandle ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpRegisterRootDevice failed" );
    }

    if( UpnpRegisterClient( m_upnpEventHandlerWrapper, NULL, &m_clientHandle ) != UPNP_E_SUCCESS ) {
        throw std::runtime_error( "UpnpRegisterClient failed" );
    }
}

int UPnPService::upnpEventHandler( Upnp_EventType eventType, void* event, void* coockie ) {
    return 0;
}

int UPnPService::upnpGetDataRequestInfo( IN const char* filename, OUT struct File_Info* info ) {
    return 0;
}

int UPnPService::upnpGetInfo( IN const char* filename, OUT struct File_Info* info ) {
    return 0;
}

UpnpWebFileHandle UPnPService::upnpOpen( IN const char* filename, IN enum UpnpOpenFileMode mode ) {
    return NULL;
}

int UPnPService::upnpRead( IN UpnpWebFileHandle fileHnd, OUT char* buf, IN size_t size ) {
    return 0;
}

int UPnPService::upnpWrite( IN UpnpWebFileHandle fileHnd, IN char* buf, IN size_t size ) {
    return 0;
}

int UPnPService::upnpSeek( IN UpnpWebFileHandle fileHnd, IN off_t offset, IN int origin ) {
    return -1;
}

int UPnPService::upnpClose( IN UpnpWebFileHandle fileHnd ) {
    return -1;
}
