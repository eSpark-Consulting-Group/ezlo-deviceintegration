#pragma once

#include "c_callback_wrapper.h"
#include "base_service.h"

#include <upnp/upnp.h>

class UPnPService final : public BaseService {
public:
    UPnPService( HubPlatform::Addons::Addon& addon, const std::string& webRoot );

    ~UPnPService();

    void start() override;

private:
    int upnpGetDataRequestInfo( IN const char* filename, OUT struct File_Info* info );

    int upnpGetInfo( IN const char* filename, OUT struct File_Info* info );

    UpnpWebFileHandle upnpOpen( IN const char* filename, IN enum UpnpOpenFileMode mode );

    int upnpRead( IN UpnpWebFileHandle fileHnd, OUT char* buf, IN size_t size );

    int upnpWrite( IN UpnpWebFileHandle fileHnd, IN char* buf, IN size_t size );

    int upnpSeek( IN UpnpWebFileHandle fileHnd, IN off_t offset, IN int origin );

    int upnpClose( IN UpnpWebFileHandle fileHnd );

    int upnpEventHandler( Upnp_EventType eventType, void* event, void* coockie );

private:
    UpnpClient_Handle m_clientHandle;
    UpnpDevice_Handle m_deviceHandle;

    CCallbackWrapper< decltype( &UPnPService::upnpOpen ), &UPnPService::upnpOpen > m_upnpOpenWrapper;
    CCallbackWrapper< decltype( &UPnPService::upnpRead ), &UPnPService::upnpRead > m_upnpReadWrapper;
    CCallbackWrapper< decltype( &UPnPService::upnpWrite ), &UPnPService::upnpWrite > m_upnpWriteWrapper;
    CCallbackWrapper< decltype( &UPnPService::upnpSeek ), &UPnPService::upnpSeek > m_upnpSeekWrapper;
    CCallbackWrapper< decltype( &UPnPService::upnpClose ), &UPnPService::upnpClose > m_upnpCloseWrapper;
    CCallbackWrapper< decltype( &UPnPService::upnpGetInfo ), &UPnPService::upnpGetInfo > m_upnpGetInfoWrapper;
    CCallbackWrapper< decltype( &UPnPService::upnpEventHandler ), &UPnPService::upnpEventHandler >
        m_upnpEventHandlerWrapper;
};
