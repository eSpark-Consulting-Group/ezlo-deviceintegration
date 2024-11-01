#include "bluetooth/utils/io_pipe.h"

namespace HubPlatform {

IOPipe::IOPipe( int fd )
: m_channel( nullptr, g_io_channel_unref ) {
    m_channel.reset( g_io_channel_unix_new( fd ) );
    g_io_channel_set_encoding( m_channel.get(), nullptr, nullptr );
    g_io_channel_set_buffered( m_channel.get(), FALSE );
    g_io_channel_set_close_on_unref( m_channel.get(), TRUE );
}

IOPipe::~IOPipe() = default;

void IOPipe::setReadHandler( IOHandler readHandler ) {
    m_readWatch.reset( new IOWatch( m_channel.get(), std::move( readHandler ), G_IO_IN ) );
}

void IOPipe::setDisconnectHandler( IOHandler disconnectHandler ) {
    m_disconnectWatch.reset( new IOWatch( m_channel.get(), std::move( disconnectHandler ), G_IO_HUP ) );
}

IOPipe::IOWatch::IOWatch( GIOChannel* channel, IOPipe::IOHandler handler, GIOCondition condition )
: m_ioHandler( std::move( handler ) ) {
    const auto priority = ( condition == G_IO_HUP ) ? G_PRIORITY_DEFAULT_IDLE : G_PRIORITY_DEFAULT;
    m_id = g_io_add_watch_full(
        channel, priority, static_cast< GIOCondition >( condition | G_IO_ERR | G_IO_NVAL ), callback, this, nullptr );
}

IOPipe::IOWatch::~IOWatch() {
    g_source_remove( m_id );
}

gboolean IOPipe::IOWatch::callback( GIOChannel* /*channel*/, GIOCondition condition, gpointer userData ) {
    if( condition & ( G_IO_ERR | G_IO_NVAL ) ) {
        return FALSE;
    }

    static_cast< IOWatch* >( userData )->m_ioHandler();

    return TRUE;
}
}  // namespace HubPlatform
