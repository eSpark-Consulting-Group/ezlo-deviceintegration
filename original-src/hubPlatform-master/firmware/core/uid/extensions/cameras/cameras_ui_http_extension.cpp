#include "uid/extensions/cameras/cameras_ui_http_extension.h"
#include "uid/extensions/ui_http_utils.h"
#include "common_utils/cpp14.h"
#include "base_utils/ezlodb.h"

namespace HubPlatform {
namespace Cameras {

namespace {
struct AudioStreamRequestUserData : public Uid::Extensions::HttpSession::UserData {
    AudioStreamRequestUserData( std::string streamId, std::string traceId, std::string spanId )
    : streamId( std::move( streamId ) )
    , traceId( std::move( traceId ) )
    , spanId( std::move( spanId ) ) {
        timestamp = time( nullptr );
    }

    time_t getRequestDuration() const {
        auto now = time( nullptr );
        return now - timestamp;
    }

    Json toJson() const {
        Json record = {
            { "streamId", streamId }, { "traceId", traceId }, { "spanId", spanId }, { "duration", getRequestDuration() }
        };
        if( bytesSent > 0 ) {
            record[ "bytesSent" ] = bytesSent;
            record[ "portions" ] = portions;
        }
        if( last ) {
            record[ "last" ] = true;
        }
        return record;
    }

public:
    std::string streamId;
    std::string traceId;
    std::string spanId;
    time_t timestamp{ 0 };

    // progress indication
    size_t bytesSent{ 0 };
    size_t portions{ 0 };
    bool last{ false };
};
}  // namespace

UiHttpExtension::UiHttpExtension( AudioStreamManager& manager )
: m_manager( manager ) {}

// TODO: Make possible emit metrics events from UiHttpExtension endpoints
int UiHttpExtension::onHttp( struct lws* wsi, Uid::Extensions::HttpSession& session ) {
    fetchOtelParams( session );
    std::string otel = ", traceId: " + session.traceId() + ", spanId: " + session.spanId();

    LOGGER_TRACE_RAW( "Input audio stream http request" );

    static std::string audioStreamPrefix = "/stream/audio/";
    if( strncmp( session.url.c_str(), audioStreamPrefix.c_str(), audioStreamPrefix.size() ) != 0 ) {
        return Uid::Extensions::Utils::writeError( wsi, 404, "Cameras addon does not provide specified functionality" );
    }

    std::string streamId = session.url.substr( audioStreamPrefix.size() );
    if( streamId.empty() ) {
        LOGGER_ERROR_RAW( cc::error( "Stream id not specified " ) + otel );
        return Uid::Extensions::Utils::writeError( wsi, 400, "Stream id not specified" );
    }

    LOGGER_INFO_RAW( cc::info( "Got connection for input audio stream " ) + cc::note( streamId ) + otel );

    auto stream = m_manager.getInputAudioStream( streamId );
    if( !stream ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to find input audio stream " ) + cc::note( streamId ) + otel );
        return Uid::Extensions::Utils::writeError( wsi, 404, "Stream with specified id not found" );
    }

    if( !stream->isRunning() ) {
        LOGGER_ERROR_RAW( cc::error( "Requested input audio stream " ) + cc::note( streamId ) +
                          cc::error( " is closed" ) + otel );
        return Uid::Extensions::Utils::writeError( wsi, 400, "Stream is closed" );
    }

    if( !stream->startDataSending() ) {
        LOGGER_ERROR_RAW( cc::error( "Failed to initialize data transmission for input audio stream " ) +
                          cc::note( streamId ) + otel );
        return Uid::Extensions::Utils::writeError( wsi, 400, "Failed to initialize data transmission" );
    }

    session.userData = std::make_unique< AudioStreamRequestUserData >( streamId, session.traceId(), session.spanId() );

    if( Log::getLogLevel() >= Log::PriorityLevel::DEBUG ) {
        Json headersArray = JSON_EMPTY_ARR;
        for( const auto& header : session.headers ) {
            Json record = { { "header", header.first }, { "value", header.second } };
            headersArray.push_back( std::move( record ) );
        }
        LOGGER_DEBUG_RAW(
            cc::debug( "Input audio stream request headers: " ) + json_write( headersArray ) + ", user data: " +
            json_write( dynamic_cast< AudioStreamRequestUserData* >( session.userData.get() )->toJson() ) );
    }

    if( session.hasHeader( "expect", "100-continue" ) ) {
        // Write 100 continue
        uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE ];
        uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - 1 ];
        lws_add_http_header_status( wsi, 100, &p, end );
        lws_finalize_write_http_header( wsi, start, &p, end );
        lws_callback_on_writable( wsi );
    }

    return 0;
}

int UiHttpExtension::onHttpBody( struct lws* wsi,
                                 Uid::Extensions::HttpSession& session,
                                 const char* data,
                                 size_t size ) {
    if( !session.userData )
        return 1;

    auto userDataPtr = dynamic_cast< AudioStreamRequestUserData* >( session.userData.get() );
    if( !userDataPtr )
        return 1;

    auto streamPtr = m_manager.getInputAudioStream( userDataPtr->streamId );
    if( !streamPtr )
        return 1;

    if( !streamPtr->send( data, size ) ) {
        LOGGER_DEBUG_RAW( cc::debug( "Failed to send new data for input audio stream " ) +
                          cc::note( userDataPtr->streamId ) + ", user data: " + cc::j( userDataPtr->toJson() ) );
        return 1;
    }

    // Register progress
    userDataPtr->bytesSent += size;
    userDataPtr->portions++;

    // Print progress message every 500000 of audio stream, it's around 30 seconds
    const auto progressPortion = 500000;
    auto printProgress = userDataPtr->bytesSent / progressPortion > ( userDataPtr->bytesSent - size ) / progressPortion;
    if( printProgress ) {
        LOGGER_DEBUG_RAW( "Input audio stream progress, user data: " + cc::j( userDataPtr->toJson() ) );
    }

    return 0;
}

int UiHttpExtension::onHttpBodyCompletion( struct lws* wsi, Uid::Extensions::HttpSession& session ) {
    if( !session.userData ) {
        return 1;
    }

    auto userDataPtr = dynamic_cast< AudioStreamRequestUserData* >( session.userData.get() );
    if( !userDataPtr )
        return 1;

    userDataPtr->last = true;
    session.addBytesRx( userDataPtr->bytesSent );

    LOGGER_INFO_RAW( cc::debug( "Got all data for input audio stream " ) + cc::note( userDataPtr->streamId ) +
                     ", user data: " + cc::j( userDataPtr->toJson() ) );

    // Write simple 200 response
    uint8_t buf[ LWS_PRE + LWS_RECOMMENDED_MIN_HEADER_SPACE ];
    uint8_t *start = &buf[ LWS_PRE ], *p = start, *end = &buf[ sizeof( buf ) - 1 ];
    lws_add_http_header_status( wsi, 200, &p, end );
    session.responseHeadersWritten = true;
    lws_finalize_write_http_header( wsi, start, &p, end );
    lws_callback_on_writable( wsi );

    return session.hasHeader( "connection", "keep-alive" ) ? 0 : 2;
}

int UiHttpExtension::onHttpClosed( struct lws* wsi, Uid::Extensions::HttpSession& session ) {
    if( !session.userData ) {
        return 1;
    }

    auto userDataPtr = dynamic_cast< AudioStreamRequestUserData* >( session.userData.get() );
    if( !userDataPtr )
        return 1;

    LOGGER_DEBUG_RAW( cc::debug( "Closing connection for input audio stream " ) + cc::note( userDataPtr->streamId ) +
                      ", user data: " + cc::j( userDataPtr->toJson() ) );

    auto streamPtr = m_manager.getInputAudioStream( userDataPtr->streamId );
    if( !streamPtr )
        return 1;

    streamPtr->stopDataSending();

    return 0;
}

int UiHttpExtension::onHttpWriteable( struct lws* wsi, Uid::Extensions::HttpSession& session ) {
    return 0;
}

bool UiHttpExtension::authorizationRequired() const {
    return true;
}

int UiHttpExtension::onTimer( struct lws* wsi, Uid::Extensions::HttpSession& session ) {
    return 0;
}

void UiHttpExtension::fetchOtelParams( Uid::Extensions::HttpSession& session ) {
    bool success = false;
    std::string value;
    std::string spanIdValue;

    std::tie( success, value ) = session.findUriParam( "traceId" );
    if( success )
        session.traceId( value );

    std::tie( success, value ) = session.findUriParam( "spanId" );
    if( success )
        session.spanId( value );

    if( session.spanId().empty() ) {
        session.spanId( "autogen_" + Db::genOid() );
    }
}

}  // namespace Cameras
}  // namespace HubPlatform
