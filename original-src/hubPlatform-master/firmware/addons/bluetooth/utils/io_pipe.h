#pragma once

#include <functional>
#include <memory>

#include <glib.h>

namespace HubPlatform {

class IOPipe {
    using IOHandler = std::function< void() >;

public:
    explicit IOPipe( int fd );
    ~IOPipe();

    void setReadHandler( IOHandler readHandler );
    void setDisconnectHandler( IOHandler disconnectHandler );

private:
    class IOWatch {
    public:
        IOWatch( GIOChannel* channel, IOHandler handler, GIOCondition condition );
        ~IOWatch();

    private:
        static gboolean callback( GIOChannel* channel, GIOCondition condition, gpointer userData );

    private:
        guint m_id;
        IOHandler m_ioHandler;
    };

private:
    std::unique_ptr< GIOChannel, decltype( &g_io_channel_unref ) > m_channel;
    std::unique_ptr< IOWatch > m_readWatch;
    std::unique_ptr< IOWatch > m_disconnectWatch;
};

}  // namespace HubPlatform
