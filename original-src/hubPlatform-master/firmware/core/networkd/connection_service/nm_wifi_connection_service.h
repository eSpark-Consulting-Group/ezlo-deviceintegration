#pragma once

#include <string>

struct mg_context;
struct mg_connection;
struct mg_request_info;

namespace HubPlatform {
namespace Network {

class WifiConnectionService final {
public:
    enum class ConnectionStatus { None, Connected, InProgress, Error };

    class ConnectionDelegate {
    public:
        virtual ~ConnectionDelegate() = default;

        virtual bool scanWifi( std::string& jsonResult ) = 0;
        virtual void connect( std::string ssid, std::string bssid, std::string encryption, std::string key ) = 0;
        virtual bool isWifiInternetAvailable() const = 0;
        virtual void temporaryShutDownAP() = 0;
        virtual void restart() = 0;
    };

    explicit WifiConnectionService( ConnectionDelegate& connectionDelegate,
                                    std::string webRoot = "root",
                                    std::string ports = "54540" );
    ~WifiConnectionService();

    WifiConnectionService( const WifiConnectionService& ) = delete;
    WifiConnectionService& operator=( const WifiConnectionService& ) = delete;

    void start() noexcept;
    void stop() noexcept;

    bool isStarted() const noexcept;

private:
    void startImpl() noexcept;
    void stopImpl() noexcept;

private:
    static void handleDataRequest( mg_connection* connection, const mg_request_info* ri, void* data );
    void send200JsonAnswer( mg_connection* connection, int contentLength );
    void send400BadRequestAnswer( mg_connection* connection );
    void send404NotFoundAnswer( mg_connection* connection );
    void send500InternalErrorAnswer( mg_connection* connection );

    void handleGetListRequest( mg_connection* connection );
    void handleGetStatusRequest( mg_connection* connection );
    void handleConnectionRequest( mg_connection* connection, const std::string& postData );
    void handleUndefinedRequest( mg_connection* connection );
    void handleCloseRequest( mg_connection* connection );
    void handleRestartRequest( mg_connection* connection );

private:
    ConnectionDelegate& m_connectionDelegate;
    std::string m_webRoot;
    std::string m_ports;
    ConnectionStatus m_status;
    int m_lastConnectionResult;

    mg_context* m_context = nullptr;
    bool m_enabled = false;
};

}  // namespace Network
}  // namespace HubPlatform
