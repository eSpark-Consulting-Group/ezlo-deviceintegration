#pragma once
#include "base_utils/runnable.h"
#include <thread>
#include <libwebsockets.h>

namespace HubPlatform {
namespace Tests {

class Configuration;

class HttpServer : public common::Runnable {
public:
public:
    HttpServer( /** server context */ );
    HttpServer( const HttpServer& ) = delete;
    HttpServer( HttpServer&& ) = default;
    HttpServer& operator=( const HttpServer& ) = delete;
    HttpServer& operator=( HttpServer&& ) = default;
    ~HttpServer() override;

    void configure( const Configuration& conf );

    void startInternal() override;
    void stopInternal() override;

    unsigned getPort() const;
    void setPort( unsigned port );
    unsigned getSecurePort() const;
    void setSecurePort( unsigned port );

private:
    void init();
    void runServer();

private:
    mutable std::thread m_thread;
    unsigned m_port = 7683;
    unsigned m_securePort = 7684;

    class Impl;
    Impl* m_pimpl;
};

}  // namespace Tests
}  // namespace HubPlatform