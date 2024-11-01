#include "camera_test/stream_tester.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}
#include <memory>

StreamTester::StreamTester( std::string url,
                            std::chrono::seconds connectionDuration,
                            std::chrono::milliseconds packetDurationThreshold,
                            OnPacketDurationThresholdExceeded packetDurationCallback )
: m_url( std::move( url ) )
, m_connectionDuration( connectionDuration )
, m_packetDurationThreshold( packetDurationThreshold )
, m_onPacketDurationCallback( std::move( packetDurationCallback ) ) {}

StreamTester::~StreamTester() {
    if( m_inputContext ) {
        avformat_close_input( &m_inputContext );
    }
}

StreamTester::TestResult StreamTester::runTest() {
    using namespace std::chrono;
    using Clock = steady_clock;

    const Clock::time_point testStartTimePoint = Clock::now();

    TestResult result;
    result.hasError = !initStream( result.errorMessage );
    if( result.hasError ) {
        return result;
    }

    const Clock::time_point streamOpenTimePoint = Clock::now();
    result.statistic.handshakeTime = duration_cast< milliseconds >( streamOpenTimePoint - testStartTimePoint );

    bool readVideoPacket = false;

    using AVPacketDeleter = void ( * )( AVPacket* );
    using AVPacketPtr = std::unique_ptr< AVPacket, AVPacketDeleter >;

    // Cleans up packet when out of scope.
    auto packetDeleter = []( AVPacket* packetPtr ) -> void {
        if( packetPtr )
            av_packet_unref( packetPtr );
    };

    AVPacket packet;
    Clock::duration startTimeDiff = Clock::now() - streamOpenTimePoint;
    while( startTimeDiff < m_connectionDuration ) {
        const int readFrameRes = av_read_frame( m_inputContext, &packet );
        if( readFrameRes < 0 ) {
            const auto streamReadingDuration = duration_cast< milliseconds >( startTimeDiff );
            result.hasError = true;
            result.errorMessage = "Can't read frame at time " + std::to_string( streamReadingDuration.count() ) + " ms";
            break;
        }

        AVPacketPtr packetUnrefGuard( &packet, packetDeleter );

        result.statistic.packetsCount++;
        startTimeDiff = Clock::now() - streamOpenTimePoint;

        const bool isKeyPacket = m_videoStreamIdx == packet.stream_index && packet.flags == AV_PKT_FLAG_KEY;
        if( !readVideoPacket && isKeyPacket ) {
            readVideoPacket = true;

            result.statistic.keyPacketTime = duration_cast< milliseconds >( startTimeDiff );
            result.statistic.firstVideoPacketIsKey = isKeyPacket;
        }

        processPacketDuration( packet );
    }

    avformat_close_input( &m_inputContext );

    return result;
}

bool StreamTester::initStream( std::string& errorMessage ) {
    AVDictionary* streamOptions = nullptr;
    av_dict_set( &streamOptions, "rtsp_transport", "tcp", 0 );
    av_dict_set( &streamOptions, "max-delay", "10", 0 );
    av_dict_set( &streamOptions, "stimeout", "5000000", 0 );    // in microsecond
    av_dict_set( &streamOptions, "analyzeduration", "50", 0 );  // in microsecond
    av_dict_set( &streamOptions, "probesize", "32", 0 );        // in microsecond

    AVFormatContext* inputContext = nullptr;
    const int formatOpenRes = avformat_open_input( &inputContext, m_url.c_str(), nullptr, &streamOptions );
    if( formatOpenRes != 0 ) {
        constexpr size_t bufferSize = 128;
        char buffer[ bufferSize ];
        av_strerror( formatOpenRes, buffer, bufferSize );

        av_dict_free( &streamOptions );
        streamOptions = nullptr;
        avformat_close_input( &inputContext );

        errorMessage = "Cannot open input: " + std::string( buffer );
        return false;
    }

    av_dict_free( &streamOptions );
    streamOptions = nullptr;

    const int findStreamInfoRes = avformat_find_stream_info( inputContext, nullptr );
    if( findStreamInfoRes < 0 ) {
        avformat_close_input( &inputContext );
        errorMessage = "Cannot find input stream information";
        return false;
    }

    const int videoStreamIdx = av_find_best_stream( inputContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0 );
    if( videoStreamIdx < 0 ) {
        avformat_close_input( &inputContext );
        errorMessage = "Cannot find a video stream in the input";
        return false;
    }

    for( int i = 0; i < inputContext->nb_streams; ++i )
        m_lastDtsPerStream[ i ] = -1;

    m_videoStreamIdx = videoStreamIdx;
    m_inputContext = inputContext;

    return true;
}

void StreamTester::processPacketDuration( const AVPacket& packet ) {
    if( !m_onPacketDurationCallback )
        return;

    int64_t& lastDts = m_lastDtsPerStream[ packet.stream_index ];
    const int64_t packetDuration = packet.dts - lastDts;
    lastDts = packet.dts;

    const AVRational streamTimeBase = m_inputContext->streams[ packet.stream_index ]->time_base;
    constexpr auto rounding = static_cast< AVRounding >( AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX );
    const auto packetDurationUs =
        std::chrono::microseconds( av_rescale_q_rnd( packetDuration, streamTimeBase, AV_TIME_BASE_Q, rounding ) );

    const auto packetDurationMs = std::chrono::duration_cast< std::chrono::milliseconds >( packetDurationUs );
    if( packetDurationMs > m_packetDurationThreshold )
        m_onPacketDurationCallback( packetDurationMs );
}
