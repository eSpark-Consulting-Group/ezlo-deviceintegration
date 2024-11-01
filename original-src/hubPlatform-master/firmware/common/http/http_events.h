#pragma once
#include "http/http_options.h"
#include "http/http_event.h"
#include "http/resources.h"
#include <json_utils/json_fwd.h>
#include <string>

namespace HubPlatform {
namespace CommonHttp {

class DataSendEvent;
class DataReceivedEvent;
class ConnectionClosedEvent;
class ProgressEvent;

class EventVisitor {
public:
    virtual ~EventVisitor() = default;

    virtual void visit( DataSendEvent& value ){};
    virtual void visit( DataReceivedEvent& value ){};
    virtual void visit( ConnectionClosedEvent& value ){};
    virtual void visit( ProgressEvent& value ){};
};

class DataSendEvent final : public Event {
public:
    DataSendEvent() = default;
    DataSendEvent( const std::string& id, const std::string& url, const Json& userData ) noexcept;

    Json toJson() const override;
    void fromJson( const Json& ) override;
    void accept( EventVisitor& visitor ) override;

    int maxDataLength() const;
    DataSendEvent& maxDataLength( int value );

private:
    int m_maxDataLength = Limits::SEND_DATA_MAX_DATA_LENGTH;
};

class DataReceivedEvent : public Event {
public:
    DataReceivedEvent() = default;
    DataReceivedEvent( const std::string& id,
                       const std::string& url,
                       const Json& userData,
                       bool multiHeaders = false ) noexcept;

    Json toJson() const override;
    void fromJson( const Json& ) override;
    void accept( EventVisitor& visitor ) override;

    void serializeHeaders( Json& headers ) const;

public:
    DataReceivedEvent& code( int code );
    DataReceivedEvent& headers( Headers&& headers );
    DataReceivedEvent& headers( const Headers& headers );
    DataReceivedEvent& data( std::string&& data );
    DataReceivedEvent& data( const std::string& data );
    DataReceivedEvent& last( bool last );
    DataReceivedEvent& multipleHeaders( bool value );

    int code() const;
    const Headers& headers() const;
    const std::string& data() const;
    bool last() const;
    bool multipleHeaders() const;

private:
    int m_code = 0;
    Headers m_headers;
    std::string m_data;
    bool m_last = true;
    bool m_multipleHeaders = false;

    Json serializeHeaders() const;
};

enum class Reason {
    NONE = 0,
    DNS_NAME = 1,
    CONNECTION_ERROR = 2,
    SSL_CONNECTION_ERROR = 3,
    CLOSED_BY_CLIENT = 4,
    CLOSED_BY_SERVER = 5,
    RESPONSE_TIMEOUT_REACHED = 6,
    UNKNOWN_ERROR = 7,

    TOO_MANY_REDIRECTS = 8,

    END_OF_REASONS
};
int toInt( Reason value );

class ConnectionClosedEvent final : public Event {
public:
    ConnectionClosedEvent( const std::string& id, const std::string& url, const Json& userData ) noexcept;
    ConnectionClosedEvent& setReason( Reason );
    ConnectionClosedEvent& reasonText( std::string&& text );
    ConnectionClosedEvent& reasonText( const std::string& text );

    Json toJson() const override;
    void fromJson( const Json& ) override;
    void accept( EventVisitor& visitor ) override;

public:
    int reasonCode() const;
    const std::string& reasonText() const;
    static const char* reasonName( Reason reason );

    void httpStatus( int code, const std::string& status );
    int httpCode() const;

    const std::string& httpCodeStatus() const;

private:
    int m_reasonCode = 0;
    std::string m_reasonText;

    int m_code = 0;            //!< HTTP status code returned by a server, optional
    std::string m_codeStatus;  //!< HTTP status message/reason text, optional
};

class ProgressEvent final : public Event {
public:
    ProgressEvent() = default;
    ProgressEvent( const std::string& id, int progress ) noexcept;

    Json toJson() const override;
    void fromJson( const Json& ) override;
    void accept( EventVisitor& visitor ) override;

    int progress() const;
    ProgressEvent& setProgress( int value );

private:
    int m_progress = 0;
};

namespace Fields {
extern const char* const code;
extern const char* const text;
extern const char* const reason;
extern const char* const codeStatus;
extern const char* const data;
extern const char* const last;
extern const char* const headers;
extern const char* const maxDataLength;
extern const char* const progress;
extern const char* const multipleHeaders;
}  // namespace Fields

}  // namespace CommonHttp
}  // namespace HubPlatform
