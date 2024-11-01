#pragma once

#include <functional>
#include <string>
#include <tiny/tiny_ws.h>

static bool g_bIsStopEzloHubEmotionsOnShutdown = false;
static std::function< void() > g_fn_on_shutdown_interrupt_nma_server_;

void stat_classified_msg( const std::string& strClassName, const std::string& s );
void stat_startup_msg( const std::string& s );
void stat_shutdown_msg( const std::string& s );
void stat_info_msg( const std::string& s );
void stat_error_msg( const std::string& s );
void stat_setup_emotions_on_stop();
void stat_handle_shutdown( int* p_nSignalNo = nullptr  // if null-at exit call, otherwise signal
);
void stat_handle_at_exit();
void stat_handle_signal( int nSignalNo );
void stat_handle_async_mechanism_error( tiny::ws::generic_sender& ws,
                                        const std::string& msg,
                                        const std::string& s1,
                                        const std::string& s2 );
