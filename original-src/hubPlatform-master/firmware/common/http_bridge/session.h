#pragma once

#include "http_bridge/command.h"
#include "json_utils/json_fwd.h"
#include <libwebsockets.h>
#include <mutex>
#include <atomic>
#include <deque>

namespace HubPlatform {
namespace HttpBridge {

using Bytes = std::vector< char >;
using HttpBridgeCommand = HttpBridge::HttpBridgeCommand;

struct SessionData {
public:
    using Header = std::pair< std::string, std::string >;
    using Headers = std::vector< Header >;

public:
    explicit SessionData( lws* wsi );
    SessionData( const SessionData& ) = delete;
    SessionData& operator=( const SessionData& ) = delete;

    HttpBridgeCommand& getCommand();
    std::pair< bool, std::string > parseGetCommand( const std::string& url );

    Bytes& getPostData();
    bool parsePostData();
    const Json& postDataJson() const;
    lws* getWsi();

    void setAnswer( Json answer );
    Json getAnswer() const;
    bool notify();
    void isPending( bool v );
    bool isPending() const;
    void finished( bool value );
    bool isFinished() const;
    void isLoggedIn( bool value );
    bool isLoggedIn() const;
    time_t created() const;

    void buildResponse( int indent = -1 );
    void appendBroadcast( std::string msg );

    // looping by serialized answer
    void setSerializedAnswer( std::string value );
    std::string getSerializedAnswer( size_t portionSize );
    bool hasSerializedAnswer() const;
    std::string& getPath();
    Headers& knownHeaders();
    const Headers& knownHeaders() const;
    bool hasHeader( const std::string& name, const std::string& value ) const;
    bool findAuthHeaders( std::string& user, std::string& token );

    bool nextBroadcast();

    Json getRequestByHandler( const std::string& handler );
    void addBytesWritten( size_t value );
    size_t bytesWritten() const;

    void headersSent( bool value );
    bool headersSent() const;
    int delayed() const;
    void delayed( int value );
    std::string peerDebugInfo();

    std::string getHeaderValue( const std::string& name ) const;
    void addHeader( const std::string& name, const std::string& value );
    Json dumpHeaders( bool hidePrivateHeaders );

    void setId( const std::string& id );
    std::string getId() const;

private:
    static void assignMessageId( Json& request, const std::string& id );

private:
    using Mutex = std::recursive_mutex;
    using UniqueLock = std::unique_lock< Mutex >;
    mutable Mutex m_mutex;

    lws* m_wsi;
    std::string m_id;
    std::string m_path;
    Headers m_knownHeaders;

    HttpBridgeCommand m_command;

    // POST request related
    Bytes m_postData;
    Json m_postDataJson;
    Json m_answer;
    std::atomic_bool m_isPending{ false };
    std::atomic_bool m_isFinished{ false };
    std::atomic_bool m_isLoggedIn{ false };
    bool m_headersSent{ false };
    time_t m_created;
    size_t m_bytesWritten{ 0 };

    std::string m_serializedAnswer;
    size_t m_portionStart{ 0 };
    size_t m_portionEnd{ 0 };

    std::deque< std::string > m_broadcasts;
    int m_delayed{ 0 };
};

using SessionDataPtr = std::unique_ptr< SessionData >;

class ApiBridge;

class Session {
public:
    enum class SessionType { HTTP, EVENT_STREAM };

public:
    explicit Session( SessionType type = SessionType::HTTP );
    ~Session();
    Session( const Session& ) = delete;
    Session& operator=( const Session& ) = delete;
    Session( Session&& ) = default;
    Session& operator=( Session&& ) = default;

    unsigned extractHeaders( lws* wsi );
    int extractHeaderTokens( lws* wsi, SessionData& sessionData );
    static int extractUriParams( lws* wsi, HttpBridgeCommand& command );

    void init( lws* wsi );
    void clear();

    SessionData* getData();
    SessionType sessionType() const;
    void sessionType( SessionType value );
    void postMessage( const std::string& msg );

public:
    lws_token_indexes method = WSI_TOKEN_GET_URI;

private:
    SessionDataPtr m_data;
    SessionType m_type{ SessionType::HTTP };
};

using SessionPtr = std::shared_ptr< Session >;

}  // namespace HttpBridge
}  // namespace HubPlatform