#if( !defined __TINY_NET_IO_H )
#define __TINY_NET_IO_H 1

#include <tiny/tiny_multithreading.h>
#include <tiny/tiny_network.h>
#include <tiny/tiny_dispatch.h>
#include <tiny/tiny_url.h>
#include <thread>
#include <atomic>

//#include <openssl/conf.h>
#include <openssl/ssl.h>
//#if (!defined OPENSSL_NO_ENGINE)
//	# include <openssl/engine.h>
//#endif
//#include <openssl/dh.h>
//#include <openssl/pem.h>
//#include <openssl/err.h>
//#include <openssl/rsa.h>
//#include <openssl/x509.h>
//#include <openssl/x509v3.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <uv.h>

extern "C" {
#include <sys/types.h>
struct addrinfo;
};  /// extern "C"

namespace tiny {
namespace network {
namespace io {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef tiny::dispatch::fn_invoke_t fn_invoke_t;
typedef std::function< void( const uint8_t* p, size_t cnt ) > fn_io_t;
typedef std::function< void( const std::string& s ) > fn_message_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace options {

class basic {
public:
    int ipVersion_;  // 4 or 6
    bool secure_;    // use SSL
    bool own_loop_;  // run own UV loop in own thread
    uv_loop_t* loop_;
    basic& clear() {
        ipVersion_ = 4;    // 4 or 6
        secure_ = false;   // use SSL
        own_loop_ = true;  // run own UV loop in own thread
        loop_ = nullptr;
        return ( *this );
    }
    int compare( const basic& other ) const {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return 0;
        if( ipVersion_ < other.ipVersion_ )
            return -1;
        if( ipVersion_ > other.ipVersion_ )
            return 1;
        if( ( !secure_ ) && other.secure_ )
            return -1;
        if( secure_ && ( !other.secure_ ) )
            return 1;
        if( ( !own_loop_ ) && other.own_loop_ )
            return -1;
        if( own_loop_ && ( !other.own_loop_ ) )
            return 1;
        // loop_ is not compared
        return 0;
    }
    basic& assign( const basic& other ) {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return ( *this );
        clear();
        ipVersion_ = other.ipVersion_;
        secure_ = other.secure_;
        own_loop_ = other.own_loop_;
        loop_ = other.loop_;
        return ( *this );
    }
    basic& move( basic& other ) {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return ( *this );
        assign( other );
        other.clear();
        return ( *this );
    }
    basic() {
        clear();
    }
    basic( const basic& other ) {
        assign( other );
    }
    basic( basic&& other ) {
        move( other );
    }
    virtual ~basic() {
        clear();
    }
    basic& operator=( const basic& other ) {
        return assign( other );
    }
    basic& operator=( basic&& other ) {
        return move( other );
    }
    bool operator==( const basic& other ) const {
        return ( compare( other ) == 0 ) ? true : false;
    }
    bool operator!=( const basic& other ) const {
        return ( compare( other ) != 0 ) ? true : false;
    }
    bool operator<( const basic& other ) const {
        return ( compare( other ) < 0 ) ? true : false;
    }
    bool operator<=( const basic& other ) const {
        return ( compare( other ) <= 0 ) ? true : false;
    }
    bool operator>( const basic& other ) const {
        return ( compare( other ) > 0 ) ? true : false;
    }
    bool operator>=( const basic& other ) const {
        return ( compare( other ) >= 0 ) ? true : false;
    }
    static uv_loop_t* stat_get_external_dispatch_loop_handle();
    uv_loop_t* get_effective_loop_handle();
};  /// class basic

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class client : public basic {
public:
    tiny::url url_;
    std::string strKeyFile_;
    client& clear() {
        basic::clear();
        url_.clear();
        strKeyFile_.clear();
        return ( *this );
    }
    int compare( const client& other ) const {
        int n = basic::compare( other );
        if( n )
            return n;
        n = url_.compare( other.url_ );
        if( n )
            return n;
        n = strKeyFile_.compare( other.strKeyFile_ );
        if( n )
            return n;
        return 0;
    }
    client& assign( const client& other ) {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return ( *this );
        basic::assign( other );
        url_ = other.url_;
        return ( *this );
    }
    client& move( client& other ) {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return ( *this );
        assign( other );
        other.clear();
        return ( *this );
    }
    client() {
        clear();
    }
    client( const client& other ) {
        assign( other );
    }
    client( client&& other ) {
        move( other );
    }
    virtual ~client() {
        clear();
    }
    client& operator=( const client& other ) {
        return assign( other );
    }
    client& operator=( client&& other ) {
        return move( other );
    }
    bool operator==( const client& other ) const {
        return ( compare( other ) == 0 ) ? true : false;
    }
    bool operator!=( const client& other ) const {
        return ( compare( other ) != 0 ) ? true : false;
    }
    bool operator<( const client& other ) const {
        return ( compare( other ) < 0 ) ? true : false;
    }
    bool operator<=( const client& other ) const {
        return ( compare( other ) <= 0 ) ? true : false;
    }
    bool operator>( const client& other ) const {
        return ( compare( other ) > 0 ) ? true : false;
    }
    bool operator>=( const client& other ) const {
        return ( compare( other ) >= 0 ) ? true : false;
    }
};  /// class client

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class server : public basic {
public:
    int port_;  // listen on port, -1 is not initialized
    size_t maxConnections_;
    bool generate_and_use_self_signed_;
    std::string strCertFile_;
    std::string strKeyFile_;
    server& clear() {
        basic::clear();
        port_ = -1;  // listen on port, -1 is not initialized
        maxConnections_ = 20 * 1000;
        generate_and_use_self_signed_ = false;
        strCertFile_.clear();
        strKeyFile_.clear();
        return ( *this );
    }
    int compare( const server& other ) const {
        int n = basic::compare( other );
        if( n )
            return n;
        if( port_ < other.port_ )
            return -1;
        if( port_ > other.port_ )
            return 1;
        if( maxConnections_ < other.maxConnections_ )
            return -1;
        if( maxConnections_ > other.maxConnections_ )
            return 1;
        if( ( !generate_and_use_self_signed_ ) && other.generate_and_use_self_signed_ )
            return -1;
        if( generate_and_use_self_signed_ && ( !other.generate_and_use_self_signed_ ) )
            return 1;
        n = strCertFile_.compare( other.strCertFile_ );
        if( n )
            return n;
        n = strKeyFile_.compare( other.strKeyFile_ );
        if( n )
            return n;
        return 0;
    }
    server& assign( const server& other ) {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return ( *this );
        basic::assign( other );
        port_ = other.port_;
        maxConnections_ = other.maxConnections_;
        return ( *this );
    }
    server& move( server& other ) {
        if( ( (void*) ( this ) ) == ( (void*) ( &other ) ) )
            return ( *this );
        assign( other );
        other.clear();
        return ( *this );
    }
    server() {
        clear();
    }
    server( const server& other ) {
        assign( other );
    }
    server( server&& other ) {
        move( other );
    }
    virtual ~server() {
        clear();
    }
    server& operator=( const server& other ) {
        return assign( other );
    }
    server& operator=( server&& other ) {
        return move( other );
    }
    bool operator==( const server& other ) const {
        return ( compare( other ) == 0 ) ? true : false;
    }
    bool operator!=( const server& other ) const {
        return ( compare( other ) != 0 ) ? true : false;
    }
    bool operator<( const server& other ) const {
        return ( compare( other ) < 0 ) ? true : false;
    }
    bool operator<=( const server& other ) const {
        return ( compare( other ) <= 0 ) ? true : false;
    }
    bool operator>( const server& other ) const {
        return ( compare( other ) > 0 ) ? true : false;
    }
    bool operator>=( const server& other ) const {
        return ( compare( other ) >= 0 ) ? true : false;
    }
};  /// class server

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

};  // namespace options

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class error_interface {
public:
    fn_message_t on_error_;
    error_interface();
    virtual ~error_interface();
    error_interface( const error_interface& ) = delete;
    error_interface( error_interface&& ) = delete;
    error_interface& operator=( const error_interface& ) = delete;
    error_interface& operator=( error_interface&& ) = delete;
    virtual void on_error( const std::string& strErrorDescription );
};  /// class error_interface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class loop_interface {
    volatile bool loop_interface_initialized_ = false;
    volatile bool own_loop_ = false;      // run own UV loop in own thread
    volatile uv_loop_t* loop_ = nullptr;  // own or external loop
    std::thread loop_thread_;
    typedef tiny::multithreading::recursive_mutex_type mutex_type;
    typedef std::lock_guard< mutex_type > lock_type;
    mutex_type loop_interface_mtx_;
    volatile bool own_loop_is_running_ = false;
    volatile bool own_loop_needs_shutdown_ = false;
    volatile bool own_loop_alive_ = false;

public:
    loop_interface();
    virtual ~loop_interface();
    loop_interface( const loop_interface& ) = delete;
    loop_interface( loop_interface&& ) = delete;
    loop_interface& operator=( const loop_interface& ) = delete;
    loop_interface& operator=( loop_interface&& ) = delete;
    void loop_interface_start( tiny::network::io::options::basic& opts );
    void loop_interface_stop( bool bWait );
    virtual bool loop_interface_needs_shutdown();
    uv_loop_s* get_effective_loop_handle();
    virtual void on_own_loop_idle();
    virtual void on_own_loop_state_check( bool isIdleCall );
    void invoke_in_loop( fn_invoke_t fn );
};  /// class loop_interface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ssl_trace_interface {
public:
    ssl_trace_interface();
    virtual ~ssl_trace_interface();
    ssl_trace_interface( const ssl_trace_interface& ) = delete;
    ssl_trace_interface( ssl_trace_interface&& ) = delete;
    ssl_trace_interface& operator=( const ssl_trace_interface& ) = delete;
    ssl_trace_interface& operator=( ssl_trace_interface&& ) = delete;
    //
    fn_message_t on_ssl_trace_message_;
    virtual void on_ssl_trace_message( const std::string& s );
    virtual void on_ssl_trace( int write_p, int version, int content_type, const void* buf, size_t len, SSL* ssl );
};  /// class ssl_trace_interface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class pipe_stream_interface : public error_interface, public ssl_trace_interface {
    std::vector< uint8_t > buffer_in_;   // app data in
    std::vector< uint8_t > buffer_out_;  // app data out
    BIO* read_bio_ = nullptr;
    BIO* write_bio_ = nullptr;
    //
public:
    pipe_stream_interface();
    virtual ~pipe_stream_interface();
    pipe_stream_interface( const pipe_stream_interface& ) = delete;
    pipe_stream_interface( pipe_stream_interface&& ) = delete;
    pipe_stream_interface& operator=( const pipe_stream_interface& ) = delete;
    pipe_stream_interface& operator=( pipe_stream_interface&& ) = delete;
    //
    virtual uv_stream_t* get_stream_handle() = 0;
    virtual void invoke_in_loop( fn_invoke_t fn ) = 0;
    //
protected:
    virtual void init_shutdown( uv_stream_t* p_uv_stream );

public:
    virtual void on_shutdown();
    virtual void on_closed();
    virtual SSL* get_ssl() = 0;
    //
    typedef std::function< void( const uint8_t* p, size_t cnt ) > fn_on_read_t;
    fn_on_read_t on_read_;
    virtual void on_read_ll( const uint8_t* p, size_t cnt );
    virtual void on_read( const uint8_t* p, size_t cnt );
    virtual void write_ll_ssl( uv_stream_t* p_uv_stream, const uint8_t* p, size_t cnt );  // low-level API
    virtual void write_ll( uv_stream_t* p_uv_stream, const uint8_t* p, size_t cnt );      // low-level API
    virtual void write( const uint8_t* p, size_t cnt ) = 0;
    //
    void configure_ssl_tracing();
    void ssl_flush_read_bio();
    void ssl_check_outgoing_application_data();
    void ssl_handle_error( int result );
    void ssl_show_error();
    void ssl_show_error( int result );
    void configure_ssl_bio();
    void ssl_read_event();
};  /// class pipe_stream_interface

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class client : public pipe_stream_interface, public loop_interface {
public:
    typedef tiny::network::io::options::client options_t;
    options_t opts_;
    SSL_CTX* ssl_ctx_;
    //
    enum class state_t {
        state_disconnected,
        state_connected,
        state_resolving,
        state_connecting,
        state_disconnecting,
        state_initializing_ssl,
    };  /// enum class state_t
private:
    state_t state_ = state_t::state_disconnected;
    uv_getaddrinfo_t addr_req_;
    uv_connect_t connect_req_;
    uv_tcp_t handle_;
    SSL* ssl_;

public:
    client( const options_t& opts );
    virtual ~client();
    client( const client& ) = delete;
    client( client&& ) = delete;
    client& operator=( const client& ) = delete;
    client& operator=( client&& ) = delete;
    fn_invoke_t on_instance_destroy_;
    fn_invoke_t on_state_changed_;
    virtual state_t state() const;
    virtual client& state( const state_t st );
    static std::string stat_state_as_str( const state_t st );
    std::string state_as_str() const;
    virtual bool connect();
    virtual bool disconnect( bool bWait = true );
    virtual void on_resolved( uv_getaddrinfo_t* req, int status, struct ::addrinfo* res );
    fn_invoke_t on_connected_;
    virtual void on_connected( uv_connect_t* req, int status );
    bool loop_interface_needs_shutdown() override;
    //
    uv_stream_t* get_stream_handle() override;
    void invoke_in_loop( fn_invoke_t fn ) override;
    //
public:
    fn_invoke_t on_shutdown_;
    fn_invoke_t on_closed_;
    void on_shutdown() override;
    void on_closed() override;
    //
    void on_read( const uint8_t* p, size_t cnt ) override;
    void write( const uint8_t* p, size_t cnt ) override;
    SSL* get_ssl() override;
};  /// class client

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class server;

class peer : public pipe_stream_interface {
public:
    enum class state_t {
        state_disconnected,
        state_connected,
        state_initializing,
        state_disconnecting,
        state_initializing_ssl,
    };  /// enum class state_t
private:
    state_t state_ = state_t::state_initializing;
    server& srv_;
    uv_tcp_t handle_;
    SSL* ssl_;

public:
    peer( server& srv );
    virtual ~peer();
    peer( const peer& ) = delete;
    peer( peer&& ) = delete;
    peer& operator=( const peer& ) = delete;
    peer& operator=( peer&& ) = delete;
    bool handle_peer_initialization_steps( uv_stream_t* uvs );
    fn_invoke_t on_instance_destroy_;
    fn_invoke_t on_state_changed_;
    virtual state_t state() const;
    virtual peer& state( const state_t st );
    static std::string stat_state_as_str( const state_t st );
    std::string state_as_str() const;
    virtual bool disconnect( bool bWait = true );
    //
    uv_stream_t* get_stream_handle() override;
    void invoke_in_loop( fn_invoke_t fn ) override;
    //
public:
    fn_invoke_t on_shutdown_;
    fn_invoke_t on_closed_;
    void on_shutdown() override;
    void on_closed() override;
    //
    void on_read( const uint8_t* p, size_t cnt ) override;
    void write( const uint8_t* p, size_t cnt ) override;
    SSL* get_ssl() override;
};  /// class peer

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class server : public loop_interface, public error_interface, public ssl_trace_interface {
public:
    typedef tiny::network::io::options::server options_t;
    options_t opts_;
    SSL_CTX* ssl_ctx_;
    enum class state_t {
        state_stopped,
        state_running,
        state_starting,
        state_stopping,
        state_creating_self_signed,
        state_initializing_ssl,
    };  /// enum class state_t
private:
    state_t state_ = state_t::state_stopped;
    uv_tcp_t listener_;

public:
    server( const options_t& opts );
    virtual ~server();
    server( const server& ) = delete;
    server( server&& ) = delete;
    server& operator=( const server& ) = delete;
    server& operator=( server&& ) = delete;
    fn_invoke_t on_instance_destroy_;
    fn_invoke_t on_state_changed_;
    virtual state_t state() const;
    virtual server& state( const state_t st );
    static std::string stat_state_as_str( const state_t st );
    std::string state_as_str() const;
    fn_invoke_t on_started_;
    fn_invoke_t on_stopped_;
    virtual bool start();
    virtual bool stop( bool bWait );
    bool loop_interface_needs_shutdown() override;
    fn_invoke_t on_shutdown_;
    fn_invoke_t on_closed_;
    virtual void on_shutdown();
    virtual void on_closed();
    typedef std::function< void( peer& refPeer ) > fn_on_peer_connected_t;
    fn_on_peer_connected_t on_peer_instantiated_;
    fn_on_peer_connected_t on_peer_connected_;
    virtual bool on_peer_connected( uv_stream_t* uvs, int status );
};  /// class server

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

};  // namespace io
};  // namespace network
};  // namespace tiny

#endif  /// (!defined __TINY_NET_IO_H)
